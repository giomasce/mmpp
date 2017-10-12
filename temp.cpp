
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

class Reactor {
public:
    Reactor(LibraryToolbox &tb) :
        tb(tb), is_var(tb.get_standard_is_var()), unificator(this->is_var)
    {
    }

    void process_hypothesis(SymTok type_sym, size_t idx) {
        auto it = this->create_hypothesis(type_sym, idx);
        LabTok type = it->second;
        ParsingTree< SymTok, LabTok > pt;
        pt.label = type;
        pt.type = type_sym;
        this->stack.push_back(pt);
    }

    void process_sentence(const Sentence &sent) {
        this->process_parsing_tree(tb.parse_sentence(sent));
    }

    void process_parsing_tree(const ParsingTree< SymTok, LabTok > &pt) {
        this->stack.push_back(tb.refresh_parsing_tree(pt));
    }

    void process_assertion(const Assertion &ass) {
        ParsingTree< SymTok, LabTok > thesis;
        vector< ParsingTree< SymTok, LabTok > > hyps;
        tie(hyps, thesis) = tb.refresh_assertion(ass);
        assert(stack.size() >= hyps.size());
        for (size_t i = 0; i < hyps.size(); i++) {
            this->unificator.add_parsing_trees(this->stack[this->stack.size()-hyps.size()+i], hyps[i]);
        }
        this->stack.resize(this->stack.size() - hyps.size());
        this->stack.push_back(thesis);
    }

    void process_label(LabTok lab) {
        this->process_assertion(tb.get_assertion(lab));
    }

    bool compute_unification() {
        bool res;
        tie(res, this->subst) = this->unificator.unify2();
        return res;
    }

    ParsingTree< SymTok, LabTok > get_theorem() {
        assert(this->stack.size() == 1);
        return substitute(this->stack.at(0), this->is_var, this->subst);
    }

    vector< ParsingTree< SymTok, LabTok > > get_hypotheses() {
        vector< ParsingTree< SymTok, LabTok > > ret;
        for (const auto &hyp : this->hypotheses) {
            SubstMap< SymTok, LabTok >::iterator it = subst.find(hyp.second);
            if (it != subst.end()) {
                ret.push_back(it->second);
            } else {
                ParsingTree< SymTok, LabTok > pt;
                pt.label = hyp.second;
                // FIXME Fill pt.type
                ret.push_back(pt);
            }
        }
        return ret;
    }

private:
    map< size_t, LabTok >::iterator create_hypothesis(SymTok type_sym, size_t idx) {
        auto it = this->hypotheses.find(idx);
        if (it == this->hypotheses.end()) {
            LabTok type;
            tie(type, ignore) = tb.new_temp_var(type_sym);
            tie(it, ignore) = this->hypotheses.insert(make_pair(idx, type));
        }
        return it;
    }

    LibraryToolbox &tb;
    const std::function< bool(LabTok) > is_var;
    Unificator< SymTok, LabTok > unificator;
    vector< ParsingTree< SymTok, LabTok > > stack;
    map< size_t, LabTok > hypotheses;
    SubstMap< SymTok, LabTok > subst;
};

void test_unification2() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;

    for (const Assertion &ass : lib.get_assertions()) {
    //while (true) {
        //auto &ass = tb.get_assertion(tb.get_label("cvjust"));
        if (!ass.is_valid() || !ass.is_theorem() || tb.get_sentence(ass.get_thesis()).at(0) != tb.get_turnstile()) {
            continue;
        }

        auto pe = ass.get_proof_executor(tb);
        auto proof = pe->uncompress();
        auto labs = proof.get_labels();
        //cout << "Proof vector: {";
        Reactor reactor(tb);
        for (auto label : labs) {
            auto ess_hyps = ass.get_ess_hyps();
            auto it = find(ess_hyps.begin(), ess_hyps.end(), label);
            if (it != ess_hyps.end()) {
                //cout << " \"*\",";
                reactor.process_hypothesis(tb.get_symbol("wff"), it-ess_hyps.begin());
            } else if (tb.get_assertion(label).is_valid() && tb.get_sentence(label).at(0) == tb.get_turnstile()) {
                //cout << " \"" << tb.resolve_label(label) << "\",";
                reactor.process_label(label);
            }
        }
        //cout << " }" << endl;
        //cout << tb.print_proof(labs, true) << endl;

        /*for (auto it = steps.begin(); it != steps.end(); it++) {
            string &step = *it;
            if (step == "*") {
                bool res = reactor.process_hypothesis(tb.get_symbol("wff"));
                assert(res);
            } else {
                bool res = reactor.process_label(tb.get_label(step));
                assert(res);
            }
        }*/

        bool res = reactor.compute_unification();
        assert(res);

        SubstMap< SymTok, LabTok > subst2;
        tie(res, subst2) = unify(reactor.get_theorem(), tb.get_parsed_sents()[ass.get_thesis()], tb.get_standard_is_var());
        assert(res);
        bool presented = false;
        for (const auto &x : subst2) {
            if (!tb.get_standard_is_var()(x.second.label)) {
                if (!presented) {
                    cout << "FORWARD UNIFICATION TEST for " << tb.resolve_label(ass.get_thesis()) << endl;
                    cout << "Proved theorem: " << tb.print_sentence(reactor.get_theorem()) << endl;
                    cout << "with hypotheses:" << endl;
                    for (const auto &hyp : reactor.get_hypotheses()) {
                        cout << " * " << tb.print_sentence(hyp) << endl;
                    }
                    cout << "Relevant substitution items:" << endl;
                    presented = true;
                }
                ParsingTree< SymTok, LabTok > pt;
                pt.label = x.first;
                cout << " * " << tb.print_sentence(pt) << ": " << tb.print_sentence(x.second) << endl;
            }
        }
        if (presented) {
            cout << endl;
        }

        tb.release_all_temp_vars();
    }
}

//#pragma GCC pop_options
