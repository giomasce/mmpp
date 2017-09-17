
#include <unordered_map>

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
        // Interpreted as biimplication; why does Z3 generates equalities between formulae?
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


struct Z3Adapter {
    solver &s;
    const LibraryToolbox &tb;

    vector< pwff > hyps;
    pwff thesis;

    pwff target;
    pwff and_hyps;
    pwff abs;

    Z3Adapter(solver &s, const LibraryToolbox &tb) : s(s), tb(tb) {}

    void add_formula(expr e, bool hyp) {
        if (hyp) {
            this->s.add(e);
        } else {
            this->s.add(!e);
        }
        pwff w = parse_expr(e);
        if (hyp) {
            this->hyps.push_back(w);
            if (this->target == NULL) {
                this->and_hyps = w;
                this->target = w;
                this->abs = w;
            } else {
                this->and_hyps = make_shared< And >(this->and_hyps, w);
                this->target = make_shared< And >(this->target, w);
                this->abs = make_shared< And >(this->abs, w);
            }
        } else {
            this->hyps.push_back(make_shared< Not >(w));
            this->thesis = w;
            if (this->target == NULL) {
                this->target = w;
                this->abs = make_shared< Not >(w);
            } else {
                this->target = make_shared< Imp >(this->target, w);
                this->abs = make_shared< And >(this->abs, make_shared< Not >(w));
            }
        }
    }
};

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

RegisteredProver id_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ph )");
RegisteredProver ff_rp = LibraryToolbox::register_prover({}, "|- ( F. -> F. )");
RegisteredProver orim1i_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )"}, "|- ( ( ph \\/ ch ) -> ( ps \\/ ch ) )");
RegisteredProver orfa_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )"}, "|- ( ( ph \\/ F. ) -> ps )");
RegisteredProver orfa2_rp = LibraryToolbox::register_prover({"|- ( ph -> F. )"}, "|- ( ( ph \\/ ps ) -> ps )");

// Produces a prover for ( ( ... ( ph_1 \/ ph_2 ) ... \/ ph_n ) -> ( ... ( ph_{i_1} \/ ph_{i_2} ) ... \/ ph_{i_k} ) ),
// where all instances of F. have been removed (unless they're all F.'s).
tuple< Prover, pwff, pwff > simplify_or(const vector< pwff > &clauses, const LibraryToolbox &tb) {
    if (clauses.size() == 0) {
        return make_tuple(tb.build_registered_prover(ff_rp, {}, {}), make_shared< False >(), make_shared< False >());
    } else {
        Prover ret = tb.build_registered_prover(id_rp, {{"ph", clauses[0]->get_type_prover(tb)}}, {});
        pwff first = clauses[0];
        pwff second = clauses[0];
        pwff falsum = make_shared< False >();
        for (size_t i = 1; i < clauses.size(); i++) {
            pwff new_clause = clauses[i];
            if (*second == *falsum) {
                ret = tb.build_registered_prover(orfa2_rp, {{"ph", first->get_truth_prover(tb)}, {"ps", new_clause->get_type_prover(tb)}}, {ret});
                second = new_clause;
            } else {
                if (*new_clause == *falsum) {
                    ret = tb.build_registered_prover(orfa_rp, {{"ph", first->get_type_prover(tb)}, {"ps", second->get_type_prover(tb)}}, {ret});
                } else {
                    ret = tb.build_registered_prover(orim1i_rp, {{"ph", first->get_type_prover(tb)}, {"ps", second->get_type_prover(tb)}, {"ch", new_clause->get_type_prover(tb)}}, {ret});
                    second = make_shared< Or >(second, new_clause);
                }
            }
            first = make_shared< Or >(first, new_clause);
        }
        return make_tuple(ret, first, second);
    }
}

RegisteredProver orim12i_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( ch -> th )"}, "|- ( ( ph \\/ ch ) -> ( ps \\/ th ) )");

