
#include <unordered_map>
#include <utility>
#include <iostream>
#include <functional>
#include <vector>
#include <string>

#include "test/test_env.h"
#include "mm/toolbox.h"
#include "mm/proof.h"
#include "parsing/unif.h"
#include "utils/utils.h"

using namespace std;

class Reactor {
public:
    Reactor(LibraryToolbox &tb, size_t hyps_num) :
        tb(tb), is_var(tb.get_standard_is_var()), unificator(this->is_var)
    {
        this->hypotheses.reserve(hyps_num);
        for (size_t i = 0; i < hyps_num; i++) {
            LabTok type;
            SymTok type_sym = tb.get_turnstile_alias();
            tie(type, ignore) = tb.new_temp_var(type_sym);
            this->hypotheses.push_back(type);
        }
    }

    /*map< size_t, LabTok >::iterator create_hypothesis(SymTok type_sym, size_t idx) {
        auto it = this->hypotheses.find(idx);
        if (it == this->hypotheses.end()) {
            LabTok type;
            tie(type, ignore) = tb.new_temp_var(type_sym);
            tie(it, ignore) = this->hypotheses.insert(make_pair(idx, type));
        }
        return it;
    }*/

    bool process_hypothesis(SymTok type_sym, size_t idx) {
        //auto it = this->create_hypothesis(type_sym, idx);
        LabTok type = this->hypotheses[idx];
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
            this->unificator.add_parsing_trees(this->stack[this->stack.size()-hyps.size()+i], hyps[i]);
            if (this->unificator.has_failed()) {
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

    bool compute_unification() {
        bool res;
        tie(res, this->subst) = this->unificator.unify();
        return res;
    }

    ParsingTree< SymTok, LabTok > get_theorem() {
        assert(this->stack.size() == 1);
        return substitute(this->stack.at(0), this->is_var, this->subst);
    }

    vector< ParsingTree< SymTok, LabTok > > get_hypotheses() {
        vector< ParsingTree< SymTok, LabTok > > ret;
        for (const auto &hyp_lab : this->hypotheses) {
            SubstMap< SymTok, LabTok >::iterator it = subst.find(hyp_lab);
            if (it != subst.end()) {
                ret.push_back(it->second);
            } else {
                ParsingTree< SymTok, LabTok > pt;
                pt.label = hyp_lab;
                pt.type = this->tb.get_turnstile_alias();
                ret.push_back(pt);
            }
        }
        return ret;
    }

private:
    LibraryToolbox &tb;
    const std::function< bool(LabTok) > &is_var;
    BilateralUnificator< SymTok, LabTok > unificator;
    vector< ParsingTree< SymTok, LabTok > > stack;
    //map< size_t, LabTok > hypotheses;
    vector< LabTok > hypotheses;
    SubstMap< SymTok, LabTok > subst;
};

void find_generalizable_theorems() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;

    for (const Assertion &ass : lib.get_assertions()) {
    //while (true) {
        //auto &ass = tb.get_assertion(tb.get_label("cvjust"));
        if (!ass.is_valid() || !ass.is_theorem() || tb.get_sentence(ass.get_thesis()).at(0) != tb.get_turnstile()) {
            continue;
        }
        if (ass.is_modif_disc() || ass.is_usage_disc()) {
            continue;
        }
        tb.new_temp_var_frame();

        auto po = ass.get_proof_operator(tb);
        auto proof = po->uncompress();
        auto labs = proof.get_labels();
        //cout << "Proof vector: {";
        Reactor reactor(tb, ass.get_ess_hyps().size());
        for (auto label : labs) {
            auto ess_hyps = ass.get_ess_hyps();
            auto it = find(ess_hyps.begin(), ess_hyps.end(), label);
            if (it != ess_hyps.end()) {
                //cout << " \"*\",";
                bool res = reactor.process_hypothesis(tb.get_turnstile_alias(), it-ess_hyps.begin());
                assert(res);
            } else if (tb.get_assertion(label).is_valid() && tb.get_sentence(label).at(0) == tb.get_turnstile()) {
                //cout << " \"" << tb.resolve_label(label) << "\",";
                bool res = reactor.process_label(label);
                assert(res);
            }
        }
        //cout << " }" << endl;
        //cout << tb.print_proof(labs, true) << endl;

        bool res = reactor.compute_unification();
        assert(res);

        SubstMap< SymTok, LabTok > subst2;
        UnilateralUnificator< SymTok, LabTok > unif(tb.get_standard_is_var());
        unif.add_parsing_trees(reactor.get_theorem(), tb.get_parsed_sents()[ass.get_thesis()]);
        auto hypotheses = reactor.get_hypotheses();
        for (size_t i = 0; i < ass.get_ess_hyps().size(); i++) {
            unif.add_parsing_trees(hypotheses[i], tb.get_parsed_sents()[ass.get_ess_hyps()[i]]);
        }
        tie(res, subst2) = unif.unify();
        assert(res);
        SubstMap< SymTok, LabTok > generalizables;
        SubstMap< SymTok, LabTok > not_generalizables;
        LabTok lab_cvv = tb.get_label("cvv");
        LabTok lab_cv = tb.get_label("cv");
        for (const auto &x : subst2) {
            bool generalizable = true;
            if (tb.get_standard_is_var()(x.second.label)) {
                generalizable = false;
            }
            if (x.second.label == lab_cvv || x.second.label == lab_cv) {
                generalizable = false;
            }
            if (generalizable) {
                generalizables.insert(x);
            } else {
                not_generalizables.insert(x);
            }
        }

        if (!generalizables.empty()) {
            cout << "GENERALIZABLE THEOREM (" << tb.resolve_label(ass.get_thesis()) << ")" << endl;
            cout << "Theorem: " << tb.print_sentence(reactor.get_theorem()) << endl;
            if (ass.get_ess_hyps().size() != 0) {
                cout << "with hypotheses:" << endl;
                for (const auto &hyp : reactor.get_hypotheses()) {
                    cout << " * " << tb.print_sentence(hyp) << endl;
                }
            }
            cout << "Substitution map for generalizable variables:" << endl;
            for (const auto &x : generalizables) {
                ParsingTree< SymTok, LabTok > pt;
                pt.label = x.first;
                cout << " * " << tb.print_sentence(pt) << ": " << tb.print_sentence(x.second) << endl;
            }
            if (!not_generalizables.empty()) {
                cout << "Substitution map for other variables:" << endl;
                for (const auto &x : not_generalizables) {
                    ParsingTree< SymTok, LabTok > pt;
                    pt.label = x.first;
                    cout << " * " << tb.print_sentence(pt) << ": " << tb.print_sentence(x.second) << endl;
                }
            }
            cout << endl;
        }

        tb.release_temp_var_frame();
    }
}

int find_generalizable_theorems_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    find_generalizable_theorems();

    return 0;
}
static_block {
    register_main_function("generalizable_theorems", find_generalizable_theorems_main);
}
