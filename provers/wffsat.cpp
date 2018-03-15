#include "wffsat.h"

#include "sat.h"
#include "wffblock.h"
#include "wff.h"

static const RegisteredProver idi_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ph )");
static const RegisteredProver concl_rp = LibraryToolbox::register_prover({"|- ( ps -> ch )"}, "|- ( ph -> ( ps -> ch ) )");
struct CNFCallbackImpl : public CNFCallbackTest {
    void prove_clause(size_t idx, const Clause &context) {
        //CNFCallbackTest::prove_clause(idx, context);
        pwff concl = this->clause_to_pwff(this->orig_clauses.at(idx));
        pwff loc_ctx = Not::create(this->clause_to_pwff(context));
        this->prover_stack.push_back(imp_intr_prover(concl, this->orig_provers.at(idx), this->glob_ctx, loc_ctx, this->tb));
    }

    void prove_not_or_elim(size_t idx, const Clause &context) {
        //CNFCallbackTest::prove_not_or_elim(idx, context);
        pwff loc_ctx = Not::create(this->clause_to_pwff(context));
        pwff concl = this->atoms[context[idx].second];
        if (context[idx].first) {
            concl = Not::create(concl);
        }
        std::vector< pwff > orands;
        for (const auto lit : context) {
            pwff new_lit = this->atoms[lit.second];
            if (!lit.first) {
                new_lit = Not::create(new_lit);
            }
            orands.push_back(new_lit);
        }
        auto p1 = this->tb.build_registered_prover(idi_rp, {{"ph", loc_ctx->get_type_prover(this->tb)}}, {});
        auto p2 = not_or_elim_prover(orands, idx, context[idx].first, p1, loc_ctx, this->tb);
        auto p3 = this->tb.build_registered_prover(concl_rp, {{"ph", this->glob_ctx->get_type_prover(this->tb)}, {"ps", loc_ctx->get_type_prover(this->tb)}, {"ch", concl->get_type_prover(this->tb)}}, {p2});
        this->prover_stack.push_back(p3);
    }

    void prove_imp_intr(const Clause &clause, const Clause &context) {
        //CNFCallbackTest::prove_imp_intr(clause, context);
        pwff loc_ctx = Not::create(this->clause_to_pwff(context));
        pwff concl = this->clause_to_pwff(clause);
        auto p1 = this->prover_stack.back();
        this->prover_stack.pop_back();
        auto p2 = imp_intr_prover(concl, p1, this->glob_ctx, loc_ctx, this->tb);
        this->prover_stack.push_back(p2);
    }

    void prove_unit_res(const Clause &clause, size_t unsolved_idx, const Clause &context) {
        //CNFCallbackTest::prove_unit_res(clause, unsolved_idx, context);
        pwff loc_ctx = Not::create(this->clause_to_pwff(context));
        std::vector< pwff > orands;
        for (const auto lit : clause) {
            pwff new_lit = this->atoms[lit.second];
            if (!lit.first) {
                new_lit = Not::create(new_lit);
            }
            orands.push_back(new_lit);
        }
        std::vector< bool > orand_sign;
        for (size_t i = 0; i < clause.size(); i++) {
            if (i != unsolved_idx) {
                orand_sign.push_back(clause[i].first);
            }
        }
        auto p1 = this->prover_stack[this->prover_stack.size()-clause.size()];
        std::vector< Prover< CheckpointedProofEngine > > provers(this->prover_stack.end()-clause.size()+1, this->prover_stack.end());
        this->prover_stack.resize(this->prover_stack.size()-clause.size());
        auto p2 = unit_res_prover(orands, orand_sign, unsolved_idx, p1, provers, this->glob_ctx, loc_ctx, this->tb);
        this->prover_stack.push_back(p2);
    }

    void prove_absurdum(const Literal &lit, const Clause &context) {
        //CNFCallbackTest::prove_absurdum(lit, context);
        pwff loc_ctx = this->clause_to_pwff(context);
        pwff concl = this->atoms[lit.second];
        auto neg_prover = this->prover_stack.back();
        this->prover_stack.pop_back();
        auto pos_prover = this->prover_stack.back();
        this->prover_stack.pop_back();
        auto p = absurdum_prover(concl, pos_prover, neg_prover, this->glob_ctx, loc_ctx, this->tb);
        this->prover_stack.push_back(p);
    }

    const LibraryToolbox &tb;
    //std::vector< Clause > orig_clauses;
    std::vector< Prover< CheckpointedProofEngine > > orig_provers;
    std::vector< Prover< CheckpointedProofEngine > > prover_stack;
    pwff glob_ctx;
    std::vector< pwff > atoms;

    CNFCallbackImpl(const LibraryToolbox &tb) : tb(tb) {}

    pwff clause_to_pwff(const Clause &c) {
        if (c.empty()) {
            return False::create();
        }
        pwff ret = this->atoms[c[0].second];
        if (!c[0].first) {
            ret = Not::create(ret);
        }
        for (size_t i = 1; i < c.size(); i++) {
            pwff new_lit = this->atoms[c[i].second];
            if (!c[i].first) {
                new_lit = Not::create(new_lit);
            }
            ret = Or::create(ret, new_lit);
        }
        return ret;
    }
};

static const RegisteredProver falsify_rp = LibraryToolbox::register_prover({"|- ( -. ph -> F. )"}, "|- ph");
std::pair<bool, Prover<CheckpointedProofEngine> > get_sat_prover(pwff wff, const LibraryToolbox &tb)
{
    auto glob_ctx = Not::create(wff);
    auto tseitin = glob_ctx->get_tseitin_cnf_problem(tb);
    auto &cnf = std::get<0>(tseitin);
    auto &ts_map = std::get<1>(tseitin);
    auto &provers = std::get<2>(tseitin);
    /*cnf.print_dimacs(cout);
    for (const auto &x : ts_map) {
        cout << (x.second + 1) << " : " << x.first->to_string() << endl;
    }*/
    CNFCallbackImpl cnf_cb(tb);
    cnf_cb.orig_clauses = cnf.clauses;
    cnf_cb.orig_provers = provers;
    cnf_cb.glob_ctx = glob_ctx;
    cnf_cb.atoms.resize(ts_map.size());
    for (const auto &x : ts_map) {
        cnf_cb.atoms[x.second] = x.first;
    }
    cnf.callback = &cnf_cb;
    const auto res = cnf.solve();
    if (res.first) {
        return make_pair(false, null_prover);
    } else {
        res.second();
        assert(cnf_cb.prover_stack.size() == 1);
        auto final_prover = tb.build_registered_prover(falsify_rp, {{"ph", wff->get_type_prover(tb)}}, {cnf_cb.prover_stack[0]});
        return make_pair(true, final_prover);
    }
}