tuple< Prover, pwff, pwff > join_or_imp(const vector< pwff > &orig_clauses, const vector< pwff > &new_clauses, vector< Prover > &provers, const LibraryToolbox &tb) {
    assert(orig_clauses.size() == new_clauses.size());
    assert(orig_clauses.size() == provers.size());
    if (orig_clauses.size() == 0) {
        return make_tuple(tb.build_registered_prover(ff_rp, {}, {}), make_shared< False >(), make_shared< False >());
    } else {
        Prover ret = provers[0];
        pwff orig_cl = orig_clauses[0];
        pwff new_cl = new_clauses[0];
        for (size_t i = 1; i < orig_clauses.size(); i++) {
            ret = tb.build_registered_prover(orim12i_rp, {{"ph", orig_cl->get_type_prover(tb)}, {"ps", new_cl->get_type_prover(tb)}, {"ch", orig_clauses[i]->get_type_prover(tb)}, {"th", new_clauses[i]->get_type_prover(tb)}}, {ret, provers[i]});
            orig_cl = make_shared< Or >(orig_cl, orig_clauses[i]);
            new_cl = make_shared< Or >(new_cl, new_clauses[i]);
        }
        return make_tuple(ret, orig_cl, new_cl);
    }
}

RegisteredProver simpld_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps /\\ ch ) )"}, "|- ( ph -> ps )");
RegisteredProver simprd_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps /\\ ch ) )"}, "|- ( ph -> ch )");
RegisteredProver mp1_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( ph -> ( ps -> ch ) )"}, "|- ( ph -> ch )");
RegisteredProver mp2_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ch )");
RegisteredProver tr_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( ch <-> th ) )"}, "|- ( ph -> ( ps <-> th ) )");
RegisteredProver a1i_rp = LibraryToolbox::register_prover({"|- ps"}, "|- ( ph -> ps )");
RegisteredProver biidd_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( ps <-> ps ) )");
RegisteredProver bibi12d_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps <-> th ) <-> ( ch <-> ta ) ) )");
RegisteredProver imbi12d_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps -> th ) <-> ( ch -> ta ) ) )");
RegisteredProver anbi12d_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps /\\ th ) <-> ( ch /\\ ta ) ) )");
RegisteredProver orbi12d_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps \\/ th ) <-> ( ch \\/ ta ) ) )");
RegisteredProver urt_rp = LibraryToolbox::register_prover({"|- -. ph"}, "|- ( ph -> F. )");
RegisteredProver urf_rp = LibraryToolbox::register_prover({"|- ph"}, "|- ( -. ph -> F. )");
RegisteredProver syl3_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( ps -> ch )", "|- ( ch -> th )"}, "|- ( ph -> th )");
//RegisteredProver url_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps \\/ ch ) )", "|- ( ph -> -. ch )"}, "|- ( ph -> ps )");
//RegisteredProver urf_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( ph -> -. ps )"}, "|- ( ph -> F. )");

