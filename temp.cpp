
#include <unordered_map>
#include <utility>
#include <iostream>
#include <functional>

#include "test.h"

#include "toolbox.h"
#include "unif.h"

using namespace std;

//#pragma GCC push_options
//#pragma GCC optimize ("O0")

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > > /*__attribute__((optimize("O0")))*/ LibraryToolbox::unify_assertion_uncached3(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first, bool up_to_hyps_perms) const
{
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > ret;

    // Parse inputs
    vector< ParsingTree< SymTok, LabTok > > pt_hyps;
    for (auto &hyp : hypotheses) {
        pt_hyps.push_back(this->parse_sentence(hyp.begin()+1, hyp.end(), this->turnstile_alias));
    }
    ParsingTree< SymTok, LabTok > pt_thesis = this->parse_sentence(thesis.begin()+1, thesis.end(), this->turnstile_alias);

    auto assertions_gen = this->lib.list_assertions();
    const std::function< bool(LabTok) > is_var = [&](LabTok x)->bool {
        const auto &types_set = this->lib.get_final_stack_frame().types_set;
        if (types_set.find(x) == types_set.end()) {
            return false;
        }
        return !this->lib.is_constant(this->lib.get_sentence(x).at(1));
    };
    while (true) {
        const Assertion *ass2 = assertions_gen();
        if (ass2 == NULL) {
            break;
        }
        const Assertion &ass = *ass2;
        if (ass.is_usage_disc()) {
            continue;
        }
        if (ass.get_ess_hyps().size() != hypotheses.size()) {
            continue;
        }
        if (thesis[0] != this->lib.get_sentence(ass.get_thesis())[0]) {
            continue;
        }
        std::unordered_map< LabTok, ParsingTree< SymTok, LabTok > > thesis_subst;
        auto &templ_pt = this->get_parsed_sents().at(ass.get_thesis());
        bool res = unify(templ_pt, pt_thesis, is_var, thesis_subst);
        if (!res) {
            continue;
        }
        // We have to generate all the hypotheses' permutations; fortunately usually hypotheses are not many
        // TODO Is there a better algorithm?
        // The i-th specified hypothesis is matched with the perm[i]-th assertion hypothesis
        vector< size_t > perm;
        for (size_t i = 0; i < hypotheses.size(); i++) {
            perm.push_back(i);
        }
        do {
            std::unordered_map< LabTok, ParsingTree< SymTok, LabTok > > subst = thesis_subst;
            res = true;
            for (size_t i = 0; i < hypotheses.size(); i++) {
                res = (hypotheses[i][0] == this->lib.get_sentence(ass.get_ess_hyps()[perm[i]])[0]);
                if (!res) {
                    break;
                }
                auto &templ_pt = this->get_parsed_sents().at(ass.get_ess_hyps()[perm[i]]);
                res = unify(templ_pt, pt_hyps[i], is_var, subst);
                if (!res) {
                    break;
                }
            }
            if (!res) {
                continue;
            }
            std::unordered_map< SymTok, std::vector< SymTok > > subst2;
            for (auto &s : subst) {
                subst2.insert(make_pair(this->lib.get_sentence(s.first).at(1), this->reconstruct_sentence(s.second)));
            }
            ret.emplace_back(ass.get_thesis(), perm, subst2);
            if (just_first) {
                return ret;
            }
            if (!up_to_hyps_perms) {
                break;
            }
        } while (next_permutation(perm.begin(), perm.end()));
    }

    return ret;
}

//#pragma GCC pop_options
