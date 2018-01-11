
#include <unordered_map>
#include <utility>
#include <iostream>
#include <functional>
#include <vector>
#include <string>

#include "test/test_env.h"

#include "mm/toolbox.h"
#include "parsing/unif.h"

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
