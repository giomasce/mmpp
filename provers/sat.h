#pragma once

#include <vector>
#include <set>
#include <cstdint>
#include <ostream>

#include "libs/minisat/Solver.h"

typedef std::pair< bool, uint32_t > Literal;

struct CNFProblem {
    size_t var_num;
    std::vector< std::vector< Literal > > clauses;

    void print_dimacs(std::ostream &stream) const;
    void feed_to_minisat(Minisat::Solver &solver) const;
    std::pair<bool, std::vector< std::pair< Literal, const std::vector< Literal >* > > > do_unit_propagation(const std::vector< Literal > &assumptions) const;
};

Minisat::Lit to_minisat_literal(const std::pair< bool, uint32_t > &lit);
std::pair< bool, uint32_t > from_minisat_literal(const Minisat::Lit &lit);
int32_t to_number_literal(const Literal &lit);
