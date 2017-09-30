
#include <iostream>
#include <chrono>

#include "reader.h"
#include "library.h"
#include "utils.h"
#include "toolbox.h"
#include "platform.h"

using namespace std;
using namespace chrono;

void unification_test() {

    cout << "Reading set.mm..." << endl;
    FileTokenizer ft("../set.mm/set.mm");
    Reader p(ft, false, true);
    p.run();
    LibraryImpl lib = p.get_library();
    LibraryToolbox tb(lib, true);
    cout << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << endl;
    cout << "Memory usage after loading the library: " << size_to_string(platform_get_current_rss()) << endl;
    vector< string > tests = { "|- ( ( A e. CC /\\ B e. CC /\\ N e. NN0 ) -> ( ( A + B ) ^ N ) = sum_ k e. ( 0 ... N ) ( ( N _C k ) x. ( ( A ^ ( N - k ) ) x. ( B ^ k ) ) ) )",
                               "|- ( ph -> ( ps <-> ps ) )",
                               "|- ( ph -> ph )" };
    int reps = 30;
    for (const auto &test : tests) {
        Sentence sent = tb.read_sentence(test);
        auto res2 = tb.unify_assertion({}, sent, false, true);
        cout << "Trying to unify " << test << endl;
        cout << "Found " << res2.size() << " matching assertions:" << endl;
        for (auto &match : res2) {
            auto &label = get<0>(match);
            const Assertion &ass = lib.get_assertion(label);
            cout << " * " << lib.resolve_label(label) << ":";
            for (auto &hyp : ass.get_ess_hyps()) {
                auto &hyp_sent = lib.get_sentence(hyp);
                cout << " & " << tb.print_sentence(hyp_sent);
            }
            auto &thesis_sent = lib.get_sentence(ass.get_thesis());
            cout << " => " << tb.print_sentence(thesis_sent) << endl;
        }

        // Do actual time measurement
        auto begin = steady_clock::now();
        for (int i = 0; i < reps; i++) {
            res2 = tb.unify_assertion({}, sent, false, true);
        }
        auto end = steady_clock::now();
        auto usecs = duration_cast< microseconds >(end - begin).count();
        cout << "It took " << usecs << " microseconds to repeat the unification " << reps << " times, which is " << (usecs / reps) << " microsecond per execution" << endl;
    }

}

void unification_loop() {

    cout << "Reading set.mm..." << endl;
    FileTokenizer ft("../set.mm/set.mm");
    Reader p(ft, false, true);
    p.run();
    LibraryImpl lib = p.get_library();
    LibraryToolbox tb(lib, true);
    cout << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << endl;
    cout << "Memory usage after loading the library: " << size_to_string(platform_get_current_rss()) << endl;
    while (true) {
        string line;
        getline(cin, line);
        if (line == "") {
            break;
        }
        size_t dollar_pos;
        vector< vector< SymTok > > hypotheses;
        while ((dollar_pos = line.find("$")) != string::npos) {
            hypotheses.push_back(tb.read_sentence(line.substr(0, dollar_pos)));
            line = line.substr(dollar_pos+1);
        }
        vector< SymTok > sent = tb.read_sentence(line);
        /*auto sent_wff = sent;
        sent_wff[0] = lib.get_symbol("wff");
        auto res = lib.prove_type(sent_wff);
        cout << "Found type proof:";
        for (auto &label : res) {
            cout << " " << lib.resolve_label(label);
        }
        cout << endl;*/
        auto res2 = tb.unify_assertion(hypotheses, sent);
        cout << "Found " << res2.size() << " matching assertions:" << endl;
        for (auto &match : res2) {
            auto &label = get<0>(match);
            const Assertion &ass = lib.get_assertion(label);
            cout << " * " << lib.resolve_label(label) << ":";
            for (auto &hyp : ass.get_ess_hyps()) {
                auto &hyp_sent = lib.get_sentence(hyp);
                cout << " & " << tb.print_sentence(hyp_sent);
            }
            auto &thesis_sent = lib.get_sentence(ass.get_thesis());
            cout << " => " << tb.print_sentence(thesis_sent) << endl;
        }
    }

}

int unification_loop_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    unification_loop();
    return 0;
}
static_block {
    register_main_function("unificator", unification_loop_main);
}

int unification_test_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    unification_test();
    return 0;
}
static_block {
    register_main_function("unification_test", unification_test_main);
}
