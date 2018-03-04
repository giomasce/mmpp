
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
    auto callback = this->callback;
    for (const auto &lit : assumptions) {
        orig_clause.push_back(make_pair(!lit.first, lit.second));
    }
    for (const auto &lit : assumptions) {
        auto neg_lit = make_pair(!lit.first, lit.second);
        bool res;
        tie(ignore, res) = ret_map.insert(make_pair(lit, ret.size()));
        if (res) {
            ret.push_back(make_pair(lit, nullptr));
            ret_provers.push_back([callback,lit,orig_clause]() {
                callback->prove_not_or_elim(lit, orig_clause);
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
                ret_provers.push_back([callback,clause_cb,orig_clause,used_provers,clause,unsolved_idx]() {
                    clause_cb(orig_clause);
                    for (const auto &used_prover : used_provers) {
                        used_prover();
                    }
                    callback->prove_unit_res(clause, unsolved_idx, orig_clause);
                });
            }
            auto conflict_it = ret_map.find(neg_unsolved);
            if (conflict_it != ret_map.end()) {
                auto pos_prover = ret_provers.back();
                auto neg_prover = ret_provers[conflict_it->second];
                if (!unsolved.first) {
                    swap(pos_prover, neg_prover);
                }
                auto lit = unsolved;
                lit.first = true;
                return make_tuple(false, ret, [orig_clause,pos_prover,neg_prover,lit,callback](){
                    pos_prover();
                    neg_prover();
                    callback->prove_absurduum(lit, orig_clause);
                });
            }
            cont = true;
        }
    }
    return make_tuple(true, ret, [](){});
}

std::pair<bool, std::function<void ()> > CNFProblem::solve()
{
    const auto &callback = this->callback;
    this->callbacks.clear();
    for (size_t i = 0; i < this->clauses.size(); i++) {
        this->callbacks.push_back([callback,i](const auto &context){
            callback->prove_clause(i, context);
        });
    }
    Minisat::Solver solver;
    this->feed_to_minisat(solver);
    bool res = solver.solve();
    if (res) {
        return make_pair(true, [](){});
    }
    // For some reason, even when the problem is UNSAT, the solver does not push the empty clause at the end
    solver.refutation.push_back({true, {}});
    for (const auto &ref : solver.refutation) {
        if (!ref.first) {
            // TODO There is room for optimization using clause deletion here
            continue;
        }
        //cout << "Inferring clause:";
        std::vector< Literal > negated_literals;
        std::vector< Literal > ref2;
        for (const auto &lit : ref.second) {
            auto lit2 = from_minisat_literal(lit);
            //cout << " " << to_number_literal(lit2);
            negated_literals.push_back(make_pair(!lit2.first, lit2.second));
            ref2.push_back(lit2);
        }
        //cout << endl;
        auto propagation = this->do_unit_propagation(negated_literals);
        assert(!get<0>(propagation));
        /*cout << "Unit propagation trace:" << endl;
        for (const auto &lit : get<1>(propagation)) {
            cout << " * " << to_number_literal(lit.first);
            if (lit.second) {
                cout << " from clause";
                for (const auto &lit2 : *lit.second) {
                    cout << " " << to_number_literal(lit2);
                }
            }
            cout << endl;
        }*/
        // The refutation worked, so that we can add the new clause
        this->clauses.push_back(ref2);
        const auto &prover = get<2>(propagation);
        this->callbacks.push_back([prover,callback,ref2](const auto &context) {
            prover();
            callback->prove_imp_intr(context, ref2);
        });
        if (ref.second.empty()) {
            // We have finally proved the empty clause, so we can return
            return make_pair(false, prover);
        }
    }
    assert(!"Should never arrive here");
    return {};
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

void CNFCallbackTest::prove_clause(size_t idx, const Clause &context)
{
    cout << "Putting on stack: NOT (";
    print_clause(cout, context);
    cout << ") -> (";
    print_clause(cout, this->orig_clauses.at(idx));
    cout << "), by hypothesis" << endl;
}

void CNFCallbackTest::prove_not_or_elim(const Literal &lit, const Clause &context)
{
    cout << "Putting on stack: NOT (";
    print_clause(cout, context);
    cout << ") -> " << to_number_literal(lit) << ", by conversion" << endl;
}

void CNFCallbackTest::prove_imp_intr(const Clause &clause, const Clause &context)
{
    cout << "Popping 1 thing from the stack and proving: NOT (";
    print_clause(cout, context);
    cout << ") -> (";
    print_clause(cout, clause);
    cout << "), by implication introduction" << endl;
}

void CNFCallbackTest::prove_unit_res(const Clause &clause, size_t unsolved_idx, const Clause &context)
{
    cout << "Popping " << clause.size() << " things from the stack and proving: NOT (";
    print_clause(cout, context);
    cout << ") -> " << to_number_literal(clause[unsolved_idx]) << " by unit resolution" << endl;
}

void CNFCallbackTest::prove_absurduum(const Literal &lit, const Clause &context)
{
    (void) lit;
    cout << "Popping 2 things from the stack and proving: (";
    print_clause(cout, context);
    cout << ") by contradiction" << endl;
}
