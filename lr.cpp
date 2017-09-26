
#include <set>
#include <map>
#include <queue>

#include "lr.h"

using namespace std;

// The state encodes (producting symbol, rule name, position, producted sentence)
template< typename SymType, typename LabType >
using LRState = set< tuple< SymType, LabType, size_t, vector< SymType > > >;

template< typename SymType, typename LabType >
static LRState< SymType, LabType > read_symbol(const LRState< SymType, LabType > &state, SymType sym, const std::unordered_map<SymType, std::vector<std::pair<LabType, vector< SymType > > > > &derivations) {
    LRState< SymType, LabType > new_state;
    set< SymType > next_syms;

    // Read sym through each of the tuples
    for (auto i : state) {
        const auto &head_sym = get<0>(i);
        const auto &rule = get<1>(i);
        const size_t &pos = get<2>(i);
        const Sentence &sent = get<3>(i);
        if (pos < sent.size() && sent[pos] == sym) {
            new_state.insert(make_tuple(head_sym, rule, pos+1, sent));
            if (pos+1 < sent.size()) {
                next_syms.insert(sent[pos+1]);
            }
        }
    }

    // Saturate with fresh derivations
    for (const auto &next_sym : next_syms) {
        auto it = derivations.find(next_sym);
        if (it != derivations.end()) {
            for (auto j : it->second) {
                const auto &lab = j.first;
                const auto &sent = j.second;
                new_state.insert(make_tuple(next_sym, lab, 0, sent));
            }
        }
    }

    return new_state;
}

template< typename SymType, typename LabType >
static set< SymType > next_possible_symbols(const LRState< SymType, LabType > &state) {
    set< SymType > res;
    for (auto i : state) {
        const size_t &pos = get<2>(i);
        const Sentence &sent = get<3>(i);
        if (pos < sent.size()) {
            res.insert(sent[pos]);
        }
    }
    return res;
}

template< typename SymType, typename LabType >
void process_derivations(const std::unordered_map<SymType, std::vector<std::pair<LabType, vector< SymType > > > > &derivations)
{
    size_t num_states = 0;
    map< LRState< SymType, LabType >, size_t > states;
    queue< LRState< SymType, LabType > > new_states;

    // Construct initial state and push it to the queue of new states
    LRState< SymType, LabType > initial;
    for (auto i : derivations) {
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
        tie(ignore, res) = states.insert(make_pair(state, num_states));
        // Proceed only if new state is actually new
        if (res) {
            num_states++;
            set< SymType > next_syms = next_possible_symbols(state);
            for (const auto &sym : next_syms) {
                new_states.push(read_symbol(state, sym, derivations));
            }
        }
    }
}
