
#include "sat.h"

#include <iostream>
#include <tuple>

using namespace std;

Minisat::Lit to_minisat_literal(const std::pair<bool, uint32_t> &lit)
{
    return Minisat::mkLit(lit.second, !lit.first);
}

std::pair<bool, uint32_t> from_minisat_literal(const Minisat::Lit &lit)
{
    return make_pair(!Minisat::sign(lit), Minisat::var(lit));
}

int32_t to_number_literal(const Literal &lit)
{
    return (lit.second + 1) * (lit.first ? 1 : -1);
}

void CNFProblem::print_dimacs(ostream &stream) const
{
    stream << "p cnf " << this->var_num << " " << this->clauses.size() << endl;
    for (const auto &clause : clauses) {
        for (const auto &lit : clause) {
            stream << to_number_literal(lit) << " ";
        }
        stream << "0" << endl;
    }
}

void CNFProblem::feed_to_minisat(Minisat::Solver &solver) const
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

std::pair<bool, std::vector<std::pair<Literal, const std::vector<Literal> *> > > CNFProblem::do_unit_propagation(const std::vector<Literal> &assumptions) const
{
    set< Literal > ret_set;
    std::vector<std::pair<Literal, const std::vector<Literal> *> > ret;
    for (const auto &lit : assumptions) {
        auto neg_lit = make_pair(!lit.first, lit.second);
        bool res;
        tie(ignore, res) = ret_set.insert(lit);
        if (res) {
            ret.push_back(make_pair(lit, nullptr));
        }
        if (ret_set.find(neg_lit) != ret_set.end()) {
            // We already have a contradiction in the assumptions
            return make_pair(false, ret);
        }
    }
    bool cont = true;
    while (cont) {
        cont = false;
        for (const auto &clause : this->clauses) {
            bool unsolved_found = false;
            bool skip_clause = false;
            Literal unsolved;
            Literal neg_unsolved;
            for (const auto &lit : clause) {
                if (ret_set.find(lit) != ret_set.end()) {
                    // The clause is automatically true and thus useless
                    skip_clause = true;
                    break;
                }
                auto neg_lit = make_pair(!lit.first, lit.second);
                if (ret_set.find(neg_lit) != ret_set.end()) {
                    // The literal is automatically false, so we can ignore it
                    continue;
                } else {
                    if (unsolved_found) {
                        // More than one unsolved literal, we cannot deduce anything
                        skip_clause = true;
                        break;
                    } else {
                        // Now we have an unsolved literal
                        unsolved_found = true;
                        unsolved = lit;
                        neg_unsolved = neg_lit;
                    }
                }
            }
            if (skip_clause) {
                continue;
            }
            if (!unsolved_found) {
                // If no unsolved literal was found, any literal will generate a contradiction; we just pick the first
                unsolved = clause.at(0);
                neg_unsolved = make_pair(!unsolved.first, unsolved.second);
            }
            // We found exactly one unsolved literal (or perhaps anyone if noone is solved), so it must be true
            bool res;
            tie(ignore, res) = ret_set.insert(unsolved);
            if (res) {
                ret.push_back(make_pair(unsolved, &clause));
            }
            if (ret_set.find(neg_unsolved) != ret_set.end()) {
                return make_pair(false, ret);
            }
            cont = true;
        }
    }
    return make_pair(true, ret);
}
