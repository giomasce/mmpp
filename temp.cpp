
#include <unordered_map>
#include <utility>
#include <iostream>
#include <functional>
#include <vector>
#include <string>

#include "test/test_env.h"

#include "mm/toolbox.h"
#include "parsing/unif.h"
#include "provers/wff.h"

using namespace std;

//#pragma GCC push_options
//#pragma GCC optimize ("O0")

//std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > > /*__attribute__((optimize("O0")))*/ LibraryToolbox::unify_assertion_uncached3(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first, bool up_to_hyps_perms) const

//#pragma GCC pop_options

class RootStats {
public:
    RootStats(const LibraryToolbox &tb) : tb(tb) {}
    void process(const ParsingTree< SymTok, LabTok > &pt) {
        LabTok root_label = pt.label;
        if (this->tb.get_standard_is_var()(root_label)) {
            root_label = 0;
        }
        stat[root_label] += 1;
    }

    void print_stats() const {
        vector< pair< uint32_t, LabTok > > stat_vect;
        for (const auto &x : this->stat) {
            stat_vect.push_back(make_pair(x.second, x.first));
        }
        sort(stat_vect.begin(), stat_vect.end());

        uint32_t total = 0;
        for (const auto &x : stat_vect) {
            cout << x.first << ": " << this->tb.resolve_label(x.second) << endl;
            total += x.first;
        }
        cout << "total = " << total << endl;
    }

private:
    const LibraryToolbox &tb;
    unordered_map< LabTok, uint32_t > stat;
};

int count_root_type_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    RootStats root(tb);
    RootStats imp1(tb);
    RootStats imp2(tb);
    LabTok imp_label = tb.get_label("wi");
    for (const auto &ass : tb.get_library().get_assertions()) {
        if (ass.is_valid() && tb.get_sentence(ass.get_thesis()).at(0) == tb.get_turnstile()) {
            const auto &pt = tb.get_parsed_sents().at(ass.get_thesis());
            root.process(pt);
            if (pt.label == imp_label) {
                imp1.process(pt.children[0]);
                imp2.process(pt.children[1]);
            }
        }
    }

    cout << "ROOT LABELS" << endl;
    root.print_stats();
    cout << endl;
    cout << "IMPLICATION ANTECEDENTS' LABELS" << endl;
    imp1.print_stats();
    cout << endl;
    cout << "IMPLICATION CONSEQUENTS' LABELS" << endl;
    imp2.print_stats();

    return 0;
}
static_block {
    register_main_function("count_root_type", count_root_type_main);
}

int temp_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto thesis = tb.read_sentence("|- ( ( x e. SetAlg /\\ A. z ( ( z C_ x /\\ z ~<_ om ) -> U. z e. x ) ) -> ( A. y e. x ( U. x \\ y ) e. x /\\ A. z ( ( z C_ x /\\ z ~<_ om ) -> U. z e. x ) ) )");
    auto hyp1 = tb.read_sentence("|- ( x e. _V -> ( x e. SetAlg <-> ( ( (/) e. x /\\ A. y e. x A. b e. x ( y i^i b ) e. x ) /\\ ( A. y e. x A. b e. x ( y u. b ) e. x /\\ A. y e. x ( U. x \\ y ) e. x ) ) ) )");
    auto hyp2 = tb.read_sentence("|- x e. _V");

    ParsingTree< SymTok, LabTok > thesis_pt = tb.parse_sentence(thesis.begin()+1, thesis.end(), tb.get_turnstile_alias());
    ParsingTree< SymTok, LabTok > hyp1_pt = tb.parse_sentence(hyp1.begin()+1, hyp1.end(), tb.get_turnstile_alias());
    ParsingTree< SymTok, LabTok > hyp2_pt = tb.parse_sentence(hyp2.begin()+1, hyp2.end(), tb.get_turnstile_alias());

    pwff wff = Imp::create(wff_from_pt(hyp2_pt, tb), Imp::create(wff_from_pt(hyp1_pt, tb), wff_from_pt(thesis_pt, tb)));

    cout << wff->to_string() << endl;

    auto tseitin = Not::create(wff)->get_tseitin_dimacs(tb);
    auto dimacs = tseitin.first;
    auto ts_map = tseitin.second;
    dimacs.print_dimacs(cout);
    for (const auto &x : ts_map) {
        cout << (x.second + 1) << " : " << x.first->to_string() << endl;
    }

    pvar_set vars;
    wff->get_variables(vars);
    cout << "There are " << vars.size() << " variables" << endl;
    auto prover = wff->get_adv_truth_prover(tb);
    cout << prover.first << endl;

    CreativeProofEngineImpl< Sentence > engine(tb);
    prover.second(engine);
    if (engine.get_proof_labels().size() > 0) {
        //cout << "adv truth proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
        cout << "stack top: " << tb.print_sentence(engine.get_stack().back(), SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << endl;
        cout << "proof length: " << engine.get_proof_labels().size() << endl;
        //UncompressedProof proof = { engine.get_proof_labels() };
    }

    return 0;
}
static_block {
    register_main_function("temp", temp_main);
}
