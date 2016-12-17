
#include "earley.h"

#include <algorithm>

using namespace std;

struct EarleyState {
    size_t initial;
    size_t current;
    SymTok type;
    LabTok der_lab;
    const vector< SymTok > *der;
    vector< EarleyState* > children;

    bool operator==(const EarleyState &x) const {
        return this->initial == x.initial && this->current == x.current && this->type == x.type && this->der_lab == x.der_lab && this->der == x.der && this->children == x.children;
    }
};

// See http://loup-vaillant.fr/tutorials/earley-parsing/recogniser
// FIXME This version does not support empty derivations
bool earley(const std::vector<SymTok> &sent, SymTok type, const std::unordered_map<SymTok, std::vector<std::pair< LabTok, std::vector<SymTok> > > > &derivations)
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
            EarleyState &cur_state = state[i][j];

            if (cur_state.current == cur_state.der->size()) {
                // If the item is finished, do the completion
                for (size_t k = 0; k < state[cur_state.initial].size(); k++) {
                    EarleyState &cur_state2 = state[cur_state.initial][k];
                    if (cur_state2.current < cur_state2.der->size() && (*cur_state2.der)[cur_state2.current] == cur_state.type) {
                        EarleyState new_item = { cur_state2.initial, cur_state2.current+1, cur_state2.type, cur_state2.der_lab, cur_state2.der, cur_state2.children };
                        new_item.children.push_back(&cur_state);
                        state[i].push_back(new_item);
                    }
                }
            } else {
                // Else search current symbol in derivations
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
                        state[i+1].push_back(new_item);
                    }
                }
            }
        }
    }

    // Final phase: see if the sentence was accepted
    for (size_t j = 0; j < state[sent.size()].size(); j++) {
        EarleyState &cur_state = state[sent.size()][j];
        if (cur_state.initial == 0 && cur_state.current == cur_state.der->size() && cur_state.type == type) {
            return true;
        }
    }
    return false;
}
