
#include "earley.h"

#include <algorithm>
#include <iostream>

#define NDEBUG

using namespace std;

struct EarleyState {
    size_t initial;
    size_t current;
    SymTok type;
    LabTok der_lab;
    const vector< SymTok > *der;
    vector< pair< size_t, size_t > > children;

    bool operator==(const EarleyState &x) const {
        return this->initial == x.initial && this->current == x.current && this->type == x.type && this->der_lab == x.der_lab && this->der == x.der;
    }

    EarleyTreeItem get_tree(const vector< vector< EarleyState > > &state) const {
        EarleyTreeItem ret;
        ret.label = this->der_lab;
        for (auto &child : this->children) {
            ret.children.push_back(state[child.first][child.second].get_tree(state));
        }
        return ret;
    }
};

// See http://loup-vaillant.fr/tutorials/earley-parsing/recogniser
// FIXME This version does not support empty derivations
EarleyTreeItem earley(const std::vector<SymTok> &sent, SymTok type, const std::unordered_map<SymTok, std::vector<std::pair< LabTok, std::vector<SymTok> > > > &derivations)
{
    /* For every position (plus one end-of-string position) we maintain a list
     * of tuples whose elements are the initial position of that item, the position
     * of the current point and the associated derivation. */
    vector< vector< EarleyState > > state;
    state.resize(sent.size()+1);

    // The state is initialized with the derivations for the target type
    for (auto &der : derivations.at(type)) {
        state[0].push_back({ 0, 0, type, der.first, &der.second, {} });
    }

    // We use vector indices instead of iterators to avoid problems with reallocation
    for (size_t i = 0; i < state.size(); i++) {
        for (size_t j = 0; j < state[i].size(); j++) {

            if (state[i][j].current == state[i][j].der->size()) {
                // If the item is finished, do the completion
                for (size_t k = 0; k < state[state[i][j].initial].size(); k++) {
                    EarleyState &cur_state2 = state[state[i][j].initial][k];
                    if (cur_state2.current < cur_state2.der->size() && (*cur_state2.der)[cur_state2.current] == state[i][j].type) {
                        EarleyState new_item = { cur_state2.initial, cur_state2.current+1, cur_state2.type, cur_state2.der_lab, cur_state2.der, cur_state2.children };
                        new_item.children.push_back(make_pair(i, j));
                        if (find(state[i].begin(), state[i].end(), new_item) == state[i].end()) {
                            state[i].push_back(new_item);
                        }
                    }
                }
            } else {
                // Else search current symbol in derivations
                EarleyState &cur_state = state[i][j];
                SymTok symbol = (*cur_state.der)[cur_state.current];
                auto it = derivations.find(symbol);
                if (it != derivations.end()) {
                    // Current symbol is in the derivations, therefore non-terminal: prediction phase
                    // Add one new item for every derivation, unless the same item is already present
                    for (auto &new_der : it->second) {
                        EarleyState new_item = { i, 0, symbol, new_der.first, &new_der.second, {} };
                        if (find(state[i].begin(), state[i].end(), new_item) == state[i].end()) {
                            state[i].push_back(new_item);
                        }
                    }
                } else {
                    // Current symbol it not in the derivations, therefore is terminal: scan phase
                    // If the symbol matches the sentence, promote item to the new bucket
                    if (symbol == sent[i]) {
                        EarleyState new_item = { cur_state.initial, cur_state.current+1, cur_state.type, cur_state.der_lab, cur_state.der, cur_state.children };
                        if (find(state[i+1].begin(), state[i+1].end(), new_item) == state[i+1].end()) {
                            state[i+1].push_back(new_item);
                        }
                    }
                }
            }
        }
    }

    // Dump final status for debug
#ifndef NDEBUG
    cout << "Earley dump" << endl;
    for (size_t i = 0; i < state.size(); i++) {
        cout << "== " << i << " ==" << endl;
        for (size_t j = 0; j < state[i].size(); j++) {
            EarleyState &cur_state = state[i][j];
            cout << cur_state.der_lab << " (" << cur_state.initial << "): " << cur_state.type << " -->";
            for (size_t k = 0; k < cur_state.der->size(); k++) {
                if (k == cur_state.current) {
                    cout << " x";
                }
                cout << " " << (*cur_state.der)[k];
            }
            cout << endl;
        }
        cout << endl;
    }
#endif

    // Final phase: see if the sentence was accepted
    for (size_t j = 0; j < state[sent.size()].size(); j++) {
        EarleyState &cur_state = state[sent.size()][j];
        if (cur_state.initial == 0 && cur_state.current == cur_state.der->size() && cur_state.type == type) {
            EarleyTreeItem tree = cur_state.get_tree(state);
            return tree;
        }
    }
    EarleyTreeItem ret;
    ret.label = 0;
    return ret;
}

void test_earley() {
    /* Describe the grammar at http://loup-vaillant.fr/tutorials/earley-parsing/recogniser with:
     *   1: Sum
     *   2: Product
     *   3: Factor
     *   4: Number
     *   10: +
     *   11: -
     *   12: *
     *   13: /
     *   14: (
     *   15: )
     *   20: 0
     *   21: 1
     *   ...
     *   29: 9
     * Only digit up to 4 are supported at the moment.
     */
    std::unordered_map<SymTok, std::vector<std::pair< LabTok, std::vector<SymTok> > > > derivations;
    derivations[1].push_back(make_pair(100, vector< SymTok >({ 1, 10, 2 })));
    derivations[1].push_back(make_pair(101, vector< SymTok >({ 1, 11, 2 })));
    derivations[1].push_back(make_pair(102, vector< SymTok >({ 2 })));
    derivations[2].push_back(make_pair(103, vector< SymTok >({ 2, 12, 3 })));
    derivations[2].push_back(make_pair(104, vector< SymTok >({ 2, 13, 3 })));
    derivations[2].push_back(make_pair(105, vector< SymTok >({ 3 })));
    derivations[3].push_back(make_pair(106, vector< SymTok >({ 14, 1, 15 })));
    derivations[3].push_back(make_pair(107, vector< SymTok >({ 4 })));
    derivations[4].push_back(make_pair(108, vector< SymTok >({ 20, 4 })));
    derivations[4].push_back(make_pair(109, vector< SymTok >({ 21, 4 })));
    derivations[4].push_back(make_pair(110, vector< SymTok >({ 22, 4 })));
    derivations[4].push_back(make_pair(111, vector< SymTok >({ 23, 4 })));
    derivations[4].push_back(make_pair(112, vector< SymTok >({ 24, 4 })));
    derivations[4].push_back(make_pair(113, vector< SymTok >({ 20 })));
    derivations[4].push_back(make_pair(114, vector< SymTok >({ 21 })));
    derivations[4].push_back(make_pair(115, vector< SymTok >({ 22 })));
    derivations[4].push_back(make_pair(116, vector< SymTok >({ 23 })));
    derivations[4].push_back(make_pair(117, vector< SymTok >({ 24 })));
    vector< SymTok > sent = { 21, 10, 14, 22, 12, 23, 11, 24, 15 };
    earley(sent, 1, derivations);
}
