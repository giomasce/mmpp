
#include <z3++.h>

#include "utils/utils.h"
#include "mm/toolbox.h"
#include "mm/setmm.h"

#define VERBOSE_Z3

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
    return static_cast< bool >(is_forall);
}

unsigned expr_get_var_index(const z3::expr &e) {
    assert(e.is_var());
    unsigned idx = Z3_get_index_value(e.ctx(), e);
    e.check_error();
    return idx;
}

/* So far this procedure is not correct, because it does not address very important
 * mismatches between the two languages of Z3 and Metamath: in Z3 function applications,
 * variable binding is explicit and position-based, while in Metamath it is implicit
 * and name-based.
 */
ParsingTree< SymTok, LabTok > expr_to_pt(const z3::expr &e, const LibraryToolbox &tb, std::vector< z3::symbol > &bound_var_stack) {
    if (e.is_app()) {
        z3::func_decl decl = e.decl();
        Z3_decl_kind kind = decl.decl_kind();
        switch (kind) {
        case Z3_OP_TRUE: {
            assert(e.num_args() == 0);
            auto ret = tb.parse_sentence(tb.read_sentence("wff T."));
            assert(ret.label != LabTok{});
            return ret; }
        case Z3_OP_FALSE: {
            assert(e.num_args() == 0);
            auto ret = tb.parse_sentence(tb.read_sentence("wff F."));
            assert(ret.label != LabTok{});
            return ret; }
        case Z3_OP_EQ: {
            assert(e.num_args() == 2);
            auto left = expr_to_pt(e.arg(0), tb, bound_var_stack);
            auto right = expr_to_pt(e.arg(1), tb, bound_var_stack);
            if (left.type == tb.get_symbol("wff")) {
                auto templ = tb.parse_sentence(tb.read_sentence("wff ( ph <-> ps )"));
                assert(templ.label != LabTok{});
                SubstMap< SymTok, LabTok > subst;
                subst[tb.get_var_sym_to_lab(tb.get_symbol("ph"))] = left;
                subst[tb.get_var_sym_to_lab(tb.get_symbol("ps"))] = right;
                return substitute(templ, tb.get_standard_is_var(), subst);
            } else {
                auto templ = tb.parse_sentence(tb.read_sentence("wff A = B"));
                assert(templ.label != LabTok{});
                SubstMap< SymTok, LabTok > subst;
                subst[tb.get_var_sym_to_lab(tb.get_symbol("A"))] = left;
                subst[tb.get_var_sym_to_lab(tb.get_symbol("B"))] = right;
                return substitute(templ, tb.get_standard_is_var(), subst);
            } }
        case Z3_OP_AND: {
            assert(e.num_args() >= 2);
            auto templ = tb.parse_sentence(tb.read_sentence("wff ( ph /\\ ps )"));
            assert(templ.label != LabTok{});
            auto ret = expr_to_pt(e.arg(0), tb, bound_var_stack);
            for (unsigned i = 1; i < e.num_args(); i++) {
                SubstMap< SymTok, LabTok > subst;
                subst[tb.get_var_sym_to_lab(tb.get_symbol("ph"))] = ret;
                subst[tb.get_var_sym_to_lab(tb.get_symbol("ps"))] = expr_to_pt(e.arg(i), tb, bound_var_stack);
                ret = substitute(templ, tb.get_standard_is_var(), subst);
            }
            return ret; }
        case Z3_OP_OR: {
            assert(e.num_args() >= 2);
            auto templ = tb.parse_sentence(tb.read_sentence("wff ( ph \\/ ps )"));
            assert(templ.label != LabTok{});
            auto ret = expr_to_pt(e.arg(0), tb, bound_var_stack);
            for (unsigned i = 1; i < e.num_args(); i++) {
                SubstMap< SymTok, LabTok > subst;
                subst[tb.get_var_sym_to_lab(tb.get_symbol("ph"))] = ret;
                subst[tb.get_var_sym_to_lab(tb.get_symbol("ps"))] = expr_to_pt(e.arg(i), tb, bound_var_stack);
                ret = substitute(templ, tb.get_standard_is_var(), subst);
            }
            return ret; }
        case Z3_OP_IFF: {
            assert(e.num_args() == 2);
            auto left = expr_to_pt(e.arg(0), tb, bound_var_stack);
            auto right = expr_to_pt(e.arg(1), tb, bound_var_stack);
            auto templ = tb.parse_sentence(tb.read_sentence("wff ( ph <-> ps )"));
            assert(templ.label != LabTok{});
            SubstMap< SymTok, LabTok > subst;
            subst[tb.get_var_sym_to_lab(tb.get_symbol("ph"))] = left;
            subst[tb.get_var_sym_to_lab(tb.get_symbol("ps"))] = right;
            return substitute(templ, tb.get_standard_is_var(), subst); }
        case Z3_OP_XOR: {
            assert(e.num_args() == 2);
            auto left = expr_to_pt(e.arg(0), tb, bound_var_stack);
            auto right = expr_to_pt(e.arg(1), tb, bound_var_stack);
            auto templ = tb.parse_sentence(tb.read_sentence("wff ( ph \\/_ ps )"));
            assert(templ.label != LabTok{});
            SubstMap< SymTok, LabTok > subst;
            subst[tb.get_var_sym_to_lab(tb.get_symbol("ph"))] = left;
            subst[tb.get_var_sym_to_lab(tb.get_symbol("ps"))] = right;
            return substitute(templ, tb.get_standard_is_var(), subst); }
        case Z3_OP_NOT: {
            assert(e.num_args() == 1);
            auto body = expr_to_pt(e.arg(0), tb, bound_var_stack);
            auto templ = tb.parse_sentence(tb.read_sentence("wff -. ph"));
            assert(templ.label != LabTok{});
            SubstMap< SymTok, LabTok > subst;
            subst[tb.get_var_sym_to_lab(tb.get_symbol("ph"))] = body;
            return substitute(templ, tb.get_standard_is_var(), subst); }
        case Z3_OP_IMPLIES: {
            assert(e.num_args() == 2);
            auto left = expr_to_pt(e.arg(0), tb, bound_var_stack);
            auto right = expr_to_pt(e.arg(1), tb, bound_var_stack);
            auto templ = tb.parse_sentence(tb.read_sentence("wff ( ph -> ps )"));
            assert(templ.label != LabTok{});
            SubstMap< SymTok, LabTok > subst;
            subst[tb.get_var_sym_to_lab(tb.get_symbol("ph"))] = left;
            subst[tb.get_var_sym_to_lab(tb.get_symbol("ps"))] = right;
            return substitute(templ, tb.get_standard_is_var(), subst); }
        case Z3_OP_UNINTERPRETED: {
            //assert(e.num_args() == 0);
            ParsingTree< SymTok, LabTok > ret;
            if (e.is_bool()) {
                ret.type = tb.get_symbol("wff");
                auto sym = tb.get_symbol(decl.name().str());
                assert(sym != SymTok{});
                ret.label = tb.get_var_sym_to_lab(sym);
                return ret;
            } else {
                ret = tb.parse_sentence(tb.read_sentence("class x"));
                assert(ret.label != LabTok{});
                SubstMap< SymTok, LabTok > subst;
                subst[tb.get_var_sym_to_lab(tb.get_symbol("x"))].type = tb.get_symbol("set");
                auto sym = tb.get_symbol(decl.name().str());
                assert(sym != SymTok{});
                subst[tb.get_var_sym_to_lab(tb.get_symbol("x"))].label = tb.get_var_sym_to_lab(sym);
                ret = substitute(ret, tb.get_standard_is_var(), subst);
                return ret;
            }
            break; }
        default:
            throw "Cannot handle this formula";
        }
    } else if (e.is_quantifier()) {
        for (unsigned i = 0; i < expr_get_num_bound(e); i++) {
            bound_var_stack.push_back(expr_get_quantifier_bound_name(e, i));
        }
        ParsingTree< SymTok, LabTok > ret = expr_to_pt(e.body(), tb, bound_var_stack);
        ParsingTree< SymTok, LabTok > templ;
        if (expr_is_quantifier_forall(e)) {
            templ = tb.parse_sentence(tb.read_sentence("wff A. x ph"));
        } else {
            templ = tb.parse_sentence(tb.read_sentence("wff E. x ph"));
        }
        assert(templ.label != LabTok{});
        for (unsigned i = 0; i < expr_get_num_bound(e); i++) {
            SubstMap< SymTok, LabTok > subst;
            subst[tb.get_var_sym_to_lab(tb.get_symbol("x"))].type = tb.get_symbol("set");
            auto sym = tb.get_symbol(bound_var_stack.back().str());
            assert(sym != SymTok{});
            subst[tb.get_var_sym_to_lab(tb.get_symbol("x"))].label = tb.get_var_sym_to_lab(sym);
            subst[tb.get_var_sym_to_lab(tb.get_symbol("ph"))] = ret;
            ret = substitute(templ, tb.get_standard_is_var(), subst);
            bound_var_stack.pop_back();
        }
        return ret;
    } else if (e.is_var()) {
        ParsingTree< SymTok, LabTok > ret;
        SubstMap< SymTok, LabTok > subst;
        unsigned idx = expr_get_var_index(e);
        auto z3sym = bound_var_stack.at(bound_var_stack.size() - 1 - idx);
        ret = tb.parse_sentence(tb.read_sentence("class x"));
        assert(ret.label != LabTok{});
        subst[tb.get_var_sym_to_lab(tb.get_symbol("x"))].type = tb.get_symbol("set");
        auto sym = tb.get_symbol(z3sym.str());
        assert(sym != SymTok{});
        subst[tb.get_var_sym_to_lab(tb.get_symbol("x"))].label = tb.get_var_sym_to_lab(sym);
        ret = substitute(ret, tb.get_standard_is_var(), subst);
        return ret;
    } else {
        throw "Cannot handle this expression";
    }
}

