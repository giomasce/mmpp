
#include <z3++.h>

#include <giolib/static_block.h>
#include <giolib/main.h>
#include <giolib/memory.h>
#include <giolib/assert.h>

#define GIO_STD_PRINTERS_INJECT_IN_STD
#include <giolib/std_printers.h>

#include "utils/utils.h"
#include "mm/toolbox.h"
#include "mm/setmm.h"
#include "z3resolver.h"
#include "fof.h"

namespace gio::mmpp::z3prover {

unsigned expr_get_num_bound(const z3::expr &e) {
    assert(e.is_quantifier());
    unsigned num = Z3_get_quantifier_num_bound(e.ctx(), e);
    e.check_error();
    return num;
}

z3::symbol expr_get_quantifier_bound_name(const z3::expr &e, unsigned i) {
    assert(e.is_quantifier());
    Z3_symbol sym = Z3_get_quantifier_bound_name(e.ctx(), e, i);
    e.check_error();
    return z3::symbol(e.ctx(), sym);
}

z3::sort expr_get_quantifier_bound_sort(const z3::expr &e, unsigned i) {
    assert(e.is_quantifier());
    Z3_sort sort = Z3_get_quantifier_bound_sort(e.ctx(), e, i);
    e.check_error();
    return z3::sort(e.ctx(), sort);
}

bool expr_is_quantifier_forall(const z3::expr &e) {
    assert(e.is_quantifier());
    Z3_bool is_forall = Z3_is_quantifier_forall(e.ctx(), e);
    e.check_error();
    return static_cast<bool>(is_forall);
}

unsigned expr_get_var_index(const z3::expr &e) {
    assert(e.is_var());
    unsigned idx = Z3_get_index_value(e.ctx(), e);
    e.check_error();
    return idx;
}

struct FormulaRecostructor {
    std::shared_ptr<const gio::mmpp::provers::fof::FOF> operator()(const z3::expr &expr) {
        return this->reconstruct_formula(expr);
    }

    std::shared_ptr<const gio::mmpp::provers::fof::FOT> reconstruct_term(const z3::expr &expr) {
        using namespace gio::mmpp::provers::fof;
        if (expr.is_app()) {
            auto decl = expr.decl();
            auto num_args = expr.num_args();
            auto arity = decl.arity();
            auto kind = decl.decl_kind();

            gio::assert_or_throw<gio::debug_exception>(arity == num_args, std::make_pair(arity, num_args));

            switch (kind) {
            case Z3_OP_UNINTERPRETED: {
                std::vector<std::shared_ptr<const FOT>> args;
                for (unsigned i = 0; i < num_args; i++) {
                    args.push_back(this->reconstruct_term(expr.arg(i)));
                }
                return Functor::create(decl.name().str(), std::move(args)); }
            default:
                throw gio::debug_exception(kind);
            }
        } else if (expr.is_var()) {
            unsigned idx = expr_get_var_index(expr);
            std::string name;
            if (idx == 0 && this->bound_var_stack.empty()) {
                name = "•";
            } else {
                name = this->bound_var_stack.at(this->bound_var_stack.size() - 1 - idx).str();
            }
            return Variable::create(name);
        } else {
            throw gio::debug_exception(expr);
        }
    }

    std::shared_ptr<const gio::mmpp::provers::fof::FOF> reconstruct_formula(const z3::expr &expr) {
        using namespace gio::mmpp::provers::fof;
        if (expr.is_app()) {
            auto decl = expr.decl();
            auto num_args = expr.num_args();
            auto arity = decl.arity();
            auto kind = decl.decl_kind();

            gio::assert_or_throw<gio::debug_exception>(arity == num_args, std::make_pair(arity, num_args));

            switch (kind) {
            case Z3_OP_TRUE:
                assert_or_throw<gio::debug_exception>(num_args == 0);
                return True::create();
            case Z3_OP_FALSE:
                assert_or_throw<gio::debug_exception>(num_args == 0);
                return False::create();
            case Z3_OP_EQ:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Equal::create(this->reconstruct_term(expr.arg(0)), this->reconstruct_term(expr.arg(1)));
            case Z3_OP_DISTINCT:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Distinct::create(this->reconstruct_term(expr.arg(0)), this->reconstruct_term(expr.arg(1)));
            case Z3_OP_AND:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return And::create(this->reconstruct_formula(expr.arg(0)), this->reconstruct_formula(expr.arg(1)));
            case Z3_OP_OR:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Or::create(this->reconstruct_formula(expr.arg(0)), this->reconstruct_formula(expr.arg(1)));
            case Z3_OP_IFF:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Iff::create(this->reconstruct_formula(expr.arg(0)), this->reconstruct_formula(expr.arg(1)));
            case Z3_OP_XOR:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Xor::create(this->reconstruct_formula(expr.arg(0)), this->reconstruct_formula(expr.arg(1)));
            case Z3_OP_NOT:
                assert_or_throw<gio::debug_exception>(num_args == 1);
                return Not::create(this->reconstruct_formula(expr.arg(0)));
            case Z3_OP_IMPLIES:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Implies::create(this->reconstruct_formula(expr.arg(0)), this->reconstruct_formula(expr.arg(1)));
            case Z3_OP_OEQ:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Oeq::create(this->reconstruct_formula(expr.arg(0)), this->reconstruct_formula(expr.arg(1)));
            case Z3_OP_UNINTERPRETED: {
                std::vector<std::shared_ptr<const FOT>> args;
                for (unsigned i = 0; i < num_args; i++) {
                    args.push_back(this->reconstruct_term(expr.arg(i)));
                }
                return Predicate::create(decl.name().str(), std::move(args)); }
            default:
                throw gio::debug_exception(kind);
            }
        } else if (expr.is_quantifier()) {
            for (unsigned i = 0; i < expr_get_num_bound(expr); i++) {
                this->bound_var_stack.push_back(expr_get_quantifier_bound_name(expr, i));
            }
            auto ret = this->reconstruct_formula(expr.body());
            bool is_forall = expr_is_quantifier_forall(expr);
            for (unsigned i = 0; i < expr_get_num_bound(expr); i++) {
                if (is_forall) {
                    ret = Forall::create(Variable::create(this->bound_var_stack.back().str()), ret);
                } else {
                    ret = Exists::create(Variable::create(this->bound_var_stack.back().str()), ret);
                }
                this->bound_var_stack.pop_back();
            }
            return ret;
        } else {
            throw gio::debug_exception(expr);
        }
    }

