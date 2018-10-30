
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

std::string get_inference_str(unsigned x) {
    switch (x) {
    case Z3_OP_PR_ASSERTED: return "Z3_OP_PR_ASSERTED";
    case Z3_OP_PR_GOAL: return "Z3_OP_PR_GOAL";
    case Z3_OP_PR_MODUS_PONENS: return "Z3_OP_PR_MODUS_PONENS";
    case Z3_OP_PR_REFLEXIVITY: return "Z3_OP_PR_REFLEXIVITY";
    case Z3_OP_PR_SYMMETRY: return "Z3_OP_PR_SYMMETRY";
    case Z3_OP_PR_TRANSITIVITY: return "Z3_OP_PR_TRANSITIVITY";
    case Z3_OP_PR_TRANSITIVITY_STAR: return "Z3_OP_PR_TRANSITIVITY_STAR";
    case Z3_OP_PR_MONOTONICITY: return "Z3_OP_PR_MONOTONICITY";
    case Z3_OP_PR_QUANT_INTRO: return "Z3_OP_PR_QUANT_INTRO";
    case Z3_OP_PR_DISTRIBUTIVITY: return "Z3_OP_PR_DISTRIBUTIVITY";
    case Z3_OP_PR_AND_ELIM: return "Z3_OP_PR_AND_ELIM";
    case Z3_OP_PR_NOT_OR_ELIM: return "Z3_OP_PR_NOT_OR_ELIM";
    case Z3_OP_PR_REWRITE: return "Z3_OP_PR_REWRITE";
    case Z3_OP_PR_REWRITE_STAR: return "Z3_OP_PR_REWRITE_STAR";
    case Z3_OP_PR_PULL_QUANT: return "Z3_OP_PR_PULL_QUANT";
    case Z3_OP_PR_PULL_QUANT_STAR: return "Z3_OP_PR_PULL_QUANT_STAR";
    case Z3_OP_PR_PUSH_QUANT: return "Z3_OP_PR_PUSH_QUANT";
    case Z3_OP_PR_ELIM_UNUSED_VARS: return "Z3_OP_PR_ELIM_UNUSED_VARS";
    case Z3_OP_PR_DER: return "Z3_OP_PR_DER";
    case Z3_OP_PR_QUANT_INST: return "Z3_OP_PR_QUANT_INST";
    case Z3_OP_PR_HYPOTHESIS: return "Z3_OP_PR_HYPOTHESIS";
    case Z3_OP_PR_LEMMA: return "Z3_OP_PR_LEMMA";
    case Z3_OP_PR_UNIT_RESOLUTION: return "Z3_OP_PR_UNIT_RESOLUTION";
    case Z3_OP_PR_IFF_TRUE: return "Z3_OP_PR_IFF_TRUE";
    case Z3_OP_PR_IFF_FALSE: return "Z3_OP_PR_IFF_FALSE";
    case Z3_OP_PR_COMMUTATIVITY: return "Z3_OP_PR_COMMUTATIVITY";
    case Z3_OP_PR_DEF_AXIOM: return "Z3_OP_PR_DEF_AXIOM";
    case Z3_OP_PR_DEF_INTRO: return "Z3_OP_PR_DEF_INTRO";
    case Z3_OP_PR_APPLY_DEF: return "Z3_OP_PR_APPLY_DEF";
    case Z3_OP_PR_IFF_OEQ: return "Z3_OP_PR_IFF_OEQ";
    case Z3_OP_PR_NNF_POS: return "Z3_OP_PR_NNF_POS";
    case Z3_OP_PR_NNF_NEG: return "Z3_OP_PR_NNF_NEG";
    case Z3_OP_PR_NNF_STAR: return "Z3_OP_PR_NNF_STAR";
    case Z3_OP_PR_CNF_STAR: return "Z3_OP_PR_CNF_STAR";
    case Z3_OP_PR_SKOLEMIZE: return "Z3_OP_PR_SKOLEMIZE";
    case Z3_OP_PR_MODUS_PONENS_OEQ: return "Z3_OP_PR_MODUS_PONENS_OEQ";
    case Z3_OP_PR_TH_LEMMA: return "Z3_OP_PR_TH_LEMMA";
    case Z3_OP_PR_HYPER_RESOLVE: return "Z3_OP_PR_HYPER_RESOLVE";
    default: return "(unknown)";
    }
}

class Formula {
public:
    virtual ~Formula() = default;
    virtual void print_to(std::ostream &s) const {
        s << "formula";
    }

protected:
    Formula() = default;
};

class Uninterpreted : public Formula, public gio::virtual_enable_create<Uninterpreted> {
public:
    void print_to(std::ostream &s) const override {
        s << this->name;
        if (!this->args.empty()) {
            s << "(";
            bool first = true;
            for (const auto &arg : this->args) {
                if (!first) {
                    s << ", ";
                }
                first = false;
                s << *arg;
            }
            s << ")";
        }
    }

protected:
    Uninterpreted(const std::string &name, const std::vector<std::shared_ptr<const Formula>> &args) : name(name), args(args) {}

private:
    std::string name;
    std::vector<std::shared_ptr<const Formula>> args;
};

class True : public Formula, public gio::virtual_enable_create<True> {
public:
    void print_to(std::ostream &s) const override {
        s << "⊤";
    }

protected:
    True() {}
};

class False : public Formula, public gio::virtual_enable_create<False> {
public:
    void print_to(std::ostream &s) const override {
        s << "⊥";
    }

protected:
    False() {}
};

class Equal : public Formula, public gio::virtual_enable_create<Equal> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "=" << *this->y << ")";
    }

protected:
    Equal(const std::shared_ptr<const Formula> &x, const std::shared_ptr<const Formula> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const Formula> x;
    std::shared_ptr<const Formula> y;
};

