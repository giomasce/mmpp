
#include <unordered_map>

#include <z3++.h>

#include "wff.h"
#include "mm/reader.h"
#include "utils/utils.h"
#include "platform.h"
#include "mm/setmm.h"
#include "test/test.h"

//#define VERBOSE_Z3

pwff parse_expr(z3::expr e, const LibraryToolbox &tb) {
    assert(e.is_app());
    z3::func_decl decl = e.decl();
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
        return Biimp::create(parse_expr(e.arg(0), tb), parse_expr(e.arg(1), tb));
    case Z3_OP_AND:
        assert(e.num_args() >= 2);
        ret = And::create(parse_expr(e.arg(0), tb), parse_expr(e.arg(1), tb));
        for (unsigned i = 2; i < e.num_args(); i++) {
            ret = And::create(ret, parse_expr(e.arg(i), tb));
        }
        return ret;
    case Z3_OP_OR:
        assert(e.num_args() >= 2);
        ret = Or::create(parse_expr(e.arg(0), tb), parse_expr(e.arg(1), tb));
        for (unsigned i = 2; i < e.num_args(); i++) {
            ret = Or::create(ret, parse_expr(e.arg(i), tb));
        }
        return ret;
    case Z3_OP_IFF:
        assert(e.num_args() == 2);
        return Biimp::create(parse_expr(e.arg(0), tb), parse_expr(e.arg(1), tb));
    case Z3_OP_XOR:
        assert(e.num_args() == 2);
        return Xor::create(parse_expr(e.arg(0), tb), parse_expr(e.arg(1), tb));
    case Z3_OP_NOT:
        assert(e.num_args() == 1);
        return Not::create(parse_expr(e.arg(0), tb));
    case Z3_OP_IMPLIES:
        assert(e.num_args() == 2);
        return Imp::create(parse_expr(e.arg(0), tb), parse_expr(e.arg(1), tb));

    case Z3_OP_UNINTERPRETED:
        assert(e.num_args() == 0);
        return Var::create(decl.name().str(), tb);

    default:
        throw "Cannot handle this formula";
    }
}

z3::expr extract_thesis(z3::expr proof) {
    return proof.arg(proof.num_args()-1);
}

void prove_and_print(pwff wff, const LibraryToolbox &tb) {
    CreativeProofEngineImpl< Sentence > engine(tb);
    wff->get_adv_truth_prover(tb).second(engine);
    if (engine.get_proof_labels().size() > 0) {
        //cout << "adv truth proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
        std::cout << "stack top: " << tb.print_sentence(engine.get_stack().back(), SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << std::endl;
        std::cout << "proof length: " << engine.get_proof_labels().size() << std::endl;
    }
}

RegisteredProver id_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ph )");
RegisteredProver ff_rp = LibraryToolbox::register_prover({}, "|- ( F. -> F. )");
RegisteredProver orim1i_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )"}, "|- ( ( ph \\/ ch ) -> ( ps \\/ ch ) )");
RegisteredProver orfa_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )"}, "|- ( ( ph \\/ F. ) -> ps )");
RegisteredProver orfa2_rp = LibraryToolbox::register_prover({"|- ( ph -> F. )"}, "|- ( ( ph \\/ ps ) -> ps )");

