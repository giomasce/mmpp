
#include <iostream>
#include <fstream>
#include <iomanip>

#include "wff.h"
#include "parser.h"
#include "unification.h"
#include "memory.h"
#include "statics.h"

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

bool mmpp_abort = false;

const string test_basename = "/home/giovanni/progetti/metamath/metamath-test";
const string tests_filenames = R"tests(
fail anatomy-bad1.mm "Simple incorrect 'anatomy' test "
fail anatomy-bad2.mm "Simple incorrect 'anatomy' test "
fail anatomy-bad3.mm "Simple incorrect 'anatomy' test "
pass big-unifier.mm
fail big-unifier-bad1.mm
fail big-unifier-bad2.mm
fail big-unifier-bad3.mm
pass demo0.mm
fail demo0-bad1.mm
pass demo0-includer.mm "Test simple file inclusion"
pass emptyline.mm 'A file with one empty line'
pass hol.mm
pass iset.mm
pass miu.mm
pass nf.mm
pass peano-fixed.mm
pass ql.mm
pass set.2010-08-29.mm
pass set.mm
fail set-dist.mm
)tests";

static vector< pair < string, bool > > get_tests() {
    istringstream iss(tests_filenames);
    vector< pair< string, bool > > tests;
    string line;
    while (getline(iss, line)) {
        string success, filename;
        istringstream iss2(line);
        iss2 >> success >> filename;
        if (filename == "") {
            continue;
        }
        tests.push_back(make_pair(filename, success == "pass"));
    }
    return tests;
}

