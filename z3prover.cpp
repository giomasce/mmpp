
#include <z3++.h>

#include "z3prover.h"

#include "wff.h"
#include "parser.h"
#include "memory.h"
#include "utils.h"

using namespace std;
using namespace z3;

pwff parse_expr(expr e) {
    assert(e.is_app());
    func_decl decl = e.decl();
    Z3_decl_kind kind = decl.decl_kind();
    pwff ret;
    switch (kind) {
    case Z3_OP_TRUE:
        assert(e.num_args() == 0);
        return make_shared< True >();
    case Z3_OP_FALSE:
        assert(e.num_args() == 0);
        return make_shared< False >();
    case Z3_OP_EQ:
        // Interpreted as biimplication; is this correct?
        assert(e.num_args() == 2);
        return make_shared< Biimp >(parse_expr(e.arg(0)), parse_expr(e.arg(1)));
    case Z3_OP_AND:
        assert(e.num_args() >= 2);
        ret = make_shared< And >(parse_expr(e.arg(0)), parse_expr(e.arg(1)));
        for (unsigned i = 2; i < e.num_args(); i++) {
            ret = make_shared< And >(ret, parse_expr(e.arg(i)));
        }
        return ret;
    case Z3_OP_OR:
        assert(e.num_args() >= 2);
        ret = make_shared< Or >(parse_expr(e.arg(0)), parse_expr(e.arg(1)));
        for (unsigned i = 2; i < e.num_args(); i++) {
            ret = make_shared< Or >(ret, parse_expr(e.arg(i)));
        }
        return ret;
    case Z3_OP_IFF:
        assert(e.num_args() == 2);
        return make_shared< Biimp >(parse_expr(e.arg(0)), parse_expr(e.arg(1)));
    case Z3_OP_XOR:
        assert(e.num_args() == 2);
        return make_shared< Xor >(parse_expr(e.arg(0)), parse_expr(e.arg(1)));
    case Z3_OP_NOT:
        assert(e.num_args() == 1);
        return make_shared< Not >(parse_expr(e.arg(0)));
    case Z3_OP_IMPLIES:
        assert(e.num_args() == 2);
        return make_shared< Imp >(parse_expr(e.arg(0)), parse_expr(e.arg(1)));

    case Z3_OP_UNINTERPRETED:
        assert(e.num_args() == 0);
        return make_shared< Var >(decl.name().str());

    default:
        throw "Cannot handle this formula";
    }
}

expr extract_thesis(expr proof) {
    return proof.arg(proof.num_args()-1);
}

void prove_and_print(pwff wff, const LibraryToolbox &tb) {
    ProofEngine engine(tb.get_library());
    wff->get_adv_truth_prover(tb)(engine);
    if (engine.get_proof_labels().size() > 0) {
        //cout << "adv truth proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
        cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
        cout << "proof length: " << engine.get_proof_labels().size() << endl;
    }
}

RegisteredProver ass_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ph )");
RegisteredProver mp1_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( ph -> ( ps -> ch ) )"}, "|- ( ph -> ch )");
RegisteredProver mp2_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ch )");
RegisteredProver tr_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( ch <-> th ) )"}, "|- ( ph -> ( ps <-> th ) )");

