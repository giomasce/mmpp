
#include <unordered_map>
#include <utility>
#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <random>

#include "test/test_env.h"
#include "toolbox.h"
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

        auto pe = ass.get_proof_executor(tb);
        auto proof = pe->uncompress();
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

        tb.release_all_temp_vars();
    }
}

int find_generalizable_theorems_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    find_generalizable_theorems();

    return 0;
}
static_block {
    register_main_function("mmpp_generalizable_theorems", find_generalizable_theorems_main);
}

int gen_random_theorems_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    random_device rand_dev;
    mt19937 rand_mt;
    rand_mt.seed(rand_dev());

    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;

    vector< const Assertion* > useful_asses;
    for (const auto &ass : lib.get_assertions()) {
        if (ass.is_valid() && lib.get_sentence(ass.get_thesis()).at(0) == tb.get_turnstile()) {
            useful_asses.push_back(&ass);
        }
    }
    cout << "There are " << useful_asses.size() << " useful theorems" << endl;

    BilateralUnificator< SymTok, LabTok > unif(tb.get_standard_is_var());
    vector< ParsingTree< SymTok, LabTok > > open_hyps;
    LabTok th_label;
    tie(th_label, ignore) = tb.new_temp_var(tb.get_turnstile_alias());
    ParsingTree< SymTok, LabTok > final_thesis;
    final_thesis.label = th_label;
    final_thesis.type = tb.get_turnstile_alias();
    open_hyps.push_back(final_thesis);

    for (size_t i = 0; i < 5; i++) {
        if (open_hyps.empty()) {
            cout << "Terminating early" << endl;
            break;
        }
        while (true) {
            // Select a random hypothesis, a random open hypothesis and let them match
            size_t ass_idx = uniform_int_distribution< size_t >(0, useful_asses.size()-1)(rand_mt);
            size_t hyp_idx = uniform_int_distribution< size_t >(0, open_hyps.size()-1)(rand_mt);
            const Assertion &ass = *useful_asses[ass_idx];
            ParsingTree< SymTok, LabTok > thesis;
            vector< ParsingTree< SymTok, LabTok > > hyps;
            tie(hyps, thesis) = tb.refresh_assertion(ass);
            auto unif2 = unif;
            unif2.add_parsing_trees(open_hyps[hyp_idx], thesis);
            if (unif2.unify().first) {
                cout << "Attaching " << tb.resolve_label(ass.get_thesis()) << " in position " << hyp_idx << endl;
                unif = unif2;
                open_hyps.erase(open_hyps.begin() + hyp_idx);
                open_hyps.insert(open_hyps.end(), hyps.begin(), hyps.end());
                break;
            }
        }
    }

    SubstMap< SymTok, LabTok > subst;
    bool res;
    tie(res, subst) = unif.unify();

    if (res) {
        cout << "Unification succedeed and proved:" << endl;
        cout << tb.print_sentence(substitute(final_thesis, tb.get_standard_is_var(), subst)) << endl;
        cout << "with the hypotheses:" << endl;
        for (const auto &hyp : open_hyps) {
            cout << " * " << tb.print_sentence(substitute(hyp, tb.get_standard_is_var(), subst)) << endl;
        }
    } else {
        cout << "Unification failed" << endl;
    }

    return 0;
}
static_block {
    register_main_function("mmpp_gen_random_theorems", gen_random_theorems_main);
}
