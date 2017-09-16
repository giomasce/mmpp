
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unordered_map>

#ifdef USE_QT
#include <QApplication>
#endif

#include <csignal>

#include "wff.h"
#include "parser.h"
#include "unification.h"
#include "memory.h"
#include "utils.h"
#include "earley.h"
#include "test.h"
#include "httpd.h"
#include "web.h"
#include "z3prover.h"

#ifdef USE_QT
#include "mainwindow.h"
#endif

using namespace std;

bool mmpp_abort = false;

void unification_loop() {

    cout << "Reading set.mm..." << endl;
    FileTokenizer ft("../set.mm/set.mm");
    Parser p(ft, false, true);
    p.run();
    LibraryImpl lib = p.get_library();
    LibraryToolbox tb(lib, true);
    cout << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << endl;
    cout << "Memory usage after loading the library: " << size_to_string(getCurrentRSS()) << endl;
    while (true) {
        string line;
        getline(cin, line);
        if (line == "") {
            break;
        }
        size_t dollar_pos;
        vector< vector< SymTok > > hypotheses;
        while ((dollar_pos = line.find("$")) != string::npos) {
            hypotheses.push_back(tb.parse_sentence(line.substr(0, dollar_pos)));
            line = line.substr(dollar_pos+1);
        }
        vector< SymTok > sent = tb.parse_sentence(line);
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

#ifdef USE_QT
int qt_main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
#endif

int test_all_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    test();
    return 0;
}

int test_one_main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Provide file name as argument, please" << endl;
        return 1;
    }
    string filename(argv[1]);
    return test_one(filename) ? 0 : 1;
}

int unification_loop_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    unification_loop();
    return 0;
}

const unordered_map< string, function< int(int, char*[]) > > MAIN_FUNCTIONS = {
    { "mmpp_test_one", test_one_main },
    { "mmpp_test_all", test_all_main },
    { "mmpp_test_z3", test_z3_main },
    { "unificator", unification_loop_main },
    { "webmmpp", httpd_main },
#ifdef USE_QT
    { "qmmpp", qt_main },
#endif
};

const function< int(int, char*[]) > DEFAULT_MAIN_FUNCTION = test_z3_main;

int main(int argc, char *argv[]) {
    char *tmp = strdup(argv[0]);
    string bname(basename(tmp));
    // string constructor should have made a copy
    free(tmp);
    function< int(int, char*[]) > main_func;
    try {
        main_func = MAIN_FUNCTIONS.at(bname);
    } catch (out_of_range e) {
        (void) e;
        // Return a default one
        main_func = DEFAULT_MAIN_FUNCTION;
    }
    try {
        return main_func(argc, argv);
    } catch (const MMPPException &e) {
        cerr << "Dying because of MMPPException with reason '" << e.get_reason() << "'..." << endl;
        return 1;
    } catch (const char* &e) {
        cerr << "Dying because of string exception '" << e << "'..." << endl;
        return 1;
    }
}
