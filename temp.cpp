
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
//vector< string > steps = { "3bitr4i", "df-clel", "sbbii", "df-clel", "bitri", "exbii", "bitri", "anbi12i", "sbf", "nfcri", "*", "3bitr4i", "abeq2", "sbbii", "abeq2", "sbalv", "sbrbis", "sbf", "nfv", "sban", "sbex" };
vector< string > steps = { "elun", "simpl1", "simpl2", "*", "elixx1", "syl2anc", "biimpa", "simp1d", "simpl1", "simpl2", "*", "elixx1", "syl2anc", "biimpa",
                           "simp2d", "simpl1", "simpl2", "*", "elixx1", "syl2anc", "biimpa", "simp3d", "simplrr", "simpl1", "simpl2", "*", "elixx1", "syl2anc",
                           "biimpa", "simp1d", "simpl2", "adantr", "simpl3", "adantr", "*", "syl3anc", "mp2and", "3jca", "simpl2", "simpl3", "*", "elixx1",
                           "syl2anc", "biimpa", "simp1d", "simplrl", "simpl2", "simpl3", "*", "elixx1", "syl2anc", "biimpa", "simp2d", "simpl1", "adantr",
                           "simpl2", "adantr", "simpl2", "simpl3", "*", "elixx1", "syl2anc", "biimpa", "simp1d", "*", "syl3anc", "mp2and", "simpl2", "simpl3",
                           "*", "elixx1", "syl2anc", "biimpa", "simp3d", "3jca", "jaodan", "simpl1", "simpl3", "*", "elixx1", "syl2anc", "biimpar", "syldan",
                           "exmid", "simpl1", "simpl2", "*", "elixx1", "syl2anc", "df-3an", "syl6bb", "adantr", "simpl1", "simpl3", "*", "elixx1", "syl2anc",
                           "biimpa", "simp1d", "simpl1", "simpl3", "*", "elixx1", "syl2anc", "biimpa", "simp2d", "jca", "biantrurd", "bitr4d", "simpl2",
                           "simpl3", "*", "elixx1", "syl2anc", "3anan12", "syl6bb", "adantr", "simpl1", "simpl3", "*", "elixx1", "syl2anc", "biimpa", "simp1d",
                           "simpl1", "simpl3", "*", "elixx1", "syl2anc", "biimpa", "simp3d", "jca", "biantrud", "simpl2", "adantr", "simpl1", "simpl3", "*",
                           "elixx1", "syl2anc", "biimpa", "simp1d", "*", "syl2anc", "3bitr2d", "orbi12d", "mpbiri", "impbida", "syl5bb", "eqrdv", };

class Reactor {
public:
    Reactor(LibraryToolbox &tb) :
        tb(tb), is_var(tb.get_standard_is_var())
    {
    }

    bool process_hypothesis(SymTok type_sym) {
        LabTok type;
        tie(type, ignore) = tb.new_temp_var(type_sym);
        this->hypotheses.push_back(type);
        ParsingTree< SymTok, LabTok > pt;
        pt.label = type;
        pt.type = type_sym;
        this->stack.push_back(pt);
        return true;
    }

    bool process_sentence(const Sentence &sent) {
        return this->process_parsing_tree(tb.parse_sentence(sent));
    }

    bool process_parsing_tree(const ParsingTree< SymTok, LabTok > &pt) {
        this->stack.push_back(tb.refresh_parsing_tree(pt));
        return true;
    }

    bool process_assertion(const Assertion &ass) {
        ParsingTree< SymTok, LabTok > thesis;
        vector< ParsingTree< SymTok, LabTok > > hyps;
        tie(hyps, thesis) = tb.refresh_assertion(ass);
        assert(stack.size() >= hyps.size());
        for (size_t i = 0; i < hyps.size(); i++) {
            bool res = unify2(this->stack[this->stack.size()-hyps.size()+i], hyps[i], this->is_var, this->subst);
            if (!res) {
                return false;
            }
        }
        this->stack.resize(this->stack.size() - hyps.size());
        this->stack.push_back(thesis);
        return true;
    }

    bool process_label(LabTok lab) {
        return this->process_assertion(tb.get_assertion(lab));
    }

    ParsingTree< SymTok, LabTok > get_theorem() {
        assert(this->stack.size() == 1);
        return substitute(this->stack.at(0), this->is_var, this->subst);
    }

    vector< ParsingTree< SymTok, LabTok > > get_hypotheses() {
        vector< ParsingTree< SymTok, LabTok > > ret;
        for (const auto &hyp : this->hypotheses) {
            ret.push_back(subst.at(hyp));
        }
        return ret;
    }

private:
    LibraryToolbox &tb;
    const std::function< bool(LabTok) > is_var;
    vector< ParsingTree< SymTok, LabTok > > stack;
    vector< LabTok > hypotheses;
    SubstMap< SymTok, LabTok > subst;
};

void inverse_unification_reactor(const ParsingTree< SymTok, LabTok > &parent, vector< string >::const_reverse_iterator &it, LibraryToolbox &tb, SubstMap< SymTok, LabTok > &subst) {
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
        inverse_unification_reactor(hyp, it, tb, subst);
    }
}

void test_unification2() {
    auto &data = get_set_mm();
    auto &tb = data.tb;

    /*{
        auto &ass = tb.get_assertion(tb.get_label("ixxun"));
        auto pe = ass.get_proof_executor(tb);
        UncompressedProof proof = pe->uncompress();
        auto labs = proof.get_labels();
        cout << "Proof vector: {";
        for (auto label : labs) {
            auto ess_hyps = ass.get_ess_hyps();
            if (find(ess_hyps.begin(), ess_hyps.end(), label) != ess_hyps.end()) {
                cout << " \"*\",";
            } else if (tb.get_assertion(label).is_valid() && tb.get_sentence(label).at(0) == tb.get_turnstile()) {
                cout << " \"" << tb.resolve_label(label) << "\",";
            }
        }
        cout << " }" << endl;
        //cout << tb.print_proof(labs, true) << endl;
    }*/

    cout << "FORWARD UNIFICATION TEST" << endl;
    Reactor reactor(tb);
    for (auto it = steps.begin(); it != steps.end(); it++) {
        string &step = *it;
        if (step == "*") {
            bool res = reactor.process_hypothesis(tb.get_symbol("wff"));
            assert(res);
        } else {
            bool res = reactor.process_label(tb.get_label(step));
            assert(res);
        }
    }
    cout << "Proved theorem: " << tb.print_sentence(reactor.get_theorem()) << endl;
    cout << "with hypotheses:" << endl;
    for (const auto &hyp : reactor.get_hypotheses()) {
        cout << " * " << tb.print_sentence(hyp) << endl;
    }

    /*cout << endl;

    cout << "INVERSE UNIFICATION TEST" << endl;
    ParsingTree< SymTok, LabTok > master = tb.parse_sentence(tb.read_sentence("ph"), tb.get_symbol("wff"));
    SubstMap< SymTok, LabTok > subst;
    auto it = steps.crbegin();
    inverse_unification_reactor(master, it, tb, subst);
    cout << "Final substitution map:" << endl;
    for (auto &i : subst) {
        cout << tb.resolve_symbol(tb.get_sentence(i.first).at(1)) << ": " << tb.print_sentence(i.second) << endl;
    }*/

    /*for (auto &hyp : hyps) {
        cout << "  * " << tb.print_sentence(hyp) << endl;
    }
    cout << " => " << tb.print_sentence(thesis) << endl;*/
}

//#pragma GCC pop_options