// Produces a prover for ( ( ... ( ph_1 \/ ph_2 ) ... \/ ph_n ) -> ( ... ( ph_{i_1} \/ ph_{i_2} ) ... \/ ph_{i_k} ) ),
// where all instances of F. have been removed (unless they're all F.'s).
std::tuple< Prover< CreativeCheckpointedProofEngine< Sentence > >, pwff, pwff > simplify_or(const std::vector< pwff > &clauses, const LibraryToolbox &tb) {
    if (clauses.size() == 0) {
        return make_tuple(tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(ff_rp, {}, {}), False::create(), False::create());
    } else {
        Prover< CreativeCheckpointedProofEngine< Sentence > > ret = tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(id_rp, {{"ph", clauses[0]->get_type_prover(tb)}}, {});
        pwff first = clauses[0];
        pwff second = clauses[0];
        pwff falsum = False::create();
        for (size_t i = 1; i < clauses.size(); i++) {
            pwff new_clause = clauses[i];
            if (*second == *falsum) {
                ret = tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(orfa2_rp, {{"ph", first->get_type_prover(tb)}, {"ps", new_clause->get_type_prover(tb)}}, {ret});
                second = new_clause;
            } else {
                if (*new_clause == *falsum) {
                    ret = tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(orfa_rp, {{"ph", first->get_type_prover(tb)}, {"ps", second->get_type_prover(tb)}}, {ret});
                } else {
                    ret = tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(orim1i_rp, {{"ph", first->get_type_prover(tb)}, {"ps", second->get_type_prover(tb)}, {"ch", new_clause->get_type_prover(tb)}}, {ret});
                    second = Or::create(second, new_clause);
                }
            }
            first = Or::create(first, new_clause);
        }
        return make_tuple(ret, first, second);
    }
}

RegisteredProver orim12d_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps -> ch ) )", "|- ( ph -> ( th -> ta ) )"}, "|- ( ph -> ( ( ps \\/ th ) -> ( ch \\/ ta ) ) )");