class Distinct : public Formula, public gio::virtual_enable_create<Distinct> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "≠" << *this->y << ")";
    }

protected:
    Distinct(const std::shared_ptr<const Formula> &x, const std::shared_ptr<const Formula> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const Formula> x;
    std::shared_ptr<const Formula> y;
};

class And : public Formula, public gio::virtual_enable_create<And> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "∧" << *this->y << ")";
    }

protected:
    And(const std::shared_ptr<const Formula> &x, const std::shared_ptr<const Formula> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const Formula> x;
    std::shared_ptr<const Formula> y;
};

class Or : public Formula, public gio::virtual_enable_create<Or> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "∨" << *this->y << ")";
    }

protected:
    Or(const std::shared_ptr<const Formula> &x, const std::shared_ptr<const Formula> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const Formula> x;
    std::shared_ptr<const Formula> y;
};

class Iff : public Formula, public gio::virtual_enable_create<Iff> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "⇔" << *this->y << ")";
    }

protected:
    Iff(const std::shared_ptr<const Formula> &x, const std::shared_ptr<const Formula> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const Formula> x;
    std::shared_ptr<const Formula> y;
};

class Xor : public Formula, public gio::virtual_enable_create<Xor> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "⊻" << *this->y << ")";
    }

protected:
    Xor(const std::shared_ptr<const Formula> &x, const std::shared_ptr<const Formula> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const Formula> x;
    std::shared_ptr<const Formula> y;
};

class Not : public Formula, public gio::virtual_enable_create<Not> {
public:
    void print_to(std::ostream &s) const override {
        s << "¬" << *this->x;
    }

protected:
    Not(const std::shared_ptr<const Formula> &x) : x(x) {}

private:
    std::shared_ptr<const Formula> x;
};

class Implies : public Formula, public gio::virtual_enable_create<Implies> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "⇒" << *this->y << ")";
    }

protected:
    Implies(const std::shared_ptr<const Formula> &x, const std::shared_ptr<const Formula> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const Formula> x;
    std::shared_ptr<const Formula> y;
};

class Oeq : public Formula, public gio::virtual_enable_create<Oeq> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "≈" << *this->y << ")";
    }

protected:
    Oeq(const std::shared_ptr<const Formula> &x, const std::shared_ptr<const Formula> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const Formula> x;
    std::shared_ptr<const Formula> y;
};

class Forall : public Formula, public gio::virtual_enable_create<Forall> {
public:
    void print_to(std::ostream &s) const override {
        s << "∀" << this->var << " " << *this->x;
    }

protected:
    Forall(const std::string &var, const std::shared_ptr<const Formula> &x) : var(var), x(x) {}

private:
    std::string var;
    std::shared_ptr<const Formula> x;
};

class Exists : public Formula, public gio::virtual_enable_create<Exists> {
public:
    void print_to(std::ostream &s) const override {
        s << "∃" << this->var << " " << *this->x;
    }

protected:
    Exists(const std::string &var, const std::shared_ptr<const Formula> &x) : var(var), x(x) {}

private:
    std::string var;
    std::shared_ptr<const Formula> x;
};

class Variable : public Formula, public gio::virtual_enable_create<Variable> {
public:
    void print_to(std::ostream &s) const override {
        s << this->name;
    }

protected:
    Variable(const std::string &name) : name(name) {}

private:
    std::string name;
};

struct FormulaRecostructor {
    std::shared_ptr<const Formula> operator()(const z3::expr &expr) {
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
                return Equal::create(this->operator()(expr.arg(0)), this->operator()(expr.arg(1)));
            case Z3_OP_DISTINCT:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Distinct::create(this->operator()(expr.arg(0)), this->operator()(expr.arg(1)));
            case Z3_OP_AND:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return And::create(this->operator()(expr.arg(0)), this->operator()(expr.arg(1)));
            case Z3_OP_OR:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Or::create(this->operator()(expr.arg(0)), this->operator()(expr.arg(1)));
            case Z3_OP_IFF:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Iff::create(this->operator()(expr.arg(0)), this->operator()(expr.arg(1)));
            case Z3_OP_XOR:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Xor::create(this->operator()(expr.arg(0)), this->operator()(expr.arg(1)));
            case Z3_OP_NOT:
                assert_or_throw<gio::debug_exception>(num_args == 1);
                return Not::create(this->operator()(expr.arg(0)));
            case Z3_OP_IMPLIES:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Implies::create(this->operator()(expr.arg(0)), this->operator()(expr.arg(1)));
            case Z3_OP_OEQ:
                assert_or_throw<gio::debug_exception>(num_args == 2);
                return Oeq::create(this->operator()(expr.arg(0)), this->operator()(expr.arg(1)));
            case Z3_OP_UNINTERPRETED: {
                std::vector<std::shared_ptr<const Formula>> args;
                for (unsigned i = 0; i < num_args; i++) {
                    args.push_back(this->operator()(expr.arg(i)));
                }
                return Uninterpreted::create(decl.name().str(), std::move(args)); }
            default:
                throw gio::debug_exception(kind);
            }
        } else if (expr.is_quantifier()) {
            for (unsigned i = 0; i < expr_get_num_bound(expr); i++) {
                this->bound_var_stack.push_back(expr_get_quantifier_bound_name(expr, i));
            }
            auto ret = this->operator()(expr.body());
            bool is_forall = expr_is_quantifier_forall(expr);
            for (unsigned i = 0; i < expr_get_num_bound(expr); i++) {
                if (is_forall) {
                    ret = Forall::create(this->bound_var_stack.back().str(), ret);
                } else {
                    ret = Exists::create(this->bound_var_stack.back().str(), ret);
                }
                this->bound_var_stack.pop_back();
            }
            return ret;
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
