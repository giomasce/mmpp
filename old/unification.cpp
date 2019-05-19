#include "unification.h"

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cassert>

#include "mm/setmm_loader.h"
#include "test/test.h"

//#define OLD_UNIFICATION_VERBOSE

// This algorithm is probably not terribly efficient
static void unify_old_internal(std::vector<SymTok>::const_iterator sent_cur, std::vector<SymTok>::const_iterator sent_end,
                           std::vector<SymTok>::const_iterator templ_cur, std::vector<SymTok>::const_iterator templ_end,
                           const Library &lib, bool allow_empty, std::unordered_map< SymTok, std::vector< SymTok > > &current_match,
                           std::vector< std::unordered_map< SymTok, std::vector< SymTok > > > &matches) {
#ifdef OLD_UNIFICATION_VERBOSE
    std::cerr << "Entering unify_internal(" << (sent_cur != sent_end ? *sent_cur : -1) <<", .., " << (templ_cur != templ_end ? *templ_cur : -1) << ", .., .., " << allow_empty << ", .., ..)" << std::endl;
#endif
    if (templ_cur == templ_end) {
#ifdef OLD_UNIFICATION_VERBOSE
        std::cerr << "Reached template end" << std::endl;
#endif
        if (sent_cur == sent_end) {
#ifdef OLD_UNIFICATION_VERBOSE
            std::cerr << "Found match" << std::endl;
#endif
            matches.push_back(current_match);
        }
    } else {
        // Process a new token from the template
        const SymTok &cur_tok = *templ_cur;
        if (lib.is_constant(cur_tok) || cur_tok == SymTok{}) {
            // Easy case: the token is a constant
            if (sent_cur != sent_end && cur_tok == *sent_cur) {
#ifdef OLD_UNIFICATION_VERBOSE
                std::cerr << "Token is a constant, which matched" << std::endl;
#endif
                unify_old_internal(sent_cur+1, sent_end, templ_cur+1, templ_end, lib, allow_empty, current_match, matches);
            } else {
#ifdef OLD_UNIFICATION_VERBOSE
                std::cerr << "Token is a constant, which did not match" << std::endl;
#endif
            }
        } else {
            // Bad case: the token is a variable
            auto subs = current_match.find(cur_tok);
            if (subs == current_match.end()) {
                // Worst case: the variable has not been bound yet, so we have to spawn all possible bindings
#ifdef OLD_UNIFICATION_VERBOSE
                std::cerr << "Token is a new variable" << std::endl;
#endif
                std::vector< SymTok > match;
                for (auto i = 0; i <= distance(sent_cur, sent_end); i++) {
                    if (i > 0 || allow_empty) {
                        auto this_it = current_match.insert(make_pair(cur_tok, match));
                        assert(this_it.second);
                        unify_old_internal(sent_cur+i, sent_end, templ_cur+1, templ_end, lib, allow_empty, current_match, matches);
                        current_match.erase(this_it.first);
                    }
                    if (i < distance(sent_cur, sent_end)) {
                        match.push_back(*(sent_cur+i));
                    }
                }
            } else {
                // Not-so-bad case: the variable has already been bound, wo we just have to check the binding
                std::vector< SymTok > &match = subs->second;
                auto len = match.size();
                if (distance(sent_cur, sent_end) >= (unsigned int) len && equal(match.begin(), match.end(), sent_cur)) {
#ifdef OLD_UNIFICATION_VERBOSE
                    std::cerr << "Token is an old variable, which matched" << std::endl;
#endif
                    unify_old_internal(sent_cur+len, sent_end, templ_cur+1, templ_end, lib, allow_empty, current_match, matches);
                } else {
#ifdef OLD_UNIFICATION_VERBOSE
                    std::cerr << "Token is an old variable, which did not match" << std::endl;
#endif
                }
            }
        }
    }
}

/*
 * Perform a first test to check that the template, without its variables, is a substring of the sentence.
 *
 * This can be used to quickly exclude most uninteresting results, and run the actual (and slow) unification
 * algorithm on much less sentences in the library. After quick measurements, it seems that there is
 * a noticeable (~ 30%) speedup on simple sentences and an order-of-magnitude one on complex sentences.
 */
static bool unify_old_internal_quick(std::vector<SymTok>::const_iterator sent_cur, std::vector<SymTok>::const_iterator sent_end,
                                 std::vector<SymTok>::const_iterator templ_cur, std::vector<SymTok>::const_iterator templ_end,
                                 const Library &lib) {
    while (templ_cur != templ_end) {
        if (!lib.is_constant(*templ_cur)) {
            templ_cur++;
        } else {
            while (true) {
                if (sent_cur == sent_end) {
                    return false;
                } else {
                    if (*sent_cur == *templ_cur) {
                        sent_cur++;
                        templ_cur++;
                        break;
                    } else {
                        sent_cur++;
                    }
                }
            }
        }
    }
    return true;
}

std::vector<std::unordered_map<SymTok, std::vector<SymTok> > > unify_old(const std::vector<SymTok> &sent, const std::vector<SymTok> &templ, const Library &lib, bool allow_empty)
{
    std::vector< std::unordered_map< SymTok, std::vector< SymTok > > > matches;
    std::unordered_map< SymTok, std::vector< SymTok > > current_match;
    if (unify_old_internal_quick(sent.begin(), sent.end(), templ.begin(), templ.end(), lib)) {
        unify_old_internal(sent.begin(), sent.end(), templ.begin(), templ.end(), lib, allow_empty, current_match, matches);
    }
    return matches;
}

#ifdef ENABLE_TEST_CODE
BOOST_AUTO_TEST_CASE(test_old_unification) {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;
    //std::cout << "Generic unification test" << std::endl;
    std::vector< SymTok > sent = tb.read_sentence("wff ( ph -> ( ps -> ch ) )");
    std::vector< SymTok > templ = tb.read_sentence("wff ( th -> et )");
    auto res = unify_old(sent, templ, lib, false);
    BOOST_TEST(res.size() == (size_t) 2);
    /*std::cout << "Matching:         " << tb.print_sentence(sent, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << std::endl << "against template: " << tb.print_sentence(templ, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << std::endl;
    for (auto &match : res) {
        std::cout << "  *";
        for (auto &var: match) {
            std::cout << " " << tb.print_sentence(Sentence({var.first}), SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << " => " << tb.print_sentence(var.second, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << "  ";
        }
        std::cout << std::endl;
    }
    std::cout << "Memory usage after test: " << size_to_string(platform_get_current_used_ram()) << std::endl << std::endl;*/
}
#endif