ParsingTree< SymTok, LabTok > expr_to_pt(const z3::expr &e, const LibraryToolbox &tb) {
    std::vector< z3::symbol > bound_var_stack;
    return expr_to_pt(e, tb, bound_var_stack);
}

void scan_proof(const z3::expr &e, const LibraryToolbox &tb, int depth = 0) {
    if (e.is_app()) {
        z3::func_decl decl = e.decl();
        auto num_args = e.num_args();
        auto arity = decl.arity();
#ifdef NDEBUG
        (void) arity;
#endif
        Z3_decl_kind kind = decl.decl_kind();

        if (Z3_OP_PR_UNDEF <= kind && kind < Z3_OP_PR_UNDEF + 0x100) {
            // Proof expressions, see the documentation of Z3_decl_kind,
            // for example in https://z3prover.github.io/api/html/group__capi.html#ga1fe4399e5468621e2a799a680c6667cd
            auto thesis_pt = expr_to_pt(e.arg(num_args-1), tb);
#ifdef VERBOSE_Z3
            std::cout << std::string(depth, ' ');
            std::cout << "Declaration: " << decl << " of arity " << arity << " and args num " << num_args << ": " << tb.print_sentence(thesis_pt) << std::endl;
#endif

            switch (kind) {
            case Z3_OP_PR_ASSERTED:
                break;
            }
        } else {
#ifdef VERBOSE_Z3
            std::cout << "unknown kind " << kind << std::endl;
#endif
            throw "Unknown kind";
        }

        for (unsigned i = 0; i < num_args-1; i++) {
            scan_proof(e.arg(i), tb, depth+1);
        }
    } else {
        throw "Expression is not an application";
    }

    /*sort s = e.get_sort();
    cout << string(depth, ' ') << "sort: " << s << " (" << s.sort_kind() << "), kind: " << e.kind() << ", num_args: " << e.num_args() << endl;*/
}

