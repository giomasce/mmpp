#ifndef LR_H
#define LR_H

#include <vector>
#include <unordered_map>
#include <utility>
#include <set>
#include <map>
#include <queue>
#include <iostream>
#include <functional>
#include <memory>

#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>

#include "parser.h"
#include "serialize_tuple.h"

#define LR_PARSER_AUTO_TEST

// The state encodes (producting symbol, rule name, position, producted sentence)
template< typename SymType, typename LabType >
using LRState = std::set< std::tuple< SymType, LabType, size_t, std::vector< SymType > > >;

template< typename SymType >
std::ostream &default_sym_printer(std::ostream &os, SymType sym) {
    return os << sym;
}

template< typename LabType >
std::ostream &default_lab_printer(std::ostream &os, LabType sym) {
    return os << sym;
}

template< typename SymType, typename LabType >
static LRState< SymType, LabType > evolve_state(const LRState< SymType, LabType > &state, SymType sym, const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector< SymType > > > > &derivations) {
    LRState< SymType, LabType > new_state;
    std::queue< SymType > next_syms_queue;
    std::set< SymType > next_syms;

    // Read sym through each of the std::tuples
    for (auto i : state) {
        const auto &head_sym = std::get<0>(i);
        const auto &rule = std::get<1>(i);
        const size_t &pos = std::get<2>(i);
        const std::vector< SymType > &sent = std::get<3>(i);
        if (pos < sent.size() && sent[pos] == sym) {
            new_state.insert(make_tuple(head_sym, rule, pos+1, sent));
            if (pos+1 < sent.size()) {
                next_syms_queue.push(sent[pos+1]);
            }
        }
    }

    // Saturate with fresh derivations
    while (!next_syms_queue.empty()) {
        auto next_sym = next_syms_queue.front();
        next_syms_queue.pop();
        bool res;
        std::tie(std::ignore, res) = next_syms.insert(next_sym);
        if (res) {
            auto it = derivations.find(next_sym);
            if (it != derivations.end()) {
                for (auto j : it->second) {
                    const auto &lab = j.first;
                    const auto &sent = j.second;
                    new_state.insert(make_tuple(next_sym, lab, 0, sent));
                    if (sent.size() > 0) {
                        next_syms_queue.push(sent[0]);
                    }
                }
            }
        }
    }

    return new_state;
}

template< typename SymType, typename LabType >
static std::set< SymType > next_possible_symbols(const LRState< SymType, LabType > &state) {
    std::set< SymType > res;
    for (auto i : state) {
        const size_t &pos = std::get<2>(i);
        const std::vector< SymType > &sent = std::get<3>(i);
        if (pos < sent.size()) {
            res.insert(sent[pos]);
        }
    }
    return res;
}

