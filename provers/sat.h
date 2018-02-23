#pragma once

#include <vector>
#include <cstdint>
#include <ostream>

#include "libs/minisat/Solver.h"

struct CNFProblem {
    size_t var_num;
    std::vector< std::vector< std::pair< bool, uint32_t > > > clauses;

    void print_dimacs(std::ostream &stream);
    void feed_to_minisat(Minisat::Solver &solver);
};

Minisat::Lit to_minisat_literal(const std::pair< bool, uint32_t > &lit);
std::pair< bool, uint32_t > from_minisat_literal(const Minisat::Lit &lit);
int32_t to_number_literal(const std::pair< bool, uint32_t > &lit);