static bool recognize(const ParsingTree< SymTok, LabTok > &pt, const std::string &model, const LibraryToolbox &tb, SubstMap< SymTok, LabTok > &subst) {
    UnilateralUnificator< SymTok, LabTok > unif(tb.get_standard_is_var());
    auto model_pt = tb.parse_sentence(tb.read_sentence(model));
    model_pt.validate(tb.get_validation_rule());
    assert(model_pt.label != LabTok{});
    unif.add_parsing_trees(model_pt, pt);
    std::set< LabTok > model_vars;
    collect_variables(model_pt, tb.get_standard_is_var(), model_vars);
    bool ret;
    std::tie(ret, subst) = unif.unify();
    if (ret) {
        for (const auto var : model_vars) {
            ParsingTree< SymTok, LabTok > pt_var;
            pt_var.label = var;
            pt_var.type = tb.get_var_lab_to_type_sym(var);
            subst.insert(std::make_pair(var, pt_var));
        }
    }
    return ret;
}

z3::expr convert_to_z3(const ParsingTree< SymTok, LabTok > &pt, const LibraryToolbox &tb, const std::set< LabTok > &set_vars, z3::sort &set_sort, z3::context &ctx) {
    assert(pt.label != LabTok{});
    SubstMap< SymTok, LabTok > subst;
    if (recognize(pt, "wff A = B", tb, subst)) {
        auto left = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("A"))), tb, set_vars, set_sort, ctx);
        auto right = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("B"))), tb, set_vars, set_sort, ctx);
        return (left == right);
    } else if (recognize(pt, "wff -. ph", tb, subst)) {
        auto op = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), tb, set_vars, set_sort, ctx);
        return !op;
    } else if (recognize(pt, "wff ( ph -> ps )", tb, subst)) {
        auto left = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), tb, set_vars, set_sort, ctx);
        auto right = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ps"))), tb, set_vars, set_sort, ctx);
        return implies(left, right);
    } else if (recognize(pt, "wff ( ph /\\ ps )", tb, subst)) {
        auto left = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), tb, set_vars, set_sort, ctx);
        auto right = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ps"))), tb, set_vars, set_sort, ctx);
        return left && right;
    } else if (recognize(pt, "wff ( ph \\/ ps )", tb, subst)) {
        auto left = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), tb, set_vars, set_sort, ctx);
        auto right = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ps"))), tb, set_vars, set_sort, ctx);
        return left || right;
    } else if (recognize(pt, "wff ( ph <-> ps )", tb, subst)) {
        auto left = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), tb, set_vars, set_sort, ctx);
        auto right = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ps"))), tb, set_vars, set_sort, ctx);
        return left == right;
    } else if (recognize(pt, "wff A. x ph", tb, subst)) {
        auto var = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("x"))), tb, set_vars, set_sort, ctx);
        auto body = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), tb, set_vars, set_sort, ctx);
        return forall(var, body);
    } else if (recognize(pt, "wff E. x ph", tb, subst)) {
        auto var = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("x"))), tb, set_vars, set_sort, ctx);
        auto body = convert_to_z3(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))), tb, set_vars, set_sort, ctx);
        return exists(var, body);
    } else if (recognize(pt, "class x", tb, subst)) {
        return ctx.constant(tb.resolve_symbol(tb.get_var_lab_to_sym(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("x"))).label)).c_str(), set_sort);
    } else if (recognize(pt, "set x", tb, subst)) {
        return ctx.constant(tb.resolve_symbol(tb.get_var_lab_to_sym(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("x"))).label)).c_str(), set_sort);
    } else if (recognize(pt, "wff ph", tb, subst)) {
        z3::sort_vector sorts(ctx);
        z3::expr_vector args(ctx);
        for (const auto x : set_vars) {
            sorts.push_back(set_sort);
            args.push_back(ctx.constant(tb.resolve_symbol(tb.get_var_lab_to_sym(x)).c_str(), set_sort));
        }
        auto func = ctx.function(tb.resolve_symbol(tb.get_var_lab_to_sym(subst.at(tb.get_var_sym_to_lab(tb.get_symbol("ph"))).label)).c_str(), sorts, ctx.bool_sort());
        return func(args);
    } else {
        assert(!"Should not arrive here");
    }
}

