#pragma once

#include <vector>
#include <set>
#include <cstdint>
#include <ostream>
#include <memory>
#include <functional>

#include "libs/minisat/Solver.h"

// A literal is represented by a sign (true is positive) and a zero-based number
typedef std::pair< bool, uint32_t > Literal;
typedef std::vector< Literal > Clause;

Minisat::Lit to_minisat_literal(const std::pair< bool, uint32_t > &lit);
std::pair< bool, uint32_t > from_minisat_literal(const Minisat::Lit &lit);
int32_t to_number_literal(const Literal &lit);
Literal from_number_literal(int32_t lit);
void print_clause(std::ostream &stream, const Clause &clause);

struct CNFCallback {
    // Push NOT context -> clause on the stack
    virtual void prove_clause(size_t idx, const Clause &context) = 0;
    // Push NOT context -> lit on the stack
    virtual void prove_not_or_elim(const Literal &lit, const Clause &context) = 0;
    // Pop clause from the stack and push NOT context -> clause
    virtual void prove_imp_intr(const Clause &clause, const Clause &context) = 0;
    // Pop NOT context -> ( lit_1 \/ ... \/ lit_n ) from the stack, pop NOT context -> -. lit_i for all i except unsolved_idx, then push NOT context -> lit_{unsolved_idx}
    virtual void prove_unit_res(const Clause &clause, size_t unsolved_idx, const Clause &context) = 0;
    // Pop NOT context -> lit and NOT context -> -. lit from stack, and push context (lit is always given positive)
    virtual void prove_absurduum(const Literal &lit, const Clause &context) = 0;
};

struct CNFCallbackTest : public CNFCallback {
    void prove_clause(size_t idx, const Clause &context);
    void prove_not_or_elim(const Literal &lit, const Clause &context);
    void prove_imp_intr(const Clause &clause, const Clause &context);
    void prove_unit_res(const Clause &clause, size_t unsolved_idx, const Clause &context);
    void prove_absurduum(const Literal &lit, const Clause &context);

    std::vector< Clause > orig_clauses;
};

struct CNFProblem {
    size_t var_num;
    std::vector< Clause > clauses;
    CNFCallback *callback;

    void print_dimacs(std::ostream &stream) const;
    void feed_to_minisat(Minisat::Solver &solver) const;
    std::pair< bool, std::function< void() > > solve();

private:
    std::tuple<bool, std::vector<std::pair<Literal, const std::vector<Literal> *> >, std::function<void ()> > do_unit_propagation(const std::vector< Literal > &assumptions) const;
    std::vector< std::function< void(const Clause &context) > > callbacks;
};

