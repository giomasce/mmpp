#include "wffsat.h"

#include "sat.h"
#include "wffblock.h"
#include "wff.h"

using namespace std;

struct CNFCallbackImpl : public CNFCallback {
    void prove_clause(size_t idx, const Clause &context) {}
    void prove_not_or_elim(size_t idx, const Clause &context) {}
    void prove_imp_intr(const Clause &clause, const Clause &context) {}
    void prove_unit_res(const Clause &clause, size_t unsolved_idx, const Clause &context) {}
    void prove_absurdum(const Literal &lit, const Clause &context) {}

    std::vector< Clause > orig_clauses;
    vector< Prover< CheckpointedProofEngine > > orig_provers;
    pwff glob_ctx;
};

std::pair<bool, Prover<CheckpointedProofEngine> > get_sat_prover(pwff wff, const LibraryToolbox &tb)
{
    auto glob_ctx = Not::create(wff);
    auto tseitin = glob_ctx->get_tseitin_cnf_problem(tb);
    auto cnf = get<0>(tseitin);
    //auto ts_map = get<1>(tseitin);
    auto provers = get<2>(tseitin);
    /*cnf.print_dimacs(cout);
    for (const auto &x : ts_map) {
        cout << (x.second + 1) << " : " << x.first->to_string() << endl;
    }*/
    CNFCallbackImpl cnf_cb;
    cnf_cb.orig_clauses = cnf.clauses;
    cnf_cb.orig_provers = provers;
    cnf_cb.glob_ctx = glob_ctx;
    cnf.callback = &cnf_cb;
    const auto res = cnf.solve();
    if (res.first) {
        return make_pair(false, null_prover);
    } else {
        cout << "The formula is UNSATisfiable" << endl;
        cout << "Unwinding the proof..." << endl;
        //res.second();
        return make_pair(true, null_prover);
    }
}
