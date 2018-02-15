
#include <unordered_map>

#include <z3++.h>

#include "wff.h"
#include "mm/reader.h"
#include "utils/utils.h"
#include "platform.h"
#include "test/test_env.h"

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
        return True::create();
    case Z3_OP_FALSE:
        assert(e.num_args() == 0);
        return False::create();
    case Z3_OP_EQ:
        // Interpreted as biimplication; why does Z3 generates equalities between formulae?
        assert(e.num_args() == 2);
        return Biimp::create(parse_expr(e.arg(0)), parse_expr(e.arg(1)));
    case Z3_OP_AND:
        assert(e.num_args() >= 2);
        ret = And::create(parse_expr(e.arg(0)), parse_expr(e.arg(1)));
        for (unsigned i = 2; i < e.num_args(); i++) {
            ret = And::create(ret, parse_expr(e.arg(i)));
        }
        return ret;
    case Z3_OP_OR:
        assert(e.num_args() >= 2);
        ret = Or::create(parse_expr(e.arg(0)), parse_expr(e.arg(1)));
        for (unsigned i = 2; i < e.num_args(); i++) {
            ret = Or::create(ret, parse_expr(e.arg(i)));
        }
        return ret;
    case Z3_OP_IFF:
        assert(e.num_args() == 2);
        return Biimp::create(parse_expr(e.arg(0)), parse_expr(e.arg(1)));
    case Z3_OP_XOR:
        assert(e.num_args() == 2);
        return Xor::create(parse_expr(e.arg(0)), parse_expr(e.arg(1)));
    case Z3_OP_NOT:
        assert(e.num_args() == 1);
        return Not::create(parse_expr(e.arg(0)));
    case Z3_OP_IMPLIES:
        assert(e.num_args() == 2);
        return Imp::create(parse_expr(e.arg(0)), parse_expr(e.arg(1)));

    case Z3_OP_UNINTERPRETED:
        assert(e.num_args() == 0);
        return Var::create(decl.name().str());

    default:
        throw "Cannot handle this formula";
    }
}

expr extract_thesis(expr proof) {
    return proof.arg(proof.num_args()-1);
}