    std::vector<z3::symbol> bound_var_stack;
};

void pretty_print_proof(const z3::expr &proof, unsigned depth = 0) {
    gio::assert_or_throw<gio::debug_exception>(proof.is_app());
    auto decl = proof.decl();
    auto num_args = proof.num_args();
    auto arity = decl.arity();
    auto kind = decl.decl_kind();
    const auto &formula = proof.arg(num_args-1);

    gio::assert_or_throw<gio::debug_exception>(arity == num_args, std::make_pair(arity, num_args));
    gio::assert_or_throw<gio::debug_exception>(Z3_OP_PR_UNDEF <= kind && kind < Z3_OP_PR_UNDEF + 0x100);

    std::cout << std::string(2*depth, ' ');
    //std::cout << formula << "\n";
    std::cout << "By " << get_inference_str(kind) << ": ";
    //try {
    std::cout << *FormulaRecostructor()(formula) << "\n";
    //} catch (...) {
    //    std::cout << "Failed with " << formula << "\n";
    //}

    for (unsigned i = 0; i < num_args-1; i++) {
        pretty_print_proof(proof.arg(i), depth+1);
    }
}

int test_z3_3_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    //auto &data = get_set_mm();
    //auto &lib = data.lib;
    //auto &tb = data.tb;

    z3::set_param("proof", true);
    z3::context c;
    //Z3_set_ast_print_mode(c, Z3_PRINT_LOW_LEVEL);

    z3::solver s(c);
    /*z3::params param(c);
    param.set("mbqi", true);
    s.set(param);*/

    /*z3::sort sets = c.uninterpreted_sort("set");
    z3::sort bools = c.bool_sort();
    z3::expr x = c.constant("x", sets);
    z3::expr y = c.constant("y", sets);
    z3::func_decl p = c.function("p", sets, bools);
    z3::func_decl q = c.function("q", sets, bools);
    s.add(x != x);*/

    z3::sort sets = c.uninterpreted_sort("set");
    z3::sort bools = c.bool_sort();
    z3::expr x = c.constant("x", sets);
    z3::expr y = c.constant("y", sets);
    z3::func_decl p = c.function("p", sets, sets, bools);
    z3::expr conj = implies(exists(x, forall(y, p(x,y))), forall(y, exists(x, p(x,y))));
    //z3::expr conj = implies(forall(y, exists(x, p(x,y))), exists(x, forall(y, p(x,y))));
    s.add(!conj);

    /*z3::expr x      = c.int_const("x");
    z3::expr y      = c.int_const("y");
    z3::expr z      = c.int_const("z");
    z3::sort I      = c.int_sort();
    z3::func_decl g = function("g", I, I);
    z3::expr conjecture1 = implies(g(g(x) - g(y)) != g(z) && x + z <= y && y <= x, z < 0);
    s.add(!conjecture1);*/

    /*z3::expr x = c.int_const("x");
    z3::expr y = c.int_const("y");
    z3::sort I = c.int_sort();
    z3::func_decl f = function("f", I, I, I);
    s.add(forall(x, y, f(x, y) >= 0));
    z3::expr a = c.int_const("a");
    s.add(f(a, a) < a);
    s.add(a < 0);*/

    std::cout << s.check() << "\n";

    std::cout << "Solver: " << std::endl << s << std::endl;

    auto res = s.check();

    if (res == z3::unsat) {
        auto proof = s.proof();
        std::cout << "System is unsatisfiable. Proof:\n";
        std::cout << proof << "\n";
        pretty_print_proof(proof);
    }
    if (res == z3::sat) {
        auto model = s.get_model();
        std::cout << "System is satisfiable. Model:\n" << model << "\n";
    }
    if (res == z3::unknown) {
        std::cout << "System is unknown...\n";
    }

    return 0;
}
gio_static_block {
    gio::register_main_function("test_z3_3", test_z3_3_main);
}

}
