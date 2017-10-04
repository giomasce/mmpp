
#include <unordered_map>
#include <utility>
#include <iostream>
#include <functional>
#include <vector>
#include <string>

#include "test.h"

#include "toolbox.h"
#include "unif.h"

using namespace std;

//#pragma GCC push_options
//#pragma GCC optimize ("O0")

//std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > > /*__attribute__((optimize("O0")))*/ LibraryToolbox::unify_assertion_uncached3(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first, bool up_to_hyps_perms) const

//vector< string > steps = { "3imtr4g", "df-clel", "df-clel", "eximdv", "anim2d", "19.21bi", "biimpi", "dfss2" };
vector< string > steps = { "3bitr4i", "df-clel", "sbbii", "df-clel", "bitri", "exbii", "bitri", "anbi12i", "sbf", "nfcri", "*", "3bitr4i", "abeq2", "sbbii", "abeq2", "sbalv", "sbrbis", "sbf", "nfv", "sban", "sbex" };

void unification_reactor(const ParsingTree< SymTok, LabTok > &parent, vector< string >::const_iterator &it, LibraryToolbox &tb, SubstMap< SymTok, LabTok > &subst) {
    string step = *it;
    it++;
    if (step == "*") {
        return;
    }
    LabTok lab = tb.get_label(step);
    const Assertion &ass = tb.get_assertion(lab);
    ParsingTree< SymTok, LabTok > thesis;
    vector< ParsingTree< SymTok, LabTok > > hyps;
    tie(hyps, thesis) = tb.refresh_assertion(ass);
    auto is_var = tb.get_standard_is_var();
    bool res = unify2(thesis, parent, is_var, subst);
    assert(res);
    for (size_t i = hyps.size(); i > 0; i--) {
        const auto &hyp = hyps[i-1];
        unification_reactor(hyp, it, tb, subst);
    }
}

void test_unification2() {
    auto &data = get_set_mm();
    auto &tb = data.tb;

    ParsingTree< SymTok, LabTok > master = tb.parse_sentence(tb.read_sentence("ph"), tb.get_symbol("wff"));
    SubstMap< SymTok, LabTok > subst;
    auto it = steps.cbegin();
    unification_reactor(master, it, tb, subst);
    cout << "Final substitution map:" << endl;
    for (auto &i : subst) {
        cout << tb.resolve_symbol(tb.get_sentence(i.first).at(1)) << ": " << tb.print_sentence(i.second) << endl;
    }

    /*for (auto &hyp : hyps) {
        cout << "  * " << tb.print_sentence(hyp) << endl;
    }
    cout << " => " << tb.print_sentence(thesis) << endl;*/
}

//#pragma GCC pop_options