Prover iterate_expr(const Z3Adapter &adapter, expr e, int depth = 0, expr *parent = NULL) {
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
            case Z3_OP_PR_ASSERTED: {
                assert(num_args == 1);
                assert(arity == 1);
                //cout << "EXPR: " << e.arg(0) << endl;
                /*cout << "HEAD WFF: " << adapter.abs->to_string() << endl;
                cout << "WFF: " << parse_expr(e.arg(0))->to_string() << endl;*/
                pwff w = parse_expr(e.arg(0));
                assert(find_if(adapter.hyps.begin(), adapter.hyps.end(), [=](const pwff &p) { return *p == *w; }) != adapter.hyps.end());
                //auto it = find_if(adapter.hyps.begin(), adapter.hyps.end(), [=](const pwff &p) { return *p == *w; });
                //size_t pos = it - adapter.hyps.begin();
                assert(adapter.hyps.size() >= 1);
                Prover ret = adapter.tb.build_registered_prover(id_rp, {{"ph", adapter.abs->get_type_prover(adapter.tb)}}, {});
                pwff cur_hyp = adapter.abs;
                for (size_t step = 0; step < adapter.hyps.size()-1; step++) {
                    auto and_hyp = dynamic_pointer_cast< And >(cur_hyp);
                    assert(and_hyp != NULL);
                    pwff left = and_hyp->get_a();
                    pwff right = and_hyp->get_b();
                    if (*and_hyp->get_b() == *w) {
                        ret = adapter.tb.build_registered_prover(simprd_rp, {{"ph", adapter.abs->get_type_prover(adapter.tb)}, {"ps", left->get_type_prover(adapter.tb)}, {"ch", right->get_type_prover(adapter.tb)}}, {ret});
                        break;
                    }
                    cur_hyp = and_hyp->get_a();
                    ret = adapter.tb.build_registered_prover(simpld_rp, {{"ph", adapter.abs->get_type_prover(adapter.tb)}, {"ps", left->get_type_prover(adapter.tb)}, {"ch", right->get_type_prover(adapter.tb)}}, {ret});
                }
                assert(*cur_hyp == *w);
                return ret;
                break; }
            case Z3_OP_PR_MODUS_PONENS: {
                assert(num_args == 3);
                assert(arity == 3);
                //cout << endl << "EXPR: " << e.arg(2);
                /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                cout << "HP2: " << parse_expr(extract_thesis(e.arg(1)))->to_string() << endl;
                cout << "TH: " << parse_expr(e.arg(2))->to_string() << endl;*/
                Prover p1 = iterate_expr(adapter, e.arg(0), depth+1, &e);
                Prover p2 = iterate_expr(adapter, e.arg(1), depth+1, &e);
                switch (extract_thesis(e.arg(1)).decl().decl_kind()) {
                case Z3_OP_EQ:
                case Z3_OP_IFF:
                    return adapter.tb.build_registered_prover(mp2_rp, {{"ph", adapter.abs->get_type_prover(adapter.tb)}, {"ps", parse_expr(extract_thesis(e.arg(0)))->get_type_prover(adapter.tb)}, {"ch", parse_expr(e.arg(2))->get_type_prover(adapter.tb)}}, {p1, p2});
                    break;
                case Z3_OP_IMPLIES:
                    return adapter.tb.build_registered_prover(mp1_rp, {{"ph", adapter.abs->get_type_prover(adapter.tb)}, {"ps", parse_expr(extract_thesis(e.arg(0)))->get_type_prover(adapter.tb)}, {"ch", parse_expr(e.arg(2))->get_type_prover(adapter.tb)}}, {p1, p2});
                    break;
                default:
                    throw "Should not arrive here";
                    break;
                }
                break; }
            case Z3_OP_PR_REWRITE: {
                assert(num_args == 1);
                assert(arity == 1);
                //cout << endl << "EXPR: " << e.arg(0);
                //cout << endl << "WFF: " << parse_expr(e.arg(0))->to_string();
                //prove_and_print(parse_expr(e.arg(0)), tb);
                // The thesis should be true independently of ph
                pwff thesis = parse_expr(e.arg(0));
                //cout << "ORACLE for '" << thesis->to_string() << "'!" << endl;
                //Prover p1 = thesis->get_adv_truth_prover(tb);
                Prover p1 = adapter.tb.build_prover({}, thesis->to_asserted_sentence(adapter.tb.get_library()), {}, {});
                return adapter.tb.build_registered_prover(a1i_rp, {{"ph", adapter.abs->get_type_prover(adapter.tb)}, {"ps", thesis->get_type_prover(adapter.tb)}}, {p1});
                break; }
            case Z3_OP_PR_TRANSITIVITY: {
                assert(num_args == 3);
                assert(arity == 3);
                //cout << endl << "EXPR: " << e.arg(2);
                /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                cout << "HP2: " << parse_expr(extract_thesis(e.arg(1)))->to_string() << endl;
                cout << "TH: " << parse_expr(e.arg(2))->to_string() << endl;*/
                Prover p1 = iterate_expr(adapter, e.arg(0), depth+1, &e);
                Prover p2 = iterate_expr(adapter, e.arg(1), depth+1, &e);
                shared_ptr< Biimp > w1 = dynamic_pointer_cast< Biimp >(parse_expr(extract_thesis(e.arg(0))));
                shared_ptr< Biimp > w2 = dynamic_pointer_cast< Biimp >(parse_expr(extract_thesis(e.arg(1))));
                assert(w1 != NULL && w2 != NULL);
                assert(*w1->get_b() == *w2->get_a());
                pwff ps = w1->get_a();
                pwff ch = w1->get_b();
                pwff th = w2->get_b();
                return adapter.tb.build_registered_prover(tr_rp, {{"ph", adapter.abs->get_type_prover(adapter.tb)}, {"ps", ps->get_type_prover(adapter.tb)}, {"ch", ch->get_type_prover(adapter.tb)}, {"th", th->get_type_prover(adapter.tb)}}, {p1, p2});
                break; }
            case Z3_OP_PR_MONOTONICITY: {
                assert(num_args == 2);
                assert(arity == 2);
                /*cout << "EXPR: " << e.arg(num_args-1) << endl;
                cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                cout << "TH: " << parse_expr(e.arg(1))->to_string() << endl;*/
                // Recognize the monotonic operation
                expr thesis = e.arg(num_args-1);
                switch (thesis.decl().decl_kind()) {
                /*case Z3_OP_EQ:
                    break;*/
                case Z3_OP_IFF: {
                    assert(thesis.num_args() == 2);
                    expr th_left = thesis.arg(0);
                    expr th_right = thesis.arg(1);
                    assert(th_left.decl().decl_kind() == th_right.decl().decl_kind());
                    assert(th_left.num_args() == th_right.num_args());
                    switch (th_left.decl().decl_kind()) {
                    case Z3_OP_AND:
                    case Z3_OP_OR:
                    case Z3_OP_IFF:
                    case Z3_OP_IMPLIES: {
                        RegisteredProver rp;
                        switch (thesis.decl().decl_kind()) {
                        case Z3_OP_AND: rp = anbi12d_rp; break;
                        case Z3_OP_OR: rp = orbi12d_rp; break;
                        case Z3_OP_IFF: rp = bibi12d_rp; break;
                        case Z3_OP_IMPLIES: rp = imbi12d_rp; break;
                        default: throw "Should not arrive here"; break;
                        }
                        assert(th_left.num_args() >= 2);
                        vector< Prover > hyps_prover;
                        vector< Prover > wffs_prover;
                        size_t used = 0;
                        for (size_t i = 0; i < th_left.num_args(); i++) {
                            if (eq(th_left.arg(i), th_right.arg(i))) {
                                hyps_prover.push_back(adapter.tb.build_registered_prover(biidd_rp, {{"ph", adapter.abs->get_type_prover(adapter.tb)}, {"ps", parse_expr(th_left.arg(i))->get_type_prover(adapter.tb)}}, {}));
                            } else {
                                assert(!used);
                                hyps_prover.push_back(iterate_expr(adapter, e.arg(0), depth+1, &e));
                                used++;
                            }
                            wffs_prover.push_back(parse_expr(th_left.arg(i))->get_type_prover(adapter.tb));
                            wffs_prover.push_back(parse_expr(th_right.arg(i))->get_type_prover(adapter.tb));
                        }
                        assert(hyps_prover.size() == th_left.num_args());
                        assert(used == num_args - 1);
                        if (th_left.num_args() > 2) {
                            cout << "ORACLE for '" << make_shared< Imp >(adapter.abs, parse_expr(extract_thesis(e)))->to_string() << "'!" << endl;
                            return make_shared< Imp >(adapter.abs, parse_expr(extract_thesis(e)))->get_adv_truth_prover(adapter.tb);
                        } else {
                            return adapter.tb.build_registered_prover(rp, {{"ph", adapter.abs->get_type_prover(adapter.tb)}, {"ps", wffs_prover[0]}, {"ch", wffs_prover[1]}, {"th", wffs_prover[2]}, {"ta", wffs_prover[3]}}, hyps_prover);
                        }
                        break; }
                    default:
                        throw "Unsupported operation";
                        break;
                    }
                    break; }
                default:
                    throw "Unsupported operation";
                    break;
                }
                break; }
            case Z3_OP_PR_UNIT_RESOLUTION: {
                cout << "Unit resolution: " << num_args << " with arity " << decl.arity() << endl;
                for (unsigned i = 0; i < num_args-1; i++) {
                    cout << "HP" << i << ": " << extract_thesis(e.arg(i)) << endl;
                    cout << "HP" << i << ": " << parse_expr(extract_thesis(e.arg(i)))->to_string() << endl;
                }
                cout << "TH: " << e.arg(num_args-1) << endl;
                cout << "TH: " << parse_expr(e.arg(num_args-1))->to_string() << endl;

                size_t elims_num = num_args - 2;
                expr or_expr = extract_thesis(e.arg(0));
                assert(or_expr.decl().decl_kind() == Z3_OP_OR);
                size_t clauses_num = or_expr.num_args();
                assert(clauses_num >= 2);
                assert(elims_num <= clauses_num);

                Prover orig_prover = iterate_expr(adapter, e.arg(0), depth+1, &e);

                vector< pwff > elims;
                vector< Prover > elim_provers;
                for (size_t i = 0; i < elims_num; i++) {
                    elims.push_back(parse_expr(extract_thesis(e.arg(i+1))));
                    elim_provers.push_back(iterate_expr(adapter, e.arg(i+1), depth+1, &e));
                }

                vector< pwff > orig_clauses;
                vector< pwff > new_clauses;
                vector< Prover > provers;
                for (size_t i = 0; i < or_expr.num_args(); i++) {
                    pwff clause = parse_expr(or_expr.arg(i));
                    orig_clauses.push_back(clause);

                    // Search an eliminator for the positive form
                    auto elim_it = find_if(elims.begin(), elims.end(), [=](const pwff &w){ return *w == *make_shared< Not >(clause); });
                    if (elim_it != elims.end()) {
                        size_t pos = elim_it - elims.begin();
                        provers.push_back(adapter.tb.build_registered_prover(urt_rp, {{"ph", clause->get_type_prover(adapter.tb)}}, {elim_provers[pos]}));
                        new_clauses.push_back(make_shared< False >());
                        continue;
                    }

                    // Search an eliminator for the negative form
                    auto clause_not = dynamic_pointer_cast< Not >(clause);
                    if (clause_not != NULL) {
                        auto elim_it = find_if(elims.begin(), elims.end(), [=](const pwff &w){ return *w == *clause_not->get_a(); });
                        if (elim_it != elims.end()) {
                            size_t pos = elim_it - elims.begin();
                            provers.push_back(adapter.tb.build_registered_prover(urf_rp, {{"ph", clause_not->get_a()->get_type_prover(adapter.tb)}}, {elim_provers[pos]}));
                            new_clauses.push_back(make_shared< False >());
                            continue;
                        }
                    }

                    // No eliminator found, keeping the clause and pushing a trivial proved
                    provers.push_back(adapter.tb.build_registered_prover(id_rp, {{"ph", clause->get_type_prover(adapter.tb)}}, {}));
                    new_clauses.push_back(clause);
                }

                Prover joined_or_prover;
                Prover simplifcation_prover;
                pwff orig_clause;
                pwff new_clause;
                pwff new_clause2;
                pwff simplified_clause;
                tie(joined_or_prover, orig_clause, new_clause) = join_or_imp(orig_clauses, new_clauses, provers, adapter.tb);
                tie(simplifcation_prover, new_clause2, simplified_clause) = simplify_or(new_clauses, adapter.tb);
                cout << orig_clause->to_string() << endl;
                cout << new_clause->to_string() << endl;
                cout << new_clause2->to_string() << endl;
                cout << simplified_clause->to_string() << endl;
                assert(*new_clause == *new_clause2);
                return adapter.tb.build_registered_prover(syl3_rp, {{"ph", adapter.abs->get_type_prover(adapter.tb)}, {"ps", orig_clause->get_type_prover(adapter.tb)}, {"ch", orig_clause->get_type_prover(adapter.tb)}, {"th", simplified_clause->get_type_prover(adapter.tb)}}, {orig_prover, joined_or_prover, simplifcation_prover});
                break; }
            /*case Z3_OP_PR_DEF_AXIOM:
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
                //prove_and_print(make_shared< Imp >(w, parse_expr(extract_thesis(e))), tb);
                cout << "ORACLE for '" << make_shared< Imp >(adapter.abs, parse_expr(extract_thesis(e)))->to_string() << "'!" << endl;
                return make_shared< Imp >(adapter.abs, parse_expr(extract_thesis(e)))->get_adv_truth_prover(adapter.tb);
                break;
            }
        } else {
            cout << "unknown kind " << kind << endl;
            throw "Unknown kind";
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

RegisteredProver refute_rp = LibraryToolbox::register_prover({"|- ( -. ph -> F. )"}, "|- ph");
RegisteredProver efald_rp = LibraryToolbox::register_prover({"|- ( ( ph /\\ -. ps ) -> F. )"}, "|- ( ph -> ps )");

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

    for (int i = 0; i < 2; i++) {
        set_param("proof", true);
        context c;

        solver s(c);
        auto adapter = Z3Adapter(s, tb);

        if (i == 0) {
            expr x = c.bool_const("ph");
            expr y = c.bool_const("ps");
            adapter.add_formula(((!(x && y)) == (!x || !y)), false);
        }

        if (i == 1) {
            expr ph = c.bool_const("ph");
            expr ps = c.bool_const("ps");
            expr ch = c.bool_const("ch");
            adapter.add_formula(implies(ph, implies(ps, ch)), true);
            adapter.add_formula(implies(!ph, implies(!ps, ch)), true);
            adapter.add_formula(implies(ph == ps, ch), false);
        }

        cout << "ABSURDUM HYPOTHESIS: " << adapter.abs->to_string() << endl;
        cout << "TARGET: " << adapter.target->to_string() << endl;
        prove_and_print(adapter.target, tb);

        switch (adapter.s.check()) {
        case unsat:   cout << "valid\n"; break;
        case sat:     cout << "not valid\n"; break;
        case unknown: cout << "unknown\n"; break;
        }

        ProofEngine engine(lib, true);
        Prover main_prover = iterate_expr(adapter, adapter.s.proof());
        bool res;
        if (adapter.hyps.size() == 1) {
            res = tb.build_registered_prover(refute_rp, {{"ph", adapter.target->get_type_prover(tb)}}, {main_prover})(engine);
        } else {
            res = tb.build_registered_prover(efald_rp, {{"ph", adapter.and_hyps->get_type_prover(tb)}, {"ps", adapter.thesis->get_type_prover(tb)}}, {main_prover})(engine);
        }
        if (res) {
            cout << endl << "FINAL PROOF FOUND!" << endl;
            //cout << "proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
            cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
            cout << "proof length: " << engine.get_proof_labels().size() << endl;
        } else {
            cout << "proof generation failed..." << endl;
        }
        cout << endl;
    }

    return 0;
}
