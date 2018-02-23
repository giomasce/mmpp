
#include "sat.h"

using namespace std;

Minisat::Lit to_minisat_literal(const std::pair<bool, uint32_t> &lit)
{
    return Minisat::mkLit(lit.second, !lit.first);
}

std::pair<bool, uint32_t> from_minisat_literal(const Minisat::Lit &lit)
{
    return make_pair(!Minisat::sign(lit), Minisat::var(lit));
}

int32_t to_number_literal(const std::pair<bool, uint32_t> &lit)
{
    return (lit.second + 1) * (lit.first ? 1 : -1);
}

void CNFProblem::print_dimacs(ostream &stream)
{
    stream << "p cnf " << this->var_num << " " << this->clauses.size() << endl;
    for (const auto &clause : clauses) {
        for (const auto &lit : clause) {
            stream << to_number_literal(lit) << " ";
        }
        stream << "0" << endl;
    }
}

void CNFProblem::feed_to_minisat(Minisat::Solver &solver)
{
    for (size_t i = 0; i < this->var_num; i++) {
        Minisat::Var var = solver.newVar();
        assert(var == (Minisat::Var) i);
    }
    for (const auto &clause : this->clauses) {
        Minisat::vec< Minisat::Lit > clause2;
        for (const auto &lit : clause) {
            clause2.push(to_minisat_literal(lit));
        }
        solver.addClause_(clause2);
    }
}