std::tuple< Prover< CreativeCheckpointedProofEngine< Sentence > >, pwff, pwff > join_or_imp(const std::vector< pwff > &orig_clauses, const std::vector< pwff > &new_clauses, std::vector< Prover< CreativeCheckpointedProofEngine< Sentence > > > &provers, const pwff &abs, const LibraryToolbox &tb) {
    assert(orig_clauses.size() == new_clauses.size());
    assert(orig_clauses.size() == provers.size());
    if (orig_clauses.size() == 0) {
        return make_tuple(tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(ff_rp, {}, {}), False::create(), False::create());
    } else {
        Prover< CreativeCheckpointedProofEngine< Sentence > > ret = provers[0];
        pwff orig_cl = orig_clauses[0];
        pwff new_cl = new_clauses[0];
        for (size_t i = 1; i < orig_clauses.size(); i++) {
            ret = tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(orim12d_rp, {{"ph", abs->get_type_prover(tb)}, {"ps", orig_cl->get_type_prover(tb)}, {"ch", new_cl->get_type_prover(tb)}, {"th", orig_clauses[i]->get_type_prover(tb)}, {"ta", new_clauses[i]->get_type_prover(tb)}}, {ret, provers[i]});
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
    z3::solver &s;
    const LibraryToolbox &tb;

    std::vector< pwff > hyps;
    pwff thesis;

    pwff target;
    pwff and_hyps;
    pwff abs;

    Z3Adapter(z3::solver &s, const LibraryToolbox &tb) : s(s), tb(tb) {}

    void add_formula(z3::expr e, bool hyp) {
        if (hyp) {
            this->s.add(e);
        } else {
            this->s.add(!e);
        }
        pwff w = parse_expr(e, tb);
        if (hyp) {
            this->hyps.push_back(w);
            if (this->target == nullptr) {
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
            if (this->target == nullptr) {
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

    Prover< CreativeCheckpointedProofEngine< Sentence > > convert_proof(z3::expr e, int depth = 0) {
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
#ifdef VERBOSE_Z3
                std::cout << std::string(depth, ' ');
                std::cout << "Declaration: " << decl << " of arity " << decl.arity() << " and args num " << num_args << std::endl;
#endif

                switch (kind) {
                case Z3_OP_PR_ASSERTED: {
                    assert(num_args == 1);
                    assert(arity == 1);
#ifdef VERBOSE_Z3
                    //cout << "EXPR: " << e.arg(0) << endl;
                    /*cout << "HEAD WFF: " << this->get_current_abs_hyps()->to_string() << endl;
                    cout << "WFF: " << parse_expr(e.arg(0))->to_string() << endl;*/
#endif
                    pwff w = parse_expr(e.arg(0), tb);
                    assert(std::find_if(this->hyps.begin(), this->hyps.end(), [=](const pwff &p) { return *p == *w; }) != this->hyps.end());
                    //auto it = find_if(this->hyps.begin(), this->hyps.end(), [=](const pwff &p) { return *p == *w; });
                    //size_t pos = it - this->hyps.begin();
                    assert(this->hyps.size() >= 1);
                    Prover< CreativeCheckpointedProofEngine< Sentence > > ret = this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(id_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}}, {});
                    pwff cur_hyp = this->get_current_abs_hyps();
                    for (size_t step = 0; step < this->hyps.size()-1; step++) {
                        auto and_hyp = std::dynamic_pointer_cast< const And >(cur_hyp);
                        assert(and_hyp != nullptr);
                        pwff left = and_hyp->get_a();
                        pwff right = and_hyp->get_b();
                        if (*and_hyp->get_b() == *w) {
                            cur_hyp = and_hyp->get_b();
                            ret = this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(simprd_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", left->get_type_prover(this->tb)}, {"ch", right->get_type_prover(this->tb)}}, {ret});
                            break;
                        }
                        cur_hyp = and_hyp->get_a();
                        ret = this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(simpld_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", left->get_type_prover(this->tb)}, {"ch", right->get_type_prover(this->tb)}}, {ret});
                    }
                    assert(*cur_hyp == *w);
                    return ret;
                    break; }
                case Z3_OP_PR_MODUS_PONENS: {
                    assert(num_args == 3);
                    assert(arity == 3);
#ifdef VERBOSE_Z3
                    //cout << endl << "EXPR: " << e.arg(2);
                    /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                    cout << "HP2: " << parse_expr(extract_thesis(e.arg(1)))->to_string() << endl;
                    cout << "TH: " << parse_expr(e.arg(2))->to_string() << endl;*/
#endif
                    Prover< CreativeCheckpointedProofEngine< Sentence > > p1 = this->convert_proof(e.arg(0), depth+1);
                    Prover< CreativeCheckpointedProofEngine< Sentence > > p2 = this->convert_proof(e.arg(1), depth+1);
                    switch (extract_thesis(e.arg(1)).decl().decl_kind()) {
                    case Z3_OP_EQ:
                    case Z3_OP_IFF:
                        return this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(mp2_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", parse_expr(extract_thesis(e.arg(0)), tb)->get_type_prover(this->tb)}, {"ch", parse_expr(e.arg(2), tb)->get_type_prover(this->tb)}}, {p1, p2});
                        break;
                    case Z3_OP_IMPLIES:
                        return this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(mp1_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", parse_expr(extract_thesis(e.arg(0)), tb)->get_type_prover(this->tb)}, {"ch", parse_expr(e.arg(2), tb)->get_type_prover(this->tb)}}, {p1, p2});
                        break;
                    default:
                        throw "Should not arrive here";
                        break;
                    }
                    break; }
                case Z3_OP_PR_REWRITE: {
                    assert(num_args == 1);
                    assert(arity == 1);
#ifdef VERBOSE_Z3
                    /*cout << "EXPR: " << e.arg(0) << endl;
                    cout << "WFF: " << parse_expr(e.arg(0))->to_string() << endl;*/
#endif
                    //prove_and_print(parse_expr(e.arg(0)), tb);
                    // The thesis should be true independently of ph
                    pwff thesis = parse_expr(e.arg(0), tb);
#ifdef VERBOSE_Z3
                    //cout << "ORACLE for '" << thesis->to_string() << "'!" << endl;
#endif
                    //Prover p1 = thesis->get_adv_truth_prover(tb);
                    auto pt = pt2_to_pt(thesis->to_parsing_tree(this->tb));
                    auto sent = this->tb.reconstruct_sentence(pt, this->tb.get_turnstile());
                    Prover< CreativeCheckpointedProofEngine< Sentence > > p1 = this->tb.build_prover({}, sent, {}, {});
                    return this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(a1i_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", thesis->get_type_prover(this->tb)}}, {p1});
                    break; }
                case Z3_OP_PR_TRANSITIVITY: {
                    assert(num_args == 3);
                    assert(arity == 3);
#ifdef VERBOSE_Z3
                    //cout << endl << "EXPR: " << e.arg(2);
                    /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                    cout << "HP2: " << parse_expr(extract_thesis(e.arg(1)))->to_string() << endl;
                    cout << "TH: " << parse_expr(e.arg(2))->to_string() << endl;*/
#endif
                    Prover< CreativeCheckpointedProofEngine< Sentence > > p1 = this->convert_proof(e.arg(0), depth+1);
                    Prover< CreativeCheckpointedProofEngine< Sentence > > p2 = this->convert_proof(e.arg(1), depth+1);
                    auto w1 = std::dynamic_pointer_cast< const Biimp >(parse_expr(extract_thesis(e.arg(0)), tb));
                    auto w2 = std::dynamic_pointer_cast< const Biimp >(parse_expr(extract_thesis(e.arg(1)), tb));
                    assert(w1 != nullptr && w2 != nullptr);
                    assert(*w1->get_b() == *w2->get_a());
                    pwff ps = w1->get_a();
                    pwff ch = w1->get_b();
                    pwff th = w2->get_b();
                    Prover< CreativeCheckpointedProofEngine< Sentence > > ret = this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(bitrd_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", ps->get_type_prover(this->tb)}, {"ch", ch->get_type_prover(this->tb)}, {"th", th->get_type_prover(this->tb)}}, {p1, p2});
                    //cerr << "TEST: " << test_prover(ret, this->tb) << endl;
                    return ret;
                    break; }
                case Z3_OP_PR_MONOTONICITY: {
                    assert(num_args == 2);
                    assert(arity == 2);
#ifdef VERBOSE_Z3
                    /*cout << "EXPR: " << e.arg(num_args-1) << endl;
                    cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                    cout << "TH: " << parse_expr(e.arg(1))->to_string() << endl;*/
#endif
                    // Recognize the monotonic operation
                    z3::expr thesis = e.arg(num_args-1);
                    switch (thesis.decl().decl_kind()) {
                    /*case Z3_OP_EQ:
                        break;*/
                    case Z3_OP_IFF: {
                        assert(thesis.num_args() == 2);
                        z3::expr th_left = thesis.arg(0);
                        z3::expr th_right = thesis.arg(1);
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
                            std::vector< Prover< CreativeCheckpointedProofEngine< Sentence > > > hyp_provers;
                            std::vector< pwff > left_wffs;
                            std::vector< pwff > right_wffs;
                            //vector< Prover > wffs_prover;
                            size_t used = 0;
                            for (unsigned int i = 0; i < th_left.num_args(); i++) {
                                if (eq(th_left.arg(i), th_right.arg(i))) {
                                    hyp_provers.push_back(this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(biidd_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", parse_expr(th_left.arg(i), tb)->get_type_prover(this->tb)}}, {}));
                                } else {
                                    assert(!used);
                                    hyp_provers.push_back(this->convert_proof(e.arg(0), depth+1));
                                    used++;
                                }
                                left_wffs.push_back(parse_expr(th_left.arg(i), tb));
                                right_wffs.push_back(parse_expr(th_right.arg(i), tb));
                            }
                            assert(hyp_provers.size() == th_left.num_args());
                            assert(used == num_args - 1);
                            Prover< CreativeCheckpointedProofEngine< Sentence > > ret = hyp_provers[0];
                            pwff left_wff = left_wffs[0];
                            pwff right_wff = right_wffs[0];
                            for (size_t i = 1; i < th_left.num_args(); i++) {
                                ret = this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)},
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
                            pwff left_wff = parse_expr(th_left.arg(0), tb);
                            pwff right_wff = parse_expr(th_right.arg(0), tb);
                            Prover< CreativeCheckpointedProofEngine< Sentence > > ret = this->convert_proof(e.arg(0), depth+1);
                            return this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(notbid_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)},
                                                                                  {"ps", left_wff->get_type_prover(this->tb)}, {"ch", right_wff->get_type_prover(this->tb)}}, {ret});
                            break; }
                        default:
                            std::cerr << "Unsupported operation " << th_left.decl().decl_kind() << std::endl;
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
#ifdef VERBOSE_Z3
                    /*cout << "Unit resolution: " << num_args << " with arity " << decl.arity() << endl;
                    for (unsigned i = 0; i < num_args-1; i++) {
                        cout << "HP" << i << ": " << extract_thesis(e.arg(i)) << endl;
                        cout << "HP" << i << ": " << parse_expr(extract_thesis(e.arg(i)))->to_string() << endl;
                    }
                    cout << "TH: " << e.arg(num_args-1) << endl;
                    cout << "TH: " << parse_expr(e.arg(num_args-1))->to_string() << endl;*/
#endif

                    size_t elims_num = num_args - 2;
                    z3::expr or_expr = extract_thesis(e.arg(0));
                    assert(or_expr.decl().decl_kind() == Z3_OP_OR);
                    size_t clauses_num = or_expr.num_args();
#ifdef NDEBUG
                    (void) clauses_num;
#endif
                    assert(clauses_num >= 2);
                    assert(elims_num <= clauses_num);

                    Prover< CreativeCheckpointedProofEngine< Sentence > > orig_prover = this->convert_proof(e.arg(0), depth+1);
                    //cerr << "TEST: " << test_prover(orig_prover, this->tb) << endl;

                    std::vector< pwff > elims;
                    std::vector< Prover< CreativeCheckpointedProofEngine< Sentence > > > elim_provers;
                    for (unsigned int i = 0; i < elims_num; i++) {
                        elims.push_back(parse_expr(extract_thesis(e.arg(i+1)), tb));
                        elim_provers.push_back(this->convert_proof(e.arg(i+1), depth+1));
                        //cerr << "TEST: " << test_prover(elim_provers.back(), this->tb) << endl;
                    }

                    std::vector< pwff > orig_clauses;
                    std::vector< pwff > new_clauses;
                    std::vector< Prover< CreativeCheckpointedProofEngine< Sentence > > > provers;
                    for (unsigned int i = 0; i < or_expr.num_args(); i++) {
                        pwff clause = parse_expr(or_expr.arg(i), tb);
                        orig_clauses.push_back(clause);

                        // Search an eliminator for the positive form
                        auto elim_it = std::find_if(elims.begin(), elims.end(), [=](const pwff &w){ return *w == *Not::create(clause); });
                        if (elim_it != elims.end()) {
                            size_t pos = elim_it - elims.begin();
                            provers.push_back(this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(urt_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", clause->get_type_prover(this->tb)}}, {elim_provers[pos]}));
                            new_clauses.push_back(False::create());
                            //cerr << "TEST 1: " << test_prover(provers.back(), this->tb) << endl;
                            continue;
                        }

                        // Search an eliminator for the negative form
                        auto clause_not = std::dynamic_pointer_cast< const Not >(clause);
                        if (clause_not != nullptr) {
                            auto elim_it = std::find_if(elims.begin(), elims.end(), [=](const pwff &w){ return *w == *clause_not->get_a(); });
                            if (elim_it != elims.end()) {
                                size_t pos = elim_it - elims.begin();
                                provers.push_back(this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(urf_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", clause_not->get_a()->get_type_prover(this->tb)}}, {elim_provers[pos]}));
                                new_clauses.push_back(False::create());
                                //cerr << "TEST 1: " << test_prover(provers.back(), this->tb) << endl;
                                continue;
                            }
                        }

                        // No eliminator found, keeping the clause and pushing a trivial proved
                        provers.push_back(this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(idd_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", clause->get_type_prover(this->tb)}}, {}));
                        new_clauses.push_back(clause);
                        //cerr << "TEST 1: " << test_prover(provers.back(), this->tb) << endl;
                    }

                    Prover< CreativeCheckpointedProofEngine< Sentence > > joined_or_prover;
                    Prover< CreativeCheckpointedProofEngine< Sentence > > simplifcation_prover;
                    pwff orig_clause;
                    pwff new_clause;
                    pwff new_clause2;
                    pwff simplified_clause;
                    std::tie(joined_or_prover, orig_clause, new_clause) = join_or_imp(orig_clauses, new_clauses, provers, this->get_current_abs_hyps(), this->tb);
                    std::tie(simplifcation_prover, new_clause2, simplified_clause) = simplify_or(new_clauses, this->tb);
#ifdef VERBOSE_Z3
                    /*cout << orig_clause->to_string() << endl;
                    cout << new_clause->to_string() << endl;
                    cout << new_clause2->to_string() << endl;
                    cout << simplified_clause->to_string() << endl;*/
#endif
                    assert(*new_clause == *new_clause2);
                    Prover< CreativeCheckpointedProofEngine< Sentence > > ret = this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(mpd_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", orig_clause->get_type_prover(this->tb)}, {"ch", new_clause->get_type_prover(this->tb)}}, {orig_prover, joined_or_prover});
                    ret = this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(syl_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", new_clause->get_type_prover(this->tb)}, {"ch", simplified_clause->get_type_prover(this->tb)}}, {ret, simplifcation_prover});
                    //cerr << "TEST: " << test_prover(ret, this->tb) << endl;
                    //assert(false);
                    return ret;
                    break; }
                case Z3_OP_PR_DEF_AXIOM: {
                    assert(num_args == 1);
                    assert(arity == 1);
#ifdef VERBOSE_Z3
                    std::cout << "EXPR: " << e.arg(0) << std::endl;
                    std::cout << "WFF: " << parse_expr(e.arg(0), tb)->to_string() << std::endl;
#endif
                    pwff thesis = parse_expr(extract_thesis(e), tb);
#ifdef VERBOSE_Z3
                    std::cout << "AXIOM ORACLE for '" << thesis->to_string() << "'!" << std::endl;
#endif
                    auto p1 = thesis->get_adv_truth_prover(this->tb).second;
                    return this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(a1i_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", thesis->get_type_prover(this->tb)}}, {p1});
                    break; }
                case Z3_OP_PR_NOT_OR_ELIM: {
                    assert(num_args == 2);
                    assert(arity == 2);
#ifdef VERBOSE_Z3
                    //cout << endl << "EXPR: " << e.arg(2);
                    /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;;
                    cout << "TH: " << parse_expr(e.arg(1))->to_string() << endl;*/
#endif
                    z3::expr not_expr = extract_thesis(e.arg(0));
                    assert(not_expr.decl().decl_kind() == Z3_OP_NOT);
                    assert(not_expr.num_args() == 1);
                    z3::expr or_expr = not_expr.arg(0);
                    assert(or_expr.decl().decl_kind() == Z3_OP_OR);
                    z3::expr not_target_expr = e.arg(1);
                    assert(not_target_expr.decl().decl_kind() == Z3_OP_NOT);
                    assert(not_target_expr.num_args() == 1);
                    pwff target_wff = parse_expr(not_target_expr.arg(0), tb);
                    Prover< CreativeCheckpointedProofEngine< Sentence > > ret = this->convert_proof(e.arg(0), depth+1);
                    pwff wff = parse_expr(or_expr, tb);
                    for (size_t i = 0; i < or_expr.num_args()-1; i++) {
                        auto wff_or = std::dynamic_pointer_cast< const Or >(wff);
                        assert(wff != nullptr);
                        if (*wff_or->get_b() == *target_wff) {
                            return this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(orsird_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", wff_or->get_a()->get_type_prover(this->tb)}, {"ch", wff_or->get_b()->get_type_prover(this->tb)}}, {ret});
                        } else {
                            ret = this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(orsild_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", wff_or->get_a()->get_type_prover(this->tb)}, {"ch", wff_or->get_b()->get_type_prover(this->tb)}}, {ret});
                            wff = wff_or->get_a();
                        }
                    }
                    assert(*wff == *target_wff);
                    return ret;
                    break; }
                case Z3_OP_PR_IFF_FALSE: {
                    assert(num_args == 2);
                    assert(arity == 2);
#ifdef VERBOSE_Z3
                    //cout << endl << "EXPR: " << e.arg(2);
                    /*cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << endl;
                    cout << "TH: " << parse_expr(e.arg(1))->to_string() << endl;*/
#endif
                    pwff hyp = parse_expr(extract_thesis(e.arg(0)), tb);
                    Prover< CreativeCheckpointedProofEngine< Sentence > > hyp_prover = this->convert_proof(e.arg(0), depth+1);
                    auto hyp_not = std::dynamic_pointer_cast< const Not >(hyp);
                    assert(hyp_not != nullptr);
                    return this->tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(bifald_rp, {{"ph", this->get_current_abs_hyps()->get_type_prover(this->tb)}, {"ps", hyp_not->get_a()->get_type_prover(this->tb)}}, {hyp_prover});
                    break; }
                case Z3_OP_PR_LEMMA: {
                    assert(num_args == 2);
                    assert(arity == 2);
#ifdef VERBOSE_Z3
                    //std::cout << "FULL: " << e << std::endl;
                    /*std::cout << "HP1: " << parse_expr(extract_thesis(e.arg(0)))->to_string() << std::endl;
                    std::cout << "TH: " << e.arg(1) << std::endl;
                    std::cout << "TH: " << parse_expr(e.arg(1))->to_string() << std::endl;*/
                    std::cout << "LEMMA ORACLE for '" << Imp::create(this->get_current_abs_hyps(), parse_expr(extract_thesis(e), tb))->to_string() << "'!" << std::endl;
#endif
                    return Imp::create(this->get_current_abs_hyps(), parse_expr(extract_thesis(e), tb))->get_adv_truth_prover(this->tb).second;
                    break; }
                /*case Z3_OP_PR_HYPOTHESIS:
#ifdef VERBOSE_Z3
                    std::cout << "hypothesis";
#endif
                    assert(num_args == 1);
                    assert(arity == 1);
#ifdef VERBOSE_Z3
                    std::cout << std::endl << "PARENT: " << *parent;
                    //std::cout << std::endl << "EXPR: " << e.arg(0);
                    std::cout << std::endl << "WFF: " << parse_expr(e.arg(0))->to_string();
#endif
                    break;*/
                default:
                    //prove_and_print(Imp::create(w, parse_expr(extract_thesis(e))), tb);
#ifdef VERBOSE_Z3
                    std::cout << "GENERIC ORACLE for '" << Imp::create(this->get_current_abs_hyps(), parse_expr(extract_thesis(e), tb))->to_string() << "'!" << std::endl;
#endif
                    return Imp::create(this->get_current_abs_hyps(), parse_expr(extract_thesis(e), tb))->get_adv_truth_prover(this->tb).second;
                    break;
                }
            } else {
#ifdef VERBOSE_Z3
                std::cout << "unknown kind " << kind << std::endl;
#endif
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

#ifdef ENABLE_TEST_CODE
BOOST_DATA_TEST_CASE(test_wff_minisat_prover, boost::unit_test::data::xrange(3), i) {
    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    z3::set_param("proof", true);
    z3::context c;

    z3::solver s(c);
    auto adapter = Z3Adapter(s, tb);

    if (i == 0) {
        z3::expr ph = c.bool_const("ph");
        z3::expr ps = c.bool_const("ps");
        adapter.add_formula(((!(ph && ps)) == (!ph || !ps)), false);
    }

    if (i == 1) {
        z3::expr ph = c.bool_const("ph");
        z3::expr ps = c.bool_const("ps");
        adapter.add_formula((ph || ps ) == (ps || ph), false);
    }

    if (i == 2) {
        z3::expr ph = c.bool_const("ph");
        z3::expr ps = c.bool_const("ps");
        z3::expr ch = c.bool_const("ch");
        adapter.add_formula(implies(ph, implies(ps, ch)), true);
        adapter.add_formula(implies(!ph, implies(!ps, ch)), true);
        adapter.add_formula(implies(ph == ps, ch), false);
    }

    //std::cout << "ABSURDUM HYPOTHESIS: " << adapter.abs->to_string() << std::endl;
    //std::cout << "TARGET: " << adapter.target->to_string() << std::endl;
    //prove_and_print(adapter.target, tb);

    /*switch (adapter.s.check()) {
    case z3::unsat:   std::cout << "valid\n"; break;
    case z3::sat:     std::cout << "not valid\n"; break;
    case z3::unknown: std::cout << "unknown\n"; break;
    }*/

    BOOST_TEST(adapter.s.check() == z3::unsat);

    CreativeProofEngineImpl< Sentence > engine(tb, true);
    Prover< CreativeCheckpointedProofEngine< Sentence > > main_prover = adapter.convert_proof(adapter.s.proof());
    bool res;
    if (adapter.hyps.size() == 1) {
        res = tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(refute_rp, {{"ph", adapter.target->get_type_prover(tb)}}, {main_prover})(engine);
    } else {
        res = tb.build_registered_prover< CreativeCheckpointedProofEngine< Sentence > >(efald_rp, {{"ph", adapter.and_hyps->get_type_prover(tb)}, {"ps", adapter.thesis->get_type_prover(tb)}}, {main_prover})(engine);
    }
    BOOST_TEST(res);
    BOOST_TEST(engine.get_stack().size() == (size_t) 1);
    // TODO Check that the correct sentence was proved!
    /*if (res) {
        std::cout << std::endl << "FINAL PROOF FOUND!" << std::endl;
        //std::cout << "proof: " << tb.print_proof(engine.get_proof_labels()) << std::endl;
        std::cout << "stack top: " << tb.print_sentence(engine.get_stack().back(), SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << std::endl;
        std::cout << "proof length: " << engine.get_proof_labels().size() << std::endl;
    } else {
        std::cout << "proof generation failed..." << std::endl;
    }
    std::cout << std::endl;*/
}
#endif

int test_z3_2_main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    try {
        z3::set_param("proof", true);
        z3::context c;
        Z3_set_ast_print_mode(c, Z3_PRINT_LOW_LEVEL);

        z3::solver s(c);
        z3::params param(c);
        param.set("mbqi", true);
        s.set(param);

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

        z3::sort things = c.uninterpreted_sort("thing");
        z3::sort bools = c.bool_sort();
        z3::expr x = c.constant("x", things);
        z3::expr y = c.constant("y", things);
        z3::func_decl p = c.function("p", things, bools);
        z3::func_decl q = c.function("q", things, bools);
        z3::expr thesis = (exists(x, forall(y, p(x) == p(y))) == (exists(x, q(x)) == exists(y, q(y)))) == (exists(x, forall(y, q(x) == q(y))) == (exists(x, p(x)) == exists(y, p(y))));
        s.add(!thesis);

        std::cout << "Solver:" << std::endl << s << std::endl;

        switch (s.check()) {
        case z3::unsat:   std::cout << "UNSAT\n"; break;
        case z3::sat:     std::cout << "SAT\n"; break;
        case z3::unknown: std::cout << "UNKNOWN\n"; break;
        }

        auto proof = s.proof();
        std::cout << "Proof:" << std::endl << proof << std::endl;

        std::cout << std::endl;
    } catch (z3::exception &e) {
        std::cerr << "Caught exception:\n" << e << std::endl;
    }

    return 0;
}
static_block {
    //register_main_function("test_z3_2", test_z3_2_main);
}