void prove_and_print(pwff wff, const LibraryToolbox &tb) {
    ExtendedProofEngine< Sentence > engine(tb);
    wff->get_adv_truth_prover(tb)(engine);
    if (engine.get_proof_labels().size() > 0) {
        //cout << "adv truth proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
        cout << "stack top: " << tb.print_sentence(engine.get_stack().back(), SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << endl;
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
tuple< Prover< ConcreteCheckpointedProofEngine< Sentence > >, pwff, pwff > simplify_or(const vector< pwff > &clauses, const LibraryToolbox &tb) {
    if (clauses.size() == 0) {
        return make_tuple(tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(ff_rp, {}, {}), False::create(), False::create());
    } else {
        Prover< ConcreteCheckpointedProofEngine< Sentence > > ret = tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(id_rp, {{"ph", clauses[0]->get_type_prover(tb)}}, {});
        pwff first = clauses[0];
        pwff second = clauses[0];
        pwff falsum = False::create();
        for (size_t i = 1; i < clauses.size(); i++) {
            pwff new_clause = clauses[i];
            if (*second == *falsum) {
                ret = tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(orfa2_rp, {{"ph", first->get_type_prover(tb)}, {"ps", new_clause->get_type_prover(tb)}}, {ret});
                second = new_clause;
            } else {
                if (*new_clause == *falsum) {
                    ret = tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(orfa_rp, {{"ph", first->get_type_prover(tb)}, {"ps", second->get_type_prover(tb)}}, {ret});
                } else {
                    ret = tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(orim1i_rp, {{"ph", first->get_type_prover(tb)}, {"ps", second->get_type_prover(tb)}, {"ch", new_clause->get_type_prover(tb)}}, {ret});
                    second = Or::create(second, new_clause);
                }
            }
            first = Or::create(first, new_clause);
        }
        return make_tuple(ret, first, second);
    }
}

RegisteredProver orim12d_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps -> ch ) )", "|- ( ph -> ( th -> ta ) )"}, "|- ( ph -> ( ( ps \\/ th ) -> ( ch \\/ ta ) ) )");

tuple< Prover< ConcreteCheckpointedProofEngine< Sentence > >, pwff, pwff > join_or_imp(const vector< pwff > &orig_clauses, const vector< pwff > &new_clauses, vector< Prover< ConcreteCheckpointedProofEngine< Sentence > > > &provers, const pwff &abs, const LibraryToolbox &tb) {
    assert(orig_clauses.size() == new_clauses.size());
    assert(orig_clauses.size() == provers.size());
    if (orig_clauses.size() == 0) {
        return make_tuple(tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(ff_rp, {}, {}), False::create(), False::create());
    } else {
        Prover< ConcreteCheckpointedProofEngine< Sentence > > ret = provers[0];
        pwff orig_cl = orig_clauses[0];
        pwff new_cl = new_clauses[0];
        for (size_t i = 1; i < orig_clauses.size(); i++) {
            ret = tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(orim12d_rp, {{"ph", abs->get_type_prover(tb)}, {"ps", orig_cl->get_type_prover(tb)}, {"ch", new_cl->get_type_prover(tb)}, {"th", orig_clauses[i]->get_type_prover(tb)}, {"ta", new_clauses[i]->get_type_prover(tb)}}, {ret, provers[i]});
            orig_cl = Or::create(orig_cl, orig_clauses[i]);
            new_cl = Or::create(new_cl, new_clauses[i]);
        }
        return make_tuple(ret, orig_cl, new_cl);
    }
}

RegisteredProver simpld_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps /\\ ch ) )"}, "|- ( ph -> ps )");
RegisteredProver simprd_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps /\\ ch ) )"}, "|- ( ph -> ch )");
RegisteredProver mp1_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( ph -> ( ps -> ch ) )"}, "|- ( ph -> ch )");
RegisteredProver mp2_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ch )");
RegisteredProver bitrd_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( ch <-> th ) )"}, "|- ( ph -> ( ps <-> th ) )");
RegisteredProver a1i_rp = LibraryToolbox::register_prover({"|- ps"}, "|- ( ph -> ps )");
RegisteredProver biidd_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( ps <-> ps ) )");
RegisteredProver bibi12d_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps <-> th ) <-> ( ch <-> ta ) ) )");
RegisteredProver imbi12d_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps -> th ) <-> ( ch -> ta ) ) )");
RegisteredProver anbi12d_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps /\\ th ) <-> ( ch /\\ ta ) ) )");
RegisteredProver orbi12d_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps \\/ th ) <-> ( ch \\/ ta ) ) )");
RegisteredProver notbid_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ( -. ps <-> -. ch ) )");
RegisteredProver urt_rp = LibraryToolbox::register_prover({"|- ( ph -> -. ps )"}, "|- ( ph -> ( ps -> F. ) )");
RegisteredProver urf_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )"}, "|- ( ph -> ( -. ps -> F. ) )");
RegisteredProver idd_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( ps -> ps ) )");
RegisteredProver mpd_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( ph -> ( ps -> ch ) )"}, "|- ( ph -> ch )");
RegisteredProver syl_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( ps -> ch )"}, "|- ( ph -> ch )");
RegisteredProver bifald_rp = LibraryToolbox::register_prover({"|- ( ph -> -. ps )"}, "|- ( ph -> ( ps <-> F. ) )");
RegisteredProver orsild_rp = LibraryToolbox::register_prover({"|- ( ph -> -. ( ps \\/ ch ) )"}, "|- ( ph -> -. ps )");
RegisteredProver orsird_rp = LibraryToolbox::register_prover({"|- ( ph -> -. ( ps \\/ ch ) )"}, "|- ( ph -> -. ch )");

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
                this->and_hyps = And::create(this->and_hyps, w);
                this->target = And::create(this->target, w);
                this->abs = And::create(this->abs, w);
            }
        } else {
            this->hyps.push_back(Not::create(w));
            this->thesis = w;
            if (this->target == NULL) {
                this->target = w;
                this->abs = Not::create(w);
            } else {
                this->target = Imp::create(this->target, w);
                this->abs = And::create(this->abs, Not::create(w));
            }
        }
    }

    pwff get_current_abs_hyps() {
        return this->abs;
    }

    Prover< ConcreteCheckpointedProofEngine< Sentence > > convert_proof(expr e, int depth = 0) {
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
                    /*cout << "HEAD WFF: " << this->get_current_abs_hyps()->to_string() << endl;
                    cout << "WFF: " << parse_expr(e.arg(0))->to_string() << endl;*/
                    pwff w = parse_expr(e.arg(0));
                    assert(find_if(this->hyps.begin(), this->hyps.end(), [=](const pwff &p) { return *p == *w; }) != this->hyps.end());
                    //auto it = find_if(this->hyps.begin(), this->hyps.end(), [=](const pwff &p) { return *p == *w; });
                    //size_t pos = it - this->hyps.begin();
                    assert(this->hyps.size() >= 1);
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > ret = this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(id_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}}, {});
                    pwff cur_hyp = this->get_current_abs_hyps();
                    for (size_t step = 0; step < this->hyps.size()-1; step++) {
                        auto and_hyp = dynamic_pointer_cast< const And >(cur_hyp);
                        assert(and_hyp != NULL);
                        pwff left = and_hyp->get_a();
                        pwff right = and_hyp->get_b();
                        if (*and_hyp->get_b() == *w) {
                            cur_hyp = and_hyp->get_b();
                            ret = this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(simprd_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", left->get_type_prover(this->tb)}, {"ch", right->get_type_prover(this->tb)}}, {ret});
                            break;
                        }
                        cur_hyp = and_hyp->get_a();
                        ret = this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(simpld_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", left->get_type_prover(this->tb)}, {"ch", right->get_type_prover(this->tb)}}, {ret});
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
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > p1 = this->convert_proof(e.arg(0), depth+1);
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > p2 = this->convert_proof(e.arg(1), depth+1);
                    switch (extract_thesis(e.arg(1)).decl().decl_kind()) {
                    case Z3_OP_EQ:
                    case Z3_OP_IFF:
                        return this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(mp2_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", parse_expr(extract_thesis(e.arg(0)))->get_type_prover(this->tb)}, {"ch", parse_expr(e.arg(2))->get_type_prover(this->tb)}}, {p1, p2});
                        break;
                    case Z3_OP_IMPLIES:
                        return this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(mp1_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", parse_expr(extract_thesis(e.arg(0)))->get_type_prover(this->tb)}, {"ch", parse_expr(e.arg(2))->get_type_prover(this->tb)}}, {p1, p2});
                        break;
                    default:
                        throw "Should not arrive here";
                        break;
                    }
                    break; }
                case Z3_OP_PR_REWRITE: {
                    assert(num_args == 1);
                    assert(arity == 1);
                    /*cout << "EXPR: " << e.arg(0) << endl;
                    cout << "WFF: " << parse_expr(e.arg(0))->to_string() << endl;*/
                    //prove_and_print(parse_expr(e.arg(0)), tb);
                    // The thesis should be true independently of ph
                    pwff thesis = parse_expr(e.arg(0));
                    //cout << "ORACLE for '" << thesis->to_string() << "'!" << endl;
                    //Prover p1 = thesis->get_adv_truth_prover(tb);
                    auto pt = pt2_to_pt(thesis->to_parsing_tree(this->tb));
                    auto sent = this->tb.reconstruct_sentence(pt, this->tb.get_turnstile());
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > p1 = this->tb.build_prover({}, sent, {}, {});
                    return this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(a1i_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", thesis->get_type_prover(this->tb)}}, {p1});
                    break; }
                case Z3_OP_PR_TRANSITIVITY: {
                    assert(num_args == 3);
                    assert(arity == 3);
                    //cout << endl << "EXPR: " << e.arg(2);
                    /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                    cout << "HP2: " << parse_expr(extract_thesis(e.arg(1)))->to_string() << endl;
                    cout << "TH: " << parse_expr(e.arg(2))->to_string() << endl;*/
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > p1 = this->convert_proof(e.arg(0), depth+1);
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > p2 = this->convert_proof(e.arg(1), depth+1);
                    auto w1 = dynamic_pointer_cast< const Biimp >(parse_expr(extract_thesis(e.arg(0))));
                    auto w2 = dynamic_pointer_cast< const Biimp >(parse_expr(extract_thesis(e.arg(1))));
                    assert(w1 != NULL && w2 != NULL);
                    assert(*w1->get_b() == *w2->get_a());
                    pwff ps = w1->get_a();
                    pwff ch = w1->get_b();
                    pwff th = w2->get_b();
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > ret = this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(bitrd_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", ps->get_type_prover(this->tb)}, {"ch", ch->get_type_prover(this->tb)}, {"th", th->get_type_prover(this->tb)}}, {p1, p2});
                    //cerr << "TEST: " << test_prover(ret, this->tb) << endl;
                    return ret;
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
                            std::function< pwff(pwff, pwff) > combiner = [](pwff, pwff)->pwff { throw "Should not arrive here"; };
                            switch (th_left.decl().decl_kind()) {
                            case Z3_OP_AND: rp = anbi12d_rp; combiner = [](pwff a, pwff b) { return And::create(a, b); }; break;
                            case Z3_OP_OR: rp = orbi12d_rp; combiner = [](pwff a, pwff b) { return Or::create(a, b); }; break;
                            case Z3_OP_IFF: rp = bibi12d_rp; break;
                            case Z3_OP_IMPLIES: rp = imbi12d_rp; break;
                            default: throw "Should not arrive here"; break;
                            }
                            assert(th_left.num_args() >= 2);
                            vector< Prover< ConcreteCheckpointedProofEngine< Sentence > > > hyp_provers;
                            vector< pwff > left_wffs;
                            vector< pwff > right_wffs;
                            //vector< Prover > wffs_prover;
                            size_t used = 0;
                            for (size_t i = 0; i < th_left.num_args(); i++) {
                                if (eq(th_left.arg(i), th_right.arg(i))) {
                                    hyp_provers.push_back(this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(biidd_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", parse_expr(th_left.arg(i))->get_type_prover(this->tb)}}, {}));
                                } else {
                                    assert(!used);
                                    hyp_provers.push_back(this->convert_proof(e.arg(0), depth+1));
                                    used++;
                                }
                                left_wffs.push_back(parse_expr(th_left.arg(i)));
                                right_wffs.push_back(parse_expr(th_right.arg(i)));
                            }
                            assert(hyp_provers.size() == th_left.num_args());
                            assert(used == num_args - 1);
                            Prover< ConcreteCheckpointedProofEngine< Sentence > > ret = hyp_provers[0];
                            pwff left_wff = left_wffs[0];
                            pwff right_wff = right_wffs[0];
                            for (size_t i = 1; i < th_left.num_args(); i++) {
                                ret = this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)},
                                                                              {"ps", left_wff->get_type_prover(this->tb)}, {"ch", right_wff->get_type_prover(this->tb)},
                                                                              {"th", left_wffs[i]->get_type_prover(this->tb)}, {"ta", right_wffs[i]->get_type_prover(this->tb)}}, {ret, hyp_provers[i]});
                                if (th_left.decl().decl_kind() == Z3_OP_AND || th_left.decl().decl_kind() == Z3_OP_OR) {
                                    left_wff = combiner(left_wff, left_wffs[i]);
                                    right_wff = combiner(right_wff, right_wffs[i]);
                                }
                            }
                            return ret;
                            break; }
                        case Z3_OP_NOT: {
                            pwff left_wff = parse_expr(th_left.arg(0));
                            pwff right_wff = parse_expr(th_right.arg(0));
                            Prover< ConcreteCheckpointedProofEngine< Sentence > > ret = this->convert_proof(e.arg(0), depth+1);
                            return this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(notbid_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)},
                                                                                  {"ps", left_wff->get_type_prover(this->tb)}, {"ch", right_wff->get_type_prover(this->tb)}}, {ret});
                            break; }
                        default:
                            cerr << "Unsupported operation " << th_left.decl().decl_kind() << endl;
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
                    /*cout << "Unit resolution: " << num_args << " with arity " << decl.arity() << endl;
                    for (unsigned i = 0; i < num_args-1; i++) {
                        cout << "HP" << i << ": " << extract_thesis(e.arg(i)) << endl;
                        cout << "HP" << i << ": " << parse_expr(extract_thesis(e.arg(i)))->to_string() << endl;
                    }
                    cout << "TH: " << e.arg(num_args-1) << endl;
                    cout << "TH: " << parse_expr(e.arg(num_args-1))->to_string() << endl;*/

                    size_t elims_num = num_args - 2;
                    expr or_expr = extract_thesis(e.arg(0));
                    assert(or_expr.decl().decl_kind() == Z3_OP_OR);
                    size_t clauses_num = or_expr.num_args();
                    assert(clauses_num >= 2);
                    assert(elims_num <= clauses_num);

                    Prover< ConcreteCheckpointedProofEngine< Sentence > > orig_prover = this->convert_proof(e.arg(0), depth+1);
                    //cerr << "TEST: " << test_prover(orig_prover, this->tb) << endl;

                    vector< pwff > elims;
                    vector< Prover< ConcreteCheckpointedProofEngine< Sentence > > > elim_provers;
                    for (size_t i = 0; i < elims_num; i++) {
                        elims.push_back(parse_expr(extract_thesis(e.arg(i+1))));
                        elim_provers.push_back(this->convert_proof(e.arg(i+1), depth+1));
                        //cerr << "TEST: " << test_prover(elim_provers.back(), this->tb) << endl;
                    }

                    vector< pwff > orig_clauses;
                    vector< pwff > new_clauses;
                    vector< Prover< ConcreteCheckpointedProofEngine< Sentence > > > provers;
                    for (size_t i = 0; i < or_expr.num_args(); i++) {
                        pwff clause = parse_expr(or_expr.arg(i));
                        orig_clauses.push_back(clause);

                        // Search an eliminator for the positive form
                        auto elim_it = find_if(elims.begin(), elims.end(), [=](const pwff &w){ return *w == *Not::create(clause); });
                        if (elim_it != elims.end()) {
                            size_t pos = elim_it - elims.begin();
                            provers.push_back(this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(urt_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", clause->get_type_prover(this->tb)}}, {elim_provers[pos]}));
                            new_clauses.push_back(False::create());
                            //cerr << "TEST 1: " << test_prover(provers.back(), this->tb) << endl;
                            continue;
                        }

                        // Search an eliminator for the negative form
                        auto clause_not = dynamic_pointer_cast< const Not >(clause);
                        if (clause_not != NULL) {
                            auto elim_it = find_if(elims.begin(), elims.end(), [=](const pwff &w){ return *w == *clause_not->get_a(); });
                            if (elim_it != elims.end()) {
                                size_t pos = elim_it - elims.begin();
                                provers.push_back(this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(urf_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", clause_not->get_a()->get_type_prover(this->tb)}}, {elim_provers[pos]}));
                                new_clauses.push_back(False::create());
                                //cerr << "TEST 1: " << test_prover(provers.back(), this->tb) << endl;
                                continue;
                            }
                        }

                        // No eliminator found, keeping the clause and pushing a trivial proved
                        provers.push_back(this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(idd_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", clause->get_type_prover(this->tb)}}, {}));
                        new_clauses.push_back(clause);
                        //cerr << "TEST 1: " << test_prover(provers.back(), this->tb) << endl;
                    }

                    Prover< ConcreteCheckpointedProofEngine< Sentence > > joined_or_prover;
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > simplifcation_prover;
                    pwff orig_clause;
                    pwff new_clause;
                    pwff new_clause2;
                    pwff simplified_clause;
                    tie(joined_or_prover, orig_clause, new_clause) = join_or_imp(orig_clauses, new_clauses, provers, this->get_current_abs_hyps(), this->tb);
                    tie(simplifcation_prover, new_clause2, simplified_clause) = simplify_or(new_clauses, this->tb);
                    /*cout << orig_clause->to_string() << endl;
                    cout << new_clause->to_string() << endl;
                    cout << new_clause2->to_string() << endl;
                    cout << simplified_clause->to_string() << endl;*/
                    assert(*new_clause == *new_clause2);
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > ret = this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(mpd_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", orig_clause->get_type_prover(this->tb)}, {"ch", new_clause->get_type_prover(this->tb)}}, {orig_prover, joined_or_prover});
                    ret = this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(syl_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", new_clause->get_type_prover(this->tb)}, {"ch", simplified_clause->get_type_prover(this->tb)}}, {ret, simplifcation_prover});
                    //cerr << "TEST: " << test_prover(ret, this->tb) << endl;
                    //assert(false);
                    return ret;
                    break; }
                case Z3_OP_PR_DEF_AXIOM: {
                    assert(num_args == 1);
                    assert(arity == 1);
                    cout << "EXPR: " << e.arg(0) << endl;
                    cout << "WFF: " << parse_expr(e.arg(0))->to_string() << endl;
                    pwff thesis = parse_expr(extract_thesis(e));
                    cout << "AXIOM ORACLE for '" << thesis->to_string() << "'!" << endl;
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > p1 = thesis->get_adv_truth_prover(this->tb);
                    return this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(a1i_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", thesis->get_type_prover(this->tb)}}, {p1});
                    break; }
                case Z3_OP_PR_NOT_OR_ELIM: {
                    assert(num_args == 2);
                    assert(arity == 2);
                    //cout << endl << "EXPR: " << e.arg(2);
                    /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;;
                    cout << "TH: " << parse_expr(e.arg(1))->to_string() << endl;*/
                    expr not_expr = extract_thesis(e.arg(0));
                    assert(not_expr.decl().decl_kind() == Z3_OP_NOT);
                    assert(not_expr.num_args() == 1);
                    expr or_expr = not_expr.arg(0);
                    assert(or_expr.decl().decl_kind() == Z3_OP_OR);
                    expr not_target_expr = e.arg(1);
                    assert(not_target_expr.decl().decl_kind() == Z3_OP_NOT);
                    assert(not_target_expr.num_args() == 1);
                    pwff target_wff = parse_expr(not_target_expr.arg(0));
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > ret = this->convert_proof(e.arg(0), depth+1);
                    pwff wff = parse_expr(or_expr);
                    for (size_t i = 0; i < or_expr.num_args()-1; i++) {
                        auto wff_or = dynamic_pointer_cast< const Or >(wff);
                        assert(wff != NULL);
                        if (*wff_or->get_b() == *target_wff) {
                            return this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(orsird_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", wff_or->get_a()->get_type_prover(this->tb)}, {"ch", wff_or->get_b()->get_type_prover(this->tb)}}, {ret});
                        } else {
                            ret = this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(orsild_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", wff_or->get_a()->get_type_prover(this->tb)}, {"ch", wff_or->get_b()->get_type_prover(this->tb)}}, {ret});
                            wff = wff_or->get_a();
                        }
                    }
                    assert(*wff == *target_wff);
                    return ret;
                    break; }
                case Z3_OP_PR_IFF_FALSE: {
                    assert(num_args == 2);
                    assert(arity == 2);
                    //cout << endl << "EXPR: " << e.arg(2);
                    /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                    cout << "TH: " << parse_expr(e.arg(1))->to_string() << endl;*/
                    pwff hyp = parse_expr(extract_thesis(e.arg(0)));
                    Prover< ConcreteCheckpointedProofEngine< Sentence > > hyp_prover = this->convert_proof(e.arg(0), depth+1);
                    auto hyp_not = dynamic_pointer_cast< const Not >(hyp);
                    assert(hyp_not != NULL);
                    return this->tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(bifald_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", hyp_not->get_a()->get_type_prover(this->tb)}}, {hyp_prover});
                    break; }
                case Z3_OP_PR_LEMMA: {
                    assert(num_args == 2);
                    assert(arity == 2);
                    //cout << "FULL: " << e << endl;
                    /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                    cout << "TH: " << e.arg(1) << endl;
                    cout << "TH: " << parse_expr(e.arg(1))->to_string() << endl;*/
                    cout << "LEMMA ORACLE for '" << Imp::create(this->get_current_abs_hyps(), parse_expr(extract_thesis(e)))->to_string() << "'!" << endl;
                    return Imp::create(this->get_current_abs_hyps(), parse_expr(extract_thesis(e)))->get_adv_truth_prover(this->tb);
                    break; }
                /*case Z3_OP_PR_HYPOTHESIS:
                    cout << "hypothesis";
                    assert(num_args == 1);
                    assert(arity == 1);
                    cout << endl << "PARENT: " << *parent;
                    //cout << endl << "EXPR: " << e.arg(0);
                    cout << endl << "WFF: " << parse_expr(e.arg(0))->to_string();
                    break;*/
                default:
                    //prove_and_print(Imp::create(w, parse_expr(extract_thesis(e))), tb);
                    cout << "GENERIC ORACLE for '" << Imp::create(this->get_current_abs_hyps(), parse_expr(extract_thesis(e)))->to_string() << "'!" << endl;
                    return Imp::create(this->get_current_abs_hyps(), parse_expr(extract_thesis(e)))->get_adv_truth_prover(this->tb);
                    break;
                }
            } else {
                cout << "unknown kind " << kind << endl;
                throw "Unknown kind";
            }

            /*for (unsigned i = 0; i < num_args; i++) {
                iterate_expr(e.arg(i), tb, depth+1);
            }*/
        } else {
            throw "Expression is not an application";
        }

        /*sort s = e.get_sort();
        cout << string(depth, ' ') << "sort: " << s << " (" << s.sort_kind() << "), kind: " << e.kind() << ", num_args: " << e.num_args() << endl;*/
    }
};

RegisteredProver refute_rp = LibraryToolbox::register_prover({"|- ( -. ph -> F. )"}, "|- ph");
RegisteredProver efald_rp = LibraryToolbox::register_prover({"|- ( ( ph /\\ -. ps ) -> F. )"}, "|- ( ph -> ps )");

int test_z3_main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    for (int i = 0; i < 3; i++) {
        set_param("proof", true);
        context c;

        solver s(c);
        auto adapter = Z3Adapter(s, tb);

        if (i == 0) {
            expr ph = c.bool_const("ph");
            expr ps = c.bool_const("ps");
            adapter.add_formula(((!(ph && ps)) == (!ph || !ps)), false);
        }

        if (i == 1) {
            expr ph = c.bool_const("ph");
            expr ps = c.bool_const("ps");
            adapter.add_formula((ph || ps ) == (ps || ph), false);
        }

        if (i == 2) {
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

        ExtendedProofEngine< Sentence > engine(tb, true);
        Prover< ConcreteCheckpointedProofEngine< Sentence > > main_prover = adapter.convert_proof(adapter.s.proof());
        bool res;
        if (adapter.hyps.size() == 1) {
            res = tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(refute_rp, {{"ph", adapter.target->get_type_prover(tb)}}, {main_prover})(engine);
        } else {
            res = tb.build_registered_prover< ConcreteCheckpointedProofEngine< Sentence > >(efald_rp, {{"ph", adapter.and_hyps->get_type_prover(tb)}, {"ps", adapter.thesis->get_type_prover(tb)}}, {main_prover})(engine);
        }
        if (res) {
            cout << endl << "FINAL PROOF FOUND!" << endl;
            //cout << "proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
            cout << "stack top: " << tb.print_sentence(engine.get_stack().back(), SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << endl;
            cout << "proof length: " << engine.get_proof_labels().size() << endl;
        } else {
            cout << "proof generation failed..." << endl;
        }
        cout << endl;
    }

    return 0;
}
static_block {
    register_main_function("test_z3", test_z3_main);
}
