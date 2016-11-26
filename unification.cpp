#include "unification.h"

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cassert>

// I do not want this to apply to cassert
#define NDEBUG

using namespace std;

// This algorithm is probably not terribly efficient
static void unify_internal(vector<SymTok>::const_iterator sent_cur, vector<SymTok>::const_iterator sent_end,
                           vector<SymTok>::const_iterator templ_cur, vector<SymTok>::const_iterator templ_end,
                           const Library &lib, bool allow_empty, unordered_map< SymTok, vector< SymTok > > &current_match,
                           vector< unordered_map< SymTok, vector< SymTok > > > &matches) {
#ifndef NDEBUG
    cerr << "Entering unify_internal(" << (sent_cur != sent_end ? *sent_cur : -1) <<", .., " << (templ_cur != templ_end ? *templ_cur : -1) << ", .., .., " << allow_empty << ", .., ..)" << endl;
#endif
    if (templ_cur == templ_end) {
#ifndef NDEBUG
        cerr << "Reached template end" << endl;
#endif
        if (sent_cur == sent_end) {
#ifndef NDEBUG
            cerr << "Found match" << endl;
#endif
            matches.push_back(current_match);
        }
    } else {
        // Process a new token from the template
        const SymTok &cur_tok = *templ_cur;
        if (lib.is_constant(cur_tok) || cur_tok == 0) {
            // Easy case: the token is a constant
            if (cur_tok == *sent_cur) {
#ifndef NDEBUG
                cerr << "Token is a constant, which matched" << endl;
#endif
                unify_internal(sent_cur+1, sent_end, templ_cur+1, templ_end, lib, allow_empty, current_match, matches);
            } else {
#ifndef NDEBUG
                cerr << "Token is a constant, which did not match" << endl;
#endif
            }
        } else {
            // Bad case: the token is a variable
            auto subs = current_match.find(cur_tok);
            if (subs == current_match.end()) {
                // Worst case: the variable has not been bound yet, so we have to spawn all possible bindings
#ifndef NDEBUG
                cerr << "Token is a new variable" << endl;
#endif
                vector< SymTok > match;
                for (auto i = 0; i <= distance(sent_cur, sent_end); i++) {
                    if (i > 0 || allow_empty) {
                        auto this_it = current_match.insert(make_pair(cur_tok, match));
                        assert(this_it.second);
                        unify_internal(sent_cur+i, sent_end, templ_cur+1, templ_end, lib, allow_empty, current_match, matches);
                        current_match.erase(this_it.first);
                    }
                    if (i < distance(sent_cur, sent_end)) {
                        match.push_back(*(sent_cur+i));
                    }
                }
            } else {
                // Not-so-bad case: the variable has already been bound, wo we just have to check the binding
                vector< SymTok > &match = subs->second;
                auto len = match.size();
                if (distance(sent_cur, sent_end) >= (unsigned int) len && equal(match.begin(), match.end(), sent_cur)) {
#ifndef NDEBUG
                    cerr << "Token is an old variable, which matched" << endl;
#endif
                    unify_internal(sent_cur+len, sent_end, templ_cur+1, templ_end, lib, allow_empty, current_match, matches);
                } else {
#ifndef NDEBUG
                    cerr << "Token is an old variable, which did not match" << endl;
#endif
                }
            }
        }
    }
}

std::vector<std::unordered_map<SymTok, std::vector<SymTok> > > unify(const std::vector<SymTok> &sent, const std::vector<SymTok> &templ, const Library &lib, bool allow_empty)
{
    vector< unordered_map< SymTok, vector< SymTok > > > matches;
    unordered_map< SymTok, vector< SymTok > > current_match;
    unify_internal(sent.begin(), sent.end(), templ.begin(), templ.end(), lib, allow_empty, current_match, matches);
    return matches;
}