template< typename SymType, typename LabType >
static void print_state(const LRState< SymType, LabType > &state,
                 const std::function< std::ostream&(std::ostream&, SymType) > &sym_printer = default_sym_printer< SymType >,
                 const std::function< std::ostream&(std::ostream&, LabType) > &lab_printer = default_lab_printer< LabType >) {
    for (auto i : state) {
        sym_printer(lab_printer(std::cout  << "(", std::get<1>(i)) << ") ", std::get<0>(i)) << ":";
        for (size_t j = 0; j < std::get<3>(i).size(); j++) {
            if (j == std::get<2>(i)) {
                std::cout << " •";
            }
            sym_printer(std::cout << " ", std::get<3>(i)[j]);
        }
        if (std::get<3>(i).size() == std::get<2>(i)) {
            std::cout << " •";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

template< typename SymType, typename LabType >
static std::pair< size_t, size_t > count_rules(const LRState< SymType, LabType > &state, const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector< SymType > > > > &derivations) {
    size_t shift_num = 0;
    size_t reduce_num = 0;
    for (auto i : state) {
        const size_t &pos = std::get<2>(i);
        const std::vector< SymType > &sent = std::get<3>(i);
        if (pos == sent.size()) {
            // If the position is at the end of the sentence, then we should reduce
            reduce_num++;
        } else {
            // If the current symbol is a terminal, then we should shift
            if (derivations.find(sent[pos]) == derivations.end()) {
                shift_num++;
            }
        }
    }
    return std::make_pair(shift_num, reduce_num);
}

template< typename SymType, typename LabType >
class LRParsingHelper {
public:
    LRParsingHelper(const std::unordered_map< size_t, std::pair< std::unordered_map< SymType, size_t >, std::vector< std::tuple< SymType, LabType, size_t, size_t > > > > &automaton,
                    const std::vector<SymType> &sent, SymType type) :
    automaton(automaton), sent(sent), type(type) {
        this->state_stack.push_back(0);
        this->sent_it = this->sent.begin();
    }

    std::pair< bool, ParsingTree< LabType > > do_parsing() {
        const auto &row = this->automaton.at(this->state_stack.back());
        const auto &shifts = row.first;
        const auto &reductions = row.second;

        // Try to perform a shift
        if (this->sent_it != sent.end()) {
            auto shift = shifts.find(*this->sent_it);
            if (shift != shifts.end()) {
                this->state_stack.push_back(shift->second);
                this->sent_it++;

                bool res;
                ParsingTree< LabType > parsing_tree;
                std::tie(res, parsing_tree) = this->do_parsing();
                if (res) {
                    return std::make_pair(res, parsing_tree);
                }

                this->sent_it--;
                this->state_stack.pop_back();
            }
        }

        // Try to perform each reduction
        for (const auto &reduction : reductions) {
            SymType head_sym;
            LabType lab;
            size_t sym_num;
            size_t var_num;
            std::tie(head_sym, lab, sym_num, var_num) = reduction;

            ParsingTree< LabType > new_parsing_tree;
            new_parsing_tree.label = lab;
            std::copy(this->parsing_tree_stack.end() - var_num, this->parsing_tree_stack.end(), std::back_inserter(new_parsing_tree.children));
            this->parsing_tree_stack.resize(this->parsing_tree_stack.size() - var_num);
            this->parsing_tree_stack.push_back(new_parsing_tree);

            // Detect if the search has terminated
            if (this->sent_it == this->sent.end() && this->parsing_tree_stack.size() == 1 && this->state_stack.size() == 1 + sym_num && head_sym == this->type) {
                return std::make_pair(true, new_parsing_tree);
            }

            std::vector< size_t > temp_states;
            std::copy(this->state_stack.end() - sym_num, this->state_stack.end(), std::back_inserter(temp_states));
            this->state_stack.resize(this->state_stack.size() - sym_num);
            const auto &new_row = this->automaton.at(this->state_stack.back());
            this->state_stack.push_back(new_row.first.at(head_sym));

            bool res;
            ParsingTree< LabType > parsing_tree;
            std::tie(res, parsing_tree) = this->do_parsing();
            if (res) {
                return std::make_pair(res, parsing_tree);
            }

            this->state_stack.pop_back();
            std::copy(temp_states.begin(), temp_states.end(), std::back_inserter(this->state_stack));
            this->parsing_tree_stack.pop_back();
            std::copy(new_parsing_tree.children.begin(), new_parsing_tree.children.end(), std::back_inserter(this->parsing_tree_stack));
        }

        return std::pair< bool, ParsingTree< LabType > >(false, {});
    }

private:
    const std::unordered_map< size_t, std::pair< std::unordered_map< SymType, size_t >, std::vector< std::tuple< SymType, LabType, size_t, size_t > > > > &automaton;
    const std::vector<SymType> &sent;
    const SymType type;

    typename std::vector<SymType>::const_iterator sent_it;
    std::vector< size_t > state_stack;
    std::vector< ParsingTree< LabType > > parsing_tree_stack;
};

/*inline void from_string(unsigned short &x, const std::string &s) {
    x = stoul(s);
}

inline void from_string(unsigned char &x, const std::string &s) {
    x = stoul(s);
}

inline void from_string(unsigned &x, const std::string &s) {
    x = stoul(s);
}

inline void from_string(unsigned long &x, const std::string &s) {
    x = stoul(s);
}

inline void from_string(unsigned long long &x, const std::string &s) {
    x = stoull(s);
}

inline void from_string(char &x, const std::string &s) {
    x = stoi(s);
}

inline void from_string(short &x, const std::string &s) {
    x = stoi(s);
}

inline void from_string(int &x, const std::string &s) {
    x = stoi(s);
}

inline void from_string(long &x, const std::string &s) {
    x = stol(s);
}

inline void from_string(long long &x, const std::string &s) {
    x = stoll(s);
}

inline void from_string(std::string &x, const std::string &s) {
    x = s;
}*/

template< typename SymType, typename LabType >
class LRParser : public Parser< SymType, LabType > {
public:
    LRParser(const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations,
             const std::function< std::ostream&(std::ostream&, SymType) > &sym_printer = default_sym_printer< SymType >,
             const std::function< std::ostream&(std::ostream&, LabType) > &lab_printer = default_lab_printer< LabType >) :
        derivations(derivations), sym_printer(sym_printer), lab_printer(lab_printer) {
        //this->initialize();
#ifdef LR_PARSER_AUTO_TEST
        this->ders_by_lab = compute_derivations_by_label(derivations);
#endif
    }

    template< class Archive >
    void to_archive(Archive &a, const std::string &digest) {
        a << digest;
        a << this->automaton;
    }

    template< class Archive >
    void from_archive(Archive &a, const std::string &digest) {
        std::string found_digest;
        a >> found_digest;
        assert(digest == found_digest);
        a >> this->automaton;
    }

    ParsingTree< LabType > parse(const std::vector<SymType> &sent, SymType type) const {
        LRParsingHelper helper(this->automaton, sent, type);
        bool res;
        ParsingTree< LabType > parsing_tree;
        std::tie(res, parsing_tree) = helper.do_parsing();
        if (res) {
#ifdef LR_PARSER_AUTO_TEST
            // Check that the returned parsing tree is correct
            assert(reconstruct_sentence(parsing_tree, this->derivations, this->ders_by_lab) == sent);
#endif
            return parsing_tree;
        } else {
            return {};
        }
    }

    void initialize() {
        size_t num_states = 0;
        std::map< LRState< SymType, LabType >, std::shared_ptr< std::pair< size_t, std::map< SymType, size_t > > > > states;
        std::set< LRState< SymType, LabType > > processed_states;
        std::queue< LRState< SymType, LabType > > new_states;

        auto get_state_data = [&](const LRState< SymType, LabType > &state)->std::shared_ptr< std::pair< size_t, std::map< SymType, size_t > > > {
            bool res;
            typename decltype(states)::iterator it;
            std::tie(it, res) = states.insert(std::make_pair(state, std::make_shared< std::pair< size_t, std::map< SymType, size_t > > >(num_states, std::map< SymType, size_t >())));
            if (res) {
                num_states++;
            }
            return it->second;
        };

        // Construct initial state and push it to the std::queue of new states
        LRState< SymType, LabType > initial;
        for (auto i : this->derivations) {
            auto sym = i.first;
            for (auto j : i.second) {
                auto lab = j.first;
                auto sent = j.second;
                initial.insert(make_tuple(sym, lab, 0, sent));
            }
        }
        new_states.push(initial);

        // Iteratively consider each new state and all its possible evolutions
        while (!new_states.empty()) {
            LRState< SymType, LabType > state = new_states.front();
            new_states.pop();
            bool res;
            std::tie(std::ignore, res) = processed_states.insert(state);
            // Proceed only if new state is actually new
            if (res) {
                auto state_data = get_state_data(state);
                const size_t state_idx = state_data->first;
                std::cout << state_idx << std::endl;
                //print_state(state, sym_printer, lab_printer);

                // Build the shift map and enqueue new states
                std::unordered_map< SymType, size_t > shifts;
                std::set< SymType > next_syms = next_possible_symbols(state);
                for (const auto &sym : next_syms) {
                    auto new_state = evolve_state(state, sym, derivations);
                    const size_t new_state_idx = get_state_data(new_state)->first;
                    shifts.insert(std::make_pair(sym, new_state_idx));
                    new_states.push(new_state);
                }

                // Build the vector of reductions
                std::vector< std::tuple< SymType, LabType, size_t, size_t > > reductions;
                for (auto rule : state) {
                    const SymType &head_sym = std::get<0>(rule);
                    const LabType &lab = std::get<1>(rule);
                    const size_t &pos = std::get<2>(rule);
                    const std::vector< SymType > &sent = std::get<3>(rule);
                    size_t sym_num = sent.size();
                    size_t var_num = 0;
                    for (const auto &sym : sent) {
                        if (this->derivations.find(sym) != this->derivations.end()) {
                            var_num++;
                        }
                    }
                    if (pos == sent.size()) {
                        reductions.push_back(std::make_tuple(head_sym, lab, sym_num, var_num));
                    }
                }

                // Insert information in the automaton
                this->automaton[state_idx] = make_pair(shifts, reductions);
            }
        }

        // Look again at all states to list conflicts
        /*for (const auto &it : states) {
            const auto &state = it.first;
            size_t shift_num;
            size_t reduce_num;
            std::tie(shift_num, reduce_num) = count_rules(state, this->derivations);
            if (reduce_num > 1 || (reduce_num > 0 && shift_num > 0)) {
                std::cout << "State " << it.second->first << " is conflicting!" << std::endl;
                print_state(state, this->sym_printer, this->lab_printer);
                std::cout << std::endl;
            }
        }*/
    }

private:
    const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations;
    /* Every state is mapped to a pair containing the shift map and the vector of reductions;
     * each reduction is described by its head symbol, its label, its number of symbols and its number of variables. */
    std::unordered_map< size_t, std::pair< std::unordered_map< SymType, size_t >, std::vector< std::tuple< SymType, LabType, size_t, size_t > > > > automaton;
    const std::function< std::ostream&(std::ostream&, SymType) > sym_printer;
    const std::function< std::ostream&(std::ostream&, LabType) > lab_printer;

#ifdef LR_PARSER_AUTO_TEST
    std::unordered_map< LabType, std::pair< SymType, std::vector< SymType > > > ders_by_lab;
#endif
};

#endif // LR_H