Prover iterate_expr(pwff w, expr e, const LibraryToolbox &tb, int depth = 0, expr *parent = NULL) {
    using z3::sort;
    if (e.is_app()) {
        func_decl decl = e.decl();
        unsigned num_args = e.num_args();
        unsigned arity = decl.arity();
        Z3_decl_kind kind = decl.decl_kind();

        if (Z3_OP_PR_UNDEF <= kind && kind < Z3_OP_PR_UNDEF + 0x100) {
            // Proof expressions, see the documentation of Z3_decl_kind,
            // for example in https://z3prover.github.io/api/html/group__capi.html#ga1fe4399e5468621e2a799a680c6667cd
            cout << string(depth, ' ');
            cout << "Declaration: " << decl << " of arity " << decl.arity() << " and args num " << num_args << endl;

            switch (kind) {
            case Z3_OP_PR_ASSERTED:
                cout << "asserted fact";
                assert(num_args == 1);
                assert(arity == 1);
                assert(w->to_string() == parse_expr(e.arg(0))->to_string());
                //cout << "EXPR: " << e.arg(0) << endl;
                //cout << "WFF: " << parse_expr(e.arg(0))->to_string() << endl;
                return tb.build_registered_prover(ass_rp, {{"ph", w->get_type_prover(tb)}}, {});
                break;
            case Z3_OP_PR_MODUS_PONENS:
                assert(num_args == 3);
                assert(arity == 3);
                //cout << endl << "EXPR: " << e.arg(2);
                /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                cout << "HP2: " << parse_expr(extract_thesis(e.arg(1)))->to_string() << endl;
                cout << "TH: " << parse_expr(e.arg(2))->to_string() << endl;*/
                {
                    Prover p1 = iterate_expr(w, e.arg(0), tb, depth+1, &e);
                    Prover p2 = iterate_expr(w, e.arg(1), tb, depth+1, &e);
                    Prover mp1 = tb.build_registered_prover(mp1_rp, {{"ph", w->get_type_prover(tb)}, {"ps", parse_expr(extract_thesis(e.arg(0)))->get_type_prover(tb)}, {"ch", parse_expr(e.arg(2))->get_type_prover(tb)}}, {p1, p2});
                    Prover mp2 = tb.build_registered_prover(mp2_rp, {{"ph", w->get_type_prover(tb)}, {"ps", parse_expr(extract_thesis(e.arg(0)))->get_type_prover(tb)}, {"ch", parse_expr(e.arg(2))->get_type_prover(tb)}}, {p1, p2});
                    return cascade_provers(mp1, mp2);
                }
                break;
            /*case Z3_OP_PR_REWRITE:
                cout << "rewrite";
                assert(num_args == 1);
                assert(arity == 1);
                //cout << endl << "EXPR: " << e.arg(0);
                cout << endl << "WFF: " << parse_expr(e.arg(0))->to_string();
                //prove_and_print(parse_expr(e.arg(0)), tb);
                break;*/
            case Z3_OP_PR_TRANSITIVITY:
                cout << "transitivity";
                assert(num_args == 3);
                assert(arity == 3);
                //cout << endl << "EXPR: " << e.arg(2);
                /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                cout << "HP2: " << parse_expr(extract_thesis(e.arg(1)))->to_string() << endl;
                cout << "TH: " << parse_expr(e.arg(2))->to_string() << endl;*/
                {
                    Prover p1 = iterate_expr(w, e.arg(0), tb, depth+1, &e);
                    Prover p2 = iterate_expr(w, e.arg(1), tb, depth+1, &e);
                    shared_ptr< Biimp > w1 = dynamic_pointer_cast< Biimp >(parse_expr(extract_thesis(e.arg(0))));
                    shared_ptr< Biimp > w2 = dynamic_pointer_cast< Biimp >(parse_expr(extract_thesis(e.arg(1))));
                    assert(w1 != NULL && w2 != NULL);
                    assert(w1->get_b()->to_string() == w2->get_a()->to_string());
                    pwff ps = w1->get_a();
                    pwff ch = w1->get_b();
                    pwff th = w2->get_b();
                    return tb.build_registered_prover(tr_rp, {{"ph", w->get_type_prover(tb)}, {"ps", ps->get_type_prover(tb)}, {"ch", ch->get_type_prover(tb)}, {"th", th->get_type_prover(tb)}}, {p1, p2});
                }
                break;
            /*case Z3_OP_PR_MONOTONICITY:
                cout << "monotonicity";
                assert(num_args == 2);
                assert(arity == 2);
                //cout << endl << "EXPR: " << e.arg(2);
                cout << endl << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string();
                //cout << endl << "TH: " << parse_expr(e.arg(1))->to_string();
                break;
            case Z3_OP_PR_UNIT_RESOLUTION:
                cout << "unit resolution";
                break;
            case Z3_OP_PR_DEF_AXIOM:
                cout << "Tseitin's axiom";
                assert(num_args == 1);
                assert(arity == 1);
                //cout << endl << "EXPR: " << e.arg(0);
                cout << endl << "WFF: " << parse_expr(e.arg(0))->to_string();
                break;
            case Z3_OP_PR_NOT_OR_ELIM:
                cout << "not-or elimination";
                assert(num_args == 2);
                assert(arity == 2);
                //cout << endl << "EXPR: " << e.arg(2);
                cout << endl << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string();
                cout << endl << "TH: " << parse_expr(e.arg(1))->to_string();
                break;
            case Z3_OP_PR_IFF_FALSE:
                cout << "iff false";
                assert(num_args == 2);
                assert(arity == 2);
                //cout << endl << "EXPR: " << e.arg(2);
                cout << endl << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string();
                cout << endl << "TH: " << parse_expr(e.arg(1))->to_string();
                break;
            case Z3_OP_PR_LEMMA:
                cout << "lemma";
                assert(num_args == 2);
                assert(arity == 2);
                //cout << endl << "EXPR: " << e.arg(2);
                cout << endl << "FULL: " << e;
                cout << endl << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string();
                cout << endl << "TH: " << parse_expr(e.arg(1))->to_string();
                break;
            case Z3_OP_PR_HYPOTHESIS:
                cout << "hypothesis";
                assert(num_args == 1);
                assert(arity == 1);
                cout << endl << "PARENT: " << *parent;
                //cout << endl << "EXPR: " << e.arg(0);
                cout << endl << "WFF: " << parse_expr(e.arg(0))->to_string();
                break;*/
            default:
                cout << "ORACLE!" << endl;
                prove_and_print(make_shared< Imp >(w, parse_expr(extract_thesis(e))), tb);
                return make_shared< Imp >(w, parse_expr(extract_thesis(e)))->get_adv_truth_prover(tb);
                break;
            }
        } else if ((Z3_OP_TRUE <= kind && kind < Z3_OP_TRUE + 0x100) || kind == Z3_OP_UNINTERPRETED) {
            // Logic formulae, we just ignore
        } else {
            cout << "unknown kind " << kind;
            throw "Unkown kind";
        }

        /*for (unsigned i = 0; i < num_args; i++) {
            iterate_expr(e.arg(i), tb, depth+1, &e);
        }*/
    } else {
        throw "Expression is not an application";
    }

    /*sort s = e.get_sort();
    cout << string(depth, ' ') << "sort: " << s << " (" << s.sort_kind() << "), kind: " << e.kind() << ", num_args: " << e.num_args() << endl;*/
}