void test() {

    /* This program just does a lot of tests on the features of the mmpp library
     */

    if (true) {
        cout << "Testing random small stuff..." << endl;
        auto ph = pwff(new Var("ph"));
        auto ps = pwff(new Var("ps"));
        auto w = pwff(new Nand(ph, ps));
        auto w2 = pwff(new Xor(w, pwff(new And(pwff(new True()), ph))));

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

        cout << "Finished. Memory usage: " << size_to_string(getCurrentRSS()) << endl << endl;
    }

    auto tests = get_tests();
    //tests = {};
    int problems = 0;
    for (auto test_pair : tests) {
        string filename = test_pair.first;
        bool success = true;
        bool expect_success = test_pair.second;

        // Useful for debugging
        /*if (filename == "big-unifier.mm") {
            mmpp_abort = true;
        } else {
            continue;
            mmpp_abort = false;
        }*/
        /*if (filename == "nf.mm") {
            break;
        }*/

        cout << "Testing file " << filename << " from " << test_basename << ", which is expected to " << (expect_success ? "pass" : "fail" ) << "..." << endl;
        try {
            cout << "Memory usage when starting: " << size_to_string(getCurrentRSS()) << endl;
            FileTokenizer ft(test_basename + "/" + filename);
            Parser p(ft, true, true);
            cout << "Reading library and executing all proofs..." << endl;
            p.run();
            Library lib = p.get_library();
            cout << "Library has " << lib.get_symbol_num() << " symbols and " << lib.get_label_num() << " labels" << endl;

            /*LabTok failing = 287;
            cout << "Checking proof of " << lib.resolve_label(failing) << endl;
            lib.get_assertion(failing).get_proof_executor(lib)->execute();*/

            if (expect_success) {
                cout << "Compressing all proofs and executing again..." << endl;
                for (auto &ass : lib.get_assertions()) {
                    if (ass.is_valid() && ass.is_theorem()) {
                        CompressedProof compressed = ass.get_proof_executor(lib)->compress();
                        compressed.get_executor(lib, ass)->execute();
                    }
                }

                cout << "Decompressing all proofs and executing again..." << endl;
                for (auto &ass : lib.get_assertions()) {
                    if (ass.is_valid() && ass.is_theorem()) {
                        UncompressedProof uncompressed = ass.get_proof_executor(lib)->uncompress();
                        uncompressed.get_executor(lib, ass)->execute();
                    }
                }

                cout << "Compressing and decompressing all proofs and executing again..." << endl;
                for (auto &ass : lib.get_assertions()) {
                    if (ass.is_valid() && ass.is_theorem()) {
                        CompressedProof compressed = ass.get_proof_executor(lib)->compress();
                        UncompressedProof uncompressed = compressed.get_executor(lib, ass)->uncompress();
                        uncompressed.get_executor(lib, ass)->execute();
                    }
                }

                cout << "Decompressing and compressing all proofs and executing again..." << endl;
                for (auto &ass : lib.get_assertions()) {
                    if (ass.is_valid() && ass.is_theorem()) {
                        UncompressedProof uncompressed = ass.get_proof_executor(lib)->uncompress();
                        CompressedProof compressed = uncompressed.get_executor(lib, ass)->compress();
                        compressed.get_executor(lib, ass)->execute();
                    }
                }
            } else {
                cout << "Skipping compression and decompression test, since unit is known to fail" << endl;
            }

            cout << "Finished. Memory usage: " << size_to_string(getCurrentRSS()) << endl;
        } catch (MMPPException e) {
            cout << "An exception with message '" << e.get_reason() << "' was thrown!" << endl;
            e.print_stacktrace(cout);
            success = false;
        }
        if (success) {
            if (expect_success) {
                cout << "Good, it worked!" << endl;
            } else {
                cout << "Bad, it should have been failed!" << endl;
                problems++;
            }
        } else {
            if (expect_success) {
                cout << "Bad, this was NOT expected!" << endl;
                problems++;
            } else {
                cout << "Good, this was expected!" << endl;
            }
        }

        cout << "-------" << endl << endl;
    }
    cout << "Found " << problems << " problems" << endl;

    if (true) {
        cout << "Doing additional tests on set.mm..." << endl;
        FileTokenizer ft(test_basename + "/set.mm");
        Parser p(ft, false, true);
        p.run();
        Library lib = p.get_library();
        cout << lib.get_symbol_num() << " symbols and " << lib.get_label_num() << " labels" << endl;
        cout << "Memory usage after loading the library: " << size_to_string(getCurrentRSS()) << endl << endl;

        if (true) {
            cout << "Generic unification test" << endl;
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
            cout << "Memory usage after test: " << size_to_string(getCurrentRSS()) << endl << endl;
        }

        if (true) {
            cout << "Statement unification test" << endl;
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
            cout << "Memory usage after test: " << size_to_string(getCurrentRSS()) << endl << endl;
        }

        if (true) {
            cout << "Type proving test" << endl;
            //auto res = lib.prove_type(parse_sentence("wff ( x = y -> ps )", lib));
            auto res = lib.prove_type(parse_sentence("wff ( [ suc z / z ] ( rec ( f , q ) ` z ) e. x <-> A. z ( z = suc z -> ( rec ( f , q ) ` z ) e. x ) )", lib));
            cout << "Found type proof:";
            for (auto &label : res) {
                cout << " " << lib.resolve_label(label);
            }
            cout << endl;
            cout << "Memory usage after test: " << size_to_string(getCurrentRSS()) << endl << endl;
        }
    }

    cout << "Maximum memory usage: " << size_to_string(getPeakRSS()) << endl;

}

void unification_loop() {

    cout << "Reading set.mm..." << endl;
    FileTokenizer ft("../set.mm/set.mm");
    Parser p(ft, false, true);
    p.run();
    Library lib = p.get_library();
    cout << lib.get_symbol_num() << " symbols and " << lib.get_label_num() << " labels" << endl;
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
            hypotheses.push_back(parse_sentence(line.substr(0, dollar_pos), lib));
            line = line.substr(dollar_pos+1);
        }
        vector< SymTok > sent = parse_sentence(line, lib);
        /*auto sent_wff = sent;
        sent_wff[0] = lib.get_symbol("wff");
        auto res = lib.prove_type(sent_wff);
        cout << "Found type proof:";
        for (auto &label : res) {
            cout << " " << lib.resolve_label(label);
        }
        cout << endl;*/
        auto res2 = lib.unify_assertion(hypotheses, sent);
        cout << "Found " << res2.size() << " matching assertions:" << endl;
        for (auto &match : res2) {
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

}

int main() {

    test();
    //unification_loop();
    return 0;

}
