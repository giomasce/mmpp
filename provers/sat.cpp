
#include "sat.h"

#include <iostream>
#include <functional>
#include <map>
#include <tuple>
#include <cmath>

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

Literal from_number_literal(int32_t lit)
{
    return make_pair(lit > 0, abs(lit) - 1);
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

std::tuple<bool, std::vector<std::pair<Literal, const std::vector<Literal> *> >, std::function< void() > > CNFProblem::do_unit_propagation(const std::vector<Literal> &assumptions) const
{
    map< Literal, size_t > ret_map;
    std::vector<std::pair<Literal, const std::vector<Literal> *> > ret;
    std::vector< std::function< void() > > ret_provers;
    Clause orig_clause;
    for (const auto &lit : assumptions) {
        orig_clause.push_back(make_pair(!lit.first, lit.second));
    }
    for (const auto &lit : assumptions) {
        auto neg_lit = make_pair(!lit.first, lit.second);
        bool res;
        tie(ignore, res) = ret_map.insert(make_pair(lit, ret.size()));
        if (res) {
            ret.push_back(make_pair(lit, nullptr));
            ret_provers.push_back([lit,orig_clause]() {
                cout << "Putting on stack: NOT (";
                print_clause(cout, orig_clause);
                cout << ") -> " << to_number_literal(lit) << ", by conversion" << endl;
            });
        }
        if (ret_map.find(neg_lit) != ret_map.end()) {
            // We already have a contradiction in the assumptions; this should actually never happens...
            assert(false);
            return make_tuple(false, ret, [](){});
        }
    }
    bool cont = true;
    while (cont) {
        cont = false;
        for (size_t clause_idx = 0; clause_idx < this->clauses.size(); clause_idx++) {
            const auto &clause = this->clauses[clause_idx];
            const auto &clause_cb = this->callbacks[clause_idx];
            bool unsolved_found = false;
            bool skip_clause = false;
            Literal unsolved;
            Literal neg_unsolved;
            size_t unsolved_idx = 0;
            vector< function< void() > > used_provers;
            for (size_t lit_idx = 0; lit_idx < clause.size(); lit_idx++) {
                const auto &lit = clause[lit_idx];
                if (ret_map.find(lit) != ret_map.end()) {
                    // The clause is automatically true and thus useless
                    skip_clause = true;
                    break;
                }
                auto neg_lit = make_pair(!lit.first, lit.second);
                auto neg_it = ret_map.find(neg_lit);
                if (neg_it != ret_map.end()) {
                    // The literal is automatically false, so we can ignore it
                    used_provers.push_back(ret_provers[neg_it->second]);
                    continue;
                } else {
                    if (unsolved_found) {
                        // More than one unsolved literal, we cannot deduce anything
                        skip_clause = true;
                        break;
                    } else {
                        // Now we have an unsolved literal
                        unsolved_found = true;
                        unsolved_idx = lit_idx;
                        unsolved = lit;
                        neg_unsolved = neg_lit;
                    }
                }
            }
            if (skip_clause) {
                continue;
            }
            if (!unsolved_found) {
                // If no unsolved literal was found, any literal will generate a contradiction; we just pick the last one
                unsolved_idx = clause.size()-1;
                unsolved = clause.at(unsolved_idx);
                neg_unsolved = make_pair(!unsolved.first, unsolved.second);
                used_provers.pop_back();
            }
            // We found exactly one unsolved literal (or perhaps anyone if noone is solved), so it must be true
            bool res;
            tie(ignore, res) = ret_map.insert(make_pair(unsolved, ret.size()));
            if (res) {
                ret.push_back(make_pair(unsolved, &clause));
                assert(used_provers.size() + 1 == clause.size());
                ret_provers.push_back([clause_cb,orig_clause,used_provers,unsolved]() {
                    clause_cb->prove(orig_clause);
                    for (const auto &used_prover : used_provers) {
                        used_prover();
                    }
                    cout << "Popping " << used_provers.size() + 1 << " things from the stack and proving: NOT (";
                    print_clause(cout, orig_clause);
                    cout << ") -> " << to_number_literal(unsolved) << " by or resolution" << endl;
                });
            }
            auto conflict_it = ret_map.find(neg_unsolved);
            if (conflict_it != ret_map.end()) {
                auto pos_prover = ret_provers.back();
                auto neg_prover = ret_provers[conflict_it->second];
                if (!unsolved.first) {
                    swap(pos_prover, neg_prover);
                }
                return make_tuple(false, ret, [orig_clause,pos_prover,neg_prover](){
                    pos_prover();
                    neg_prover();
                    cout << "Popping 2 things from the stack and proving: (";
                    print_clause(cout, orig_clause);
                    cout << ") by contradiction" << endl;
                });
            }
            cont = true;
        }
    }
    return make_tuple(true, ret, [](){});
}

void print_clause(ostream &stream, const Clause &clause)
{
    bool first = true;
    for (const auto &lit : clause) {
        if (first) {
            first = false;
        } else {
            stream << " ";
        }
        stream << to_number_literal(lit);
    }
}

void HypClauseCallback::prove(const Clause &context)
{
    cout << "Putting on stack: NOT (";
    print_clause(cout, context);
    cout << ") -> (";
    print_clause(cout, this->clause);
    cout << "), by hypothesis" << endl;
}

HypClauseCallback::HypClauseCallback(const Clause &clause) : clause(clause) {}

void ProvenClauseCallback::prove(const Clause &context)
{
    this->prover();
    cout << "Popping 1 thing from the stack and proving: NOT (";
    print_clause(cout, context);
    cout << ") -> (";
    print_clause(cout, this->clause);
    cout << "), by simplification" << endl;
}

ProvenClauseCallback::ProvenClauseCallback(const Clause &clause, const std::function<void ()> &prover) : clause(clause), prover(prover)
{
}
