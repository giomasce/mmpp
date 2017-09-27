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

#include "parser.h"

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
class LRParser : public Parser< SymType, LabType > {
public:
    LRParser(const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations,
             const std::function< std::ostream&(std::ostream&, SymType) > &sym_printer = default_sym_printer< SymType >,
             const std::function< std::ostream&(std::ostream&, LabType) > &lab_printer = default_lab_printer< LabType >) :
        derivations(derivations), sym_printer(sym_printer), lab_printer(lab_printer) {
        this->process_derivations();
    }

    ParsingTree< LabType > parse(const std::vector<SymType> &sent, SymType type) const {
        return {};
    }

private:
    void process_derivations() {
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
                const size_t &state_idx = state_data->first;
                std::map< SymType, size_t > &state_map = state_data->second;
                std::cout << state_idx << std::endl;
                //print_state(state, sym_printer, lab_printer);
                std::set< SymType > next_syms = next_possible_symbols(state);
                for (const auto &sym : next_syms) {
                    auto new_state = evolve_state(state, sym, derivations);
                    const size_t new_state_idx = get_state_data(new_state)->first;
                    state_map.insert(std::make_pair(sym, new_state_idx));
                    new_states.push(new_state);
                }
            }
        }

        // Look again at all states and check for conflicts
        for (const auto &it : states) {
            const auto &state = it.first;
            size_t shift_num;
            size_t reduce_num;
            std::tie(shift_num, reduce_num) = count_rules(state, this->derivations);
            if (reduce_num > 1 || (reduce_num > 0 && shift_num > 0)) {
                std::cout << "State " << it.second->first << " is conflicting!" << std::endl;
                print_state(state, this->sym_printer, this->lab_printer);
                std::cout << std::endl;
            }
        }
    }

    const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations;
    const std::function< std::ostream&(std::ostream&, SymType) > &sym_printer;
    const std::function< std::ostream&(std::ostream&, LabType) > &lab_printer;
};

#endif // LR_H
