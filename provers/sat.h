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

struct ClauseCallback {
    virtual void prove(const Clause &context) = 0;
};

struct HypClauseCallback : public ClauseCallback {
    void prove(const Clause &context) override;
    HypClauseCallback(const Clause &clause);

    Clause clause;
};

struct ProvenClauseCallback : public ClauseCallback {
    void prove(const Clause &context) override;
    ProvenClauseCallback(const Clause &clause, const std::function< void() > &prover);

    Clause clause;
    std::function< void() > prover;
};

struct CNFProblem {
    size_t var_num;
    std::vector< Clause > clauses;
    std::vector< std::shared_ptr< ClauseCallback > > callbacks;

    void print_dimacs(std::ostream &stream) const;
    void feed_to_minisat(Minisat::Solver &solver) const;
    std::tuple<bool, std::vector<std::pair<Literal, const std::vector<Literal> *> >, std::function<void ()> > do_unit_propagation(const std::vector< Literal > &assumptions) const;
};