int test_z3_2_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    try {
        z3::set_param("proof", true);
        z3::context c;
        //Z3_set_ast_print_mode(c, Z3_PRINT_LOW_LEVEL);

        z3::solver s(c);
        /*z3::params param(c);
        param.set("mbqi", true);
        s.set(param);*/

        /*z3::sort things = c.uninterpreted_sort("thing");
        z3::sort bool_sort = c.bool_sort();
        z3::func_decl man = c.function("man", 1, &things, bool_sort);
        z3::func_decl mortal = c.function("mortal", 1, &things, bool_sort);
        z3::expr socrates = c.constant("socrates", things);
        z3::expr x = c.constant("x", things);
        z3::expr hyp1 = forall(x, implies(man(x), mortal(x)));
        z3::expr hyp2 = man(socrates);
        z3::expr thesis = mortal(socrates);
        s.add(hyp1);
        s.add(hyp2);
        s.add(!thesis);*/

        z3::sort sets = c.uninterpreted_sort("set");
        z3::sort bools = c.bool_sort();
        z3::expr x = c.constant("x", sets);
        z3::expr y = c.constant("y", sets);
        z3::func_decl p = c.function("p", sets, bools);
        z3::func_decl q = c.function("q", sets, bools);
        //z3::expr thesis = (exists(x, forall(y, p(x) == p(y))) == (exists(x, q(x)) == exists(y, q(y)))) == (exists(x, forall(y, q(x) == q(y))) == (exists(x, p(x)) == exists(y, p(y))));
        //z3::expr thesis = (x == x) && forall(x, x == x);
        auto pt = tb.parse_sentence(tb.read_sentence("|- ( ( ( y = z -> ( ( x = y -> ph ) /\\ E. x ( x = y /\\ ph ) ) ) /\\ E. y ( y = z /\\ ( ( x = y -> ph ) /\\ E. x ( x = y /\\ ph ) ) ) ) <-> ( ( y = z -> ( ( x = z -> ph ) /\\ E. x ( x = z /\\ ph ) ) ) /\\ E. y ( y = z /\\ ( ( x = z -> ph ) /\\ E. x ( x = z /\\ ph ) ) ) ) )"));
        //auto pt = tb.parse_sentence(tb.read_sentence("|- A. x x = x"));
        std::set< LabTok > set_vars;
        collect_variables(pt, std::function< bool(LabTok) >([&tb](auto x) { return tb.get_standard_is_var()(x) && tb.get_var_lab_to_type_sym(x) == tb.get_symbol("set"); }), set_vars);
        z3::expr thesis = convert_to_z3(pt, tb, set_vars, sets, c);
        std::vector< z3::symbol > tmp;
        auto pt_thesis = expr_to_pt(thesis, tb, tmp);
        assert(pt_thesis.validate(tb.get_validation_rule()));
        s.add(!thesis);

        std::cout << "Recostructed PT: " << tb.print_sentence(pt_thesis) << std::endl;

        std::cout << "Solver: " << std::endl << s << std::endl;

        switch (s.check()) {
        case z3::unsat:   std::cout << "UNSAT\n"; break;
        case z3::sat:     std::cout << "SAT\n"; break;
        case z3::unknown: std::cout << "UNKNOWN\n"; break;
        }

        auto proof = s.proof();
        //std::cout << "Proof:" << std::endl << proof << std::endl;
        scan_proof(proof, tb);

        std::cout << std::endl;
    } catch (z3::exception &e) {
        std::cerr << "Caught exception:\n" << e << std::endl;
    }

    return 0;
}
static_block {
    register_main_function("test_z3_2", test_z3_2_main);
}
