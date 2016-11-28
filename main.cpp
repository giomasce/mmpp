
#include <iostream>
#include <fstream>
#include <iomanip>

#include "wff.h"
#include "parser.h"
#include "unification.h"
#include "memory.h"

using namespace std;

// Partly taken from http://programanddesign.com/cpp/human-readable-file-size-in-c/
static inline string size_to_string(size_t size) {
    ostringstream stream;
    int i = 0;
    const char* units[] = {"B", "kiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};
    while (size > 1024) {
        size /= 1024;
        i++;
    }
    stream << fixed << setprecision(2) << size << " " << units[i];
    return stream.str();
}

int main() {

    // This program just does a lot of tests on the features of the mmpp library

    if (false) {
        auto ph = pwff(new Var("ph"));
        auto ps = pwff(new Var("ps"));
        auto w = pwff(new Nand(ph, ps));
        auto w2 = pwff(new Xor(w, ph));

        cout << w2->to_string() << endl;
        cout << w2->imp_not_form()->to_string() << endl;

        CompressedDecoder cd;
        string test_enc[] = { "A", "B", "T", "UA", "UB", "UT", "VA", "VB", "YT", "UUA", "YYT", "UUUA", "UUUAZ" };
        int test_dec[] = { 1, 2, 20, 21, 22, 40, 41, 42, 120, 121, 620, 621, 0 };
        for (auto &str : test_enc) {
            cout << "Testing " << str << ":";
            for (auto &c : str) {
                cout << " " << c << "->" << cd.push_char(c);
            }
            cout << endl;
        }
        CompressedEncoder ce;
        for (auto &val : test_dec) {
            cout << "Testing " << val << ": " << ce.push_code(val) << endl;
        }

        cout << "Memory usage: " << size_to_string(getCurrentRSS()) << endl;
    }

    if (true) {
        fstream in("/home/giovanni/progetti/metamath/set.mm/set.mm");
        FileTokenizer ft(in);
        Parser p(ft, true);
        p.run();
        Library lib = p.get_library();
        cout << lib.get_symbol_num() << " symbols and " << lib.get_label_num() << " labels" << endl;

        if (true) {
            vector< SymTok > sent = parse_sentence("wff ( ph -> ( ps -> ch ) )", lib);
            vector< SymTok > templ = parse_sentence("wff ( th -> et )", lib);
            auto res = unify(sent, templ, lib, false);
            cout << "Matching:         " << print_sentence(sent, lib) << endl << "against template: " << print_sentence(templ, lib) << endl;
            for (auto &match : res) {
                cout << "  *";
                for (auto &var: match) {
                    cout << " " << print_sentence({var.first}, lib) << " => " << print_sentence(var.second, lib) << "  ";
                }
                cout << endl;
            }
        }

        if (true) {
            //auto res = lib.unify_assertion({ parse_sentence("|- ( ch -> th )", lib), parse_sentence("|- ch", lib) }, parse_sentence("|- th", lib));
            auto res = lib.unify_assertion({ parse_sentence("|- ( ch -> ( ph -> ps ) )", lib), parse_sentence("|- ch", lib) }, parse_sentence("|- ( ph -> ps )", lib));
            cout << "Found " << res.size() << " matching assertions:" << endl;
            for (auto &match : res) {
                auto &label = match.first;
                const Assertion &ass = lib.get_assertion(label);
                cout << " * " << lib.resolve_label(label) << ":";
                for (auto &hyp : ass.get_ess_hyps()) {
                    auto &hyp_sent = lib.get_sentence(hyp);
                    cout << " & " << print_sentence(hyp_sent, lib);
                }
                auto &thesis_sent = lib.get_sentence(ass.get_thesis());
                cout << " => " << print_sentence(thesis_sent, lib) << endl;
            }
        }

        if (true) {
            //auto res = lib.prove_type(parse_sentence("wff ( x = y -> ps )", lib));
            auto res = lib.prove_type(parse_sentence("wff ( [ suc z / z ] ( rec ( f , q ) ` z ) e. x <-> A. z ( z = suc z -> ( rec ( f , q ) ` z ) e. x ) )", lib));
            cout << "Found type proof:";
            for (auto &label : res) {
                cout << " " << lib.resolve_label(label);
            }
            cout << endl;
        }

        if (true) {
            cout << "Executing all proofs..." << endl;
            for (auto &ass : lib.get_assertions()) {
                if (ass.is_valid() && ass.is_theorem()) {
                    ass.get_proof_executor(lib)->execute();
                }
            }
        }

        cout << "Memory usage: " << size_to_string(getCurrentRSS()) << endl;
    }

    if (true) {
        fstream in("/home/giovanni/progetti/metamath/demo0.mm");
        FileTokenizer ft(in);
        Parser p(ft, true);
        p.run();
        Library lib = p.get_library();
        cout << lib.get_symbol_num() << " symbols and " << lib.get_label_num() << " labels" << endl;

        cout << "Memory usage: " << size_to_string(getCurrentRSS()) << endl;
    }

    cout << "Maximum memory usage: " << size_to_string(getPeakRSS()) << endl;

    return 0;

}