void add_formula(solver &s, pwff &w, expr e, bool hyp) {
    if (hyp) {
        s.add(e);
    } else {
        s.add(!e);
    }
    if (w == NULL) {
        w = parse_expr(e);
    } else {
        if (hyp) {
            w = make_shared< And >(w, parse_expr(e));
        } else {
            w = make_shared< Imp >(w, parse_expr(e));
        }
    }
}

RegisteredProver refute_rp = LibraryToolbox::register_prover({"|- ( -. ph -> F. )"}, "|- ph");

int test_z3_main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    cout << "Reading set.mm..." << endl;
    FileTokenizer ft("../set.mm/set.mm");
    Parser p(ft, false, true);
    p.run();
    LibraryImpl lib = p.get_library();
    LibraryToolbox tb(lib, true);
    cout << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << endl;
    cout << "Memory usage after loading the library: " << size_to_string(getCurrentRSS()) << endl;

    set_param("proof", true);
    context c;

    pwff wff = NULL;
    solver s(c);

    expr x = c.bool_const("ph");
    expr y = c.bool_const("ps");
    add_formula(s, wff, ((!(x && y)) == (!x || !y)), false);

    /*expr ph = c.bool_const("ph");
    expr ps = c.bool_const("ps");
    expr ch = c.bool_const("ch");
    add_formula(s, wff, implies(ph, implies(ps, ch)), true);
    add_formula(s, wff, implies(!ph, implies(!ps, ch)), true);
    add_formula(s, wff, implies(ph == ps, ch), false);*/

    cout << "ORIG: " << wff->to_string() << endl;
    prove_and_print(wff, tb);

    switch (s.check()) {
    case unsat:   cout << "valid\n"; break;
    case sat:     cout << "not valid\n"; break;
    case unknown: cout << "unknown\n"; break;
    }

    ProofEngine engine(lib, true);
    Prover main_prover = iterate_expr(make_shared< Not >(wff), s.proof(), tb);
    bool res = tb.build_registered_prover(refute_rp, {{"ph", wff->get_type_prover(tb)}}, {main_prover})(engine);
    if (res) {
        cout << endl << "FINAL PROOF FOUND!" << endl;
        cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
        cout << "proof length: " << engine.get_proof_labels().size() << endl;
    }

    return 0;
}
