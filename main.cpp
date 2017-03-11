
#include <iostream>
#include <fstream>
#include <iomanip>

#include <QApplication>

#include <csignal>

#include "wff.h"
#include "parser.h"
#include "unification.h"
#include "memory.h"
#include "utils.h"
#include "earley.h"
#include "mainwindow.h"
#include "test.h"
#include "httpd.h"
#include "webendpoint.h"

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

int qt_main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

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

atomic< bool > signalled;
void int_handler(int signal) {
    (void) signal;
    signalled = true;
}

int httpd_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    cout << "Reading set.mm..." << endl;
    FileTokenizer ft("../set.mm/set.mm");
    //FileTokenizer ft("../metamath/ql.mm");
    Parser p(ft, false, true);
    p.run();
    LibraryImpl lib = p.get_library();
    //LibraryToolbox tb(lib, true);
    cout << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << endl;
    cout << "Memory usage after loading the library: " << size_to_string(getCurrentRSS()) << endl;

    WebEndpoint endpoint(lib);

    HTTPD_microhttpd httpd(8888, endpoint);

    struct sigaction act;
    act.sa_handler = int_handler;
    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    httpd.start();
    while (true) {
        if (signalled) {
            cerr << "Signal received, stopping..." << endl;
            httpd.stop();
            break;
        }
        sleep(1);
    }
    httpd.join();

    return 0;
}

int main(int argc, char *argv[]) {
    //return test_one_main(argc, argv);
    //return test_all_main(argc, argv);
    //return unification_loop_main(argc, argv);
    //return qt_main(argc, argv);
    return httpd_main(argc, argv);
}
