#include "test.h"

#include <ostream>
#include <functional>

#include "wff.h"
#include "reader.h"
#include "unification.h"
#include "memory.h"
#include "utils.h"
#include "earley.h"
#include "lr.h"
#include "utils.h"

using namespace std;

const string test_basename = "../metamath-test";
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
pass lof.mm
pass lofmathbox.mm
pass lofset.mm
pass set(lof).mm
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

bool test_one(string filename, bool advanced_tests) {
    bool success = true;
    try {
        cout << "Memory usage when starting: " << size_to_string(getCurrentRSS()) << endl;
        FileTokenizer ft(test_basename + "/" + filename);
        Reader p(ft, true, true);
        cout << "Reading library and executing all proofs..." << endl;
        p.run();
        LibraryImpl lib = p.get_library();
        cout << "Library has " << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << endl;

        /*LabTok failing = 287;
        cout << "Checking proof of " << lib.resolve_label(failing) << endl;
        lib.get_assertion(failing).get_proof_executor(lib)->execute();*/

        if (advanced_tests) {
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
            cout << "Skipping compression and decompression test" << endl;
        }

        cout << "Finished. Memory usage: " << size_to_string(getCurrentRSS()) << endl;
    } catch (const MMPPException &e) {
        cout << "An exception with message '" << e.get_reason() << "' was thrown!" << endl;
        e.print_stacktrace(cout);
        success = false;
    } catch (const ProofException &e) {
        cout << "An exception with message '" << e.get_reason() << "' was thrown!" << endl;
        success = false;
    }

    return success;
}

template< typename SymType, typename LabType >
void test_parsers(const std::vector<SymType> &sent, SymType type, const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations) {
    cout << "Earley parser" << endl;
    auto res = earley(sent, type, derivations);
    assert(res.label != 0);

    cout << "LR parser" << endl;
    process_derivations(derivations);
}

void test_grammar1() {
    /* Describe the grammar at http://loup-vaillant.fr/tutorials/earley-parsing/recogniser.
     * Only digit up to 4 are used to keep tables small and debugging easy.
     */
    std::unordered_map<string, std::vector<std::pair< size_t, std::vector<string> > > > derivations;
    derivations["Sum"].push_back(make_pair(100, vector< string >({ "Sum", "+", "Product" })));
    derivations["Sum"].push_back(make_pair(101, vector< string >({ "Sum", "-", "Product" })));
    derivations["Sum"].push_back(make_pair(102, vector< string >({ "Product" })));
    derivations["Product"].push_back(make_pair(103, vector< string >({ "Product", "*", "Product" })));
    derivations["Product"].push_back(make_pair(104, vector< string >({ "Product", "/", "Product" })));
    derivations["Product"].push_back(make_pair(105, vector< string >({ "Factor" })));
    derivations["Factor"].push_back(make_pair(106, vector< string >({ "(", "Sum", ")" })));
    derivations["Factor"].push_back(make_pair(107, vector< string >({ "Number" })));
    derivations["Number"].push_back(make_pair(108, vector< string >({ "0", "Number" })));
    derivations["Number"].push_back(make_pair(109, vector< string >({ "1", "Number" })));
    derivations["Number"].push_back(make_pair(110, vector< string >({ "2", "Number" })));
    derivations["Number"].push_back(make_pair(111, vector< string >({ "3", "Number" })));
    derivations["Number"].push_back(make_pair(112, vector< string >({ "4", "Number" })));
    derivations["Number"].push_back(make_pair(113, vector< string >({ "0" })));
    derivations["Number"].push_back(make_pair(114, vector< string >({ "1" })));
    derivations["Number"].push_back(make_pair(115, vector< string >({ "2" })));
    derivations["Number"].push_back(make_pair(116, vector< string >({ "3" })));
    derivations["Number"].push_back(make_pair(117, vector< string >({ "4" })));
    vector< string > sent = { "1", "+", "(", "2", "*", "3", "-", "4", ")" };
    test_parsers< string, size_t >(sent, "Sum", derivations);
}

void test_grammar2() {
    /* Describe the grammar at https://web.cs.dal.ca/~sjackson/lalr1.html.
     */
    std::unordered_map<char, std::vector<std::pair< size_t, std::vector<char> > > > derivations;
    derivations['S'].push_back(make_pair(1, vector< char >({ 'N' })));
    derivations['N'].push_back(make_pair(2, vector< char >({ 'V', '=', 'E' })));
    derivations['N'].push_back(make_pair(3, vector< char >({ 'E' })));
    derivations['E'].push_back(make_pair(4, vector< char >({ 'V' })));
    derivations['V'].push_back(make_pair(5, vector< char >({ 'x' })));
    derivations['V'].push_back(make_pair(6, vector< char >({ '*', 'E' })));
    vector< char > sent = { 'x', '=', '*', 'x' };
    test_parsers< char, size_t >(sent, 'S', derivations);
}

void test_grammar3() {
    /* Describe the grammar at https://en.wikipedia.org/wiki/LR_parser.
     */
    std::unordered_map<string, std::vector<std::pair< size_t, std::vector<string> > > > derivations;
    derivations["Goal"].push_back(make_pair(1, vector< string >({ "Sums" })));
    derivations["Sums"].push_back(make_pair(2, vector< string >({ "Sums", "+", "Products" })));
    derivations["Sums"].push_back(make_pair(3, vector< string >({ "Products" })));
    derivations["Products"].push_back(make_pair(4, vector< string >({ "Products", "*", "Value" })));
    derivations["Products"].push_back(make_pair(5, vector< string >({ "Value" })));
    derivations["Value"].push_back(make_pair(6, vector< string >({ "int" })));
    derivations["Value"].push_back(make_pair(7, vector< string >({ "id" })));
    vector< string > sent = { "id", "*", "int", "+", "int" };
    test_parsers< string, size_t >(sent, "Goal", derivations);
}

void test_grammar4() {
    /* Describe the grammar at https://dickgrune.com/Books/PTAPG_1st_Edition/BookBody.pd, page 201.
     */
    std::unordered_map<char, std::vector<std::pair< size_t, std::vector<char> > > > derivations;
    derivations['S'].push_back(make_pair(1, vector< char >({ 'E' })));
    derivations['E'].push_back(make_pair(2, vector< char >({ 'E', '-', 'T' })));
    derivations['E'].push_back(make_pair(3, vector< char >({ 'T' })));
    derivations['T'].push_back(make_pair(4, vector< char >({ 'n' })));
    derivations['T'].push_back(make_pair(5, vector< char >({ '(', 'E', ')' })));
    vector< char > sent = { 'n', '-', 'n', '-', 'n' };
    test_parsers< char, size_t >(sent, 'S', derivations);
}

void test() {

    /* This program just does a lot of tests on the features of the mmpp library
     */

    if (false) {
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

    if (false) {
        cout << "Generic parser test" << endl;
        test_grammar1();
        test_grammar2();
        test_grammar3();
        test_grammar4();
    }

    if (true) {
        cout << "LR persing on set.mm" << endl;
        FileTokenizer ft("../set.mm/set.mm");
        Reader p(ft, false, true);
        p.run();
        LibraryImpl lib = p.get_library();
        LibraryToolbox tb(lib, false);
        cout << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << endl;
        cout << "Memory usage after loading the library: " << size_to_string(getCurrentRSS()) << endl << endl;

        std::function< std::ostream&(std::ostream&, SymTok) > sym_printer =  [&](ostream &os, SymTok sym)->ostream& { return os << lib.resolve_symbol(sym); };
        std::function< std::ostream&(std::ostream&, LabTok) > lab_printer =  [&](ostream &os, LabTok lab)->ostream& { return os << lib.resolve_label(lab); };
        process_derivations(tb.get_derivations(), sym_printer, lab_printer);
    }
    return;

    if (false) {
        cout << "Doing additional tests on set.mm..." << endl;
        FileTokenizer ft("../set.mm/set.mm");
        Reader p(ft, false, true);
        p.run();
        LibraryImpl lib = p.get_library();
        LibraryToolbox tb(lib, true);
        cout << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << endl;
        cout << "Memory usage after loading the library: " << size_to_string(getCurrentRSS()) << endl << endl;

        if (true) {
            cout << "Generic unification test" << endl;
            vector< SymTok > sent = tb.parse_sentence("wff ( ph -> ( ps -> ch ) )");
            vector< SymTok > templ = tb.parse_sentence("wff ( th -> et )");
            auto res = unify(sent, templ, lib, false);
            cout << "Matching:         " << tb.print_sentence(sent) << endl << "against template: " << tb.print_sentence(templ) << endl;
            for (auto &match : res) {
                cout << "  *";
                for (auto &var: match) {
                    cout << " " << tb.print_sentence({var.first}) << " => " << tb.print_sentence(var.second) << "  ";
                }
                cout << endl;
            }
            cout << "Memory usage after test: " << size_to_string(getCurrentRSS()) << endl << endl;
        }

        if (true) {
            cout << "Statement unification test" << endl;
            //auto res = lib.unify_assertion({ parse_sentence("|- ( ch -> th )", lib), parse_sentence("|- ch", lib) }, parse_sentence("|- th", lib));
            auto res = tb.unify_assertion({ tb.parse_sentence("|- ( ch -> ( ph -> ps ) )"), tb.parse_sentence("|- ch") }, tb.parse_sentence("|- ( ph -> ps )"));
            cout << "Found " << res.size() << " matching assertions:" << endl;
            for (auto &match : res) {
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
            cout << "Memory usage after test: " << size_to_string(getCurrentRSS()) << endl << endl;
        }

        if (true) {
            cout << "Type proving test" << endl;
            //auto sent = lib.parse_sentence("wff ph");
            auto sent = tb.parse_sentence("wff ( [ suc z / z ] ( rec ( f , q ) ` z ) e. x <-> A. z ( z = suc z -> ( rec ( f , q ) ` z ) e. x ) )");
            cout << "Sentence is " << tb.print_sentence(sent) << endl;
            cout << "HTML sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_HTML) << endl;
            cout << "Alt HTML sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_ALTHTML) << endl;
            cout << "LaTeX sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_LATEX) << endl;
            ProofEngine engine(lib);
            tb.build_type_prover(sent)(engine);
            auto res = engine.get_proof_labels();
            cout << "Found type proof: " << tb.print_proof(res) << endl;
            cout << "Memory usage after test: " << size_to_string(getCurrentRSS()) << endl << endl;
        }

        vector< pwff > wffs = { /*pwff(new True()), pwff(new False()), pwff(new Not(pwff(new True()))), pwff(new Not(pwff(new False()))),
                                pwff(new Var("ph")), pwff(new Not(pwff(new Var("ph")))),
                                pwff(new Var("ps")), pwff(new Not(pwff(new Var("ps")))),
                                pwff(new Imp(pwff(new True()), pwff(new True()))),
                                pwff(new Imp(pwff(new True()), pwff(new False()))),
                                pwff(new Imp(pwff(new False()), pwff(new True()))),
                                pwff(new Imp(pwff(new False()), pwff(new False()))),
                                pwff(new Biimp(pwff(new True()), pwff(new True()))),
                                pwff(new Biimp(pwff(new True()), pwff(new False()))),
                                pwff(new Biimp(pwff(new False()), pwff(new True()))),
                                pwff(new Biimp(pwff(new False()), pwff(new False()))),
                                pwff(new And(pwff(new True()), pwff(new True()))),
                                pwff(new And(pwff(new True()), pwff(new False()))),
                                pwff(new And(pwff(new False()), pwff(new True()))),
                                pwff(new And(pwff(new False()), pwff(new False()))),*/
                                pwff(new And3(pwff(new True()), pwff(new True()), pwff(new True()))),
                                pwff(new And3(pwff(new True()), pwff(new False()), pwff(new True()))),
                                pwff(new And3(pwff(new False()), pwff(new True()), pwff(new True()))),
                                pwff(new And3(pwff(new False()), pwff(new False()), pwff(new True()))),
                                pwff(new And3(pwff(new True()), pwff(new True()), pwff(new False()))),
                                pwff(new And3(pwff(new True()), pwff(new False()), pwff(new False()))),
                                pwff(new And3(pwff(new False()), pwff(new True()), pwff(new False()))),
                                pwff(new And3(pwff(new False()), pwff(new False()), pwff(new False()))),
                                pwff(new Or(pwff(new True()), pwff(new True()))),
                                pwff(new Or(pwff(new True()), pwff(new False()))),
                                pwff(new Or(pwff(new False()), pwff(new True()))),
                                pwff(new Or(pwff(new False()), pwff(new False()))),
                                pwff(new Nand(pwff(new True()), pwff(new True()))),
                                pwff(new Nand(pwff(new True()), pwff(new False()))),
                                pwff(new Nand(pwff(new False()), pwff(new True()))),
                                pwff(new Nand(pwff(new False()), pwff(new False()))),
                                pwff(new Xor(pwff(new True()), pwff(new True()))),
                                pwff(new Xor(pwff(new True()), pwff(new False()))),
                                pwff(new Xor(pwff(new False()), pwff(new True()))),
                                pwff(new Xor(pwff(new False()), pwff(new False()))),
                                pwff(new Imp(pwff(new Var("ph")), pwff(new Var("ps")))),
                                pwff(new And(pwff(new Var("ph")), pwff(new Var("ps")))),
                                pwff(new And(pwff(new True()), pwff(new And(pwff(new Var("ph")), pwff(new False()))))),
                                pwff(new And(pwff(new False()), pwff(new And(pwff(new True()), pwff(new True()))))),
                              };

        if (true) {
            cout << "WFF type proving test" << endl;
            for (pwff &wff : wffs) {
                //wff->prove_type(lib, engine);
                cout << "WFF: " << wff->to_string() << endl;
                {
                    ProofEngine engine(lib);
                    wff->get_type_prover(tb)(engine);
                    if (engine.get_proof_labels().size() > 0) {
                        cout << "type proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
                        cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
                    }
                }
                cout << endl;
            }
        }

        if (true) {
            cout << "WFF proving test" << endl;
            for (pwff &wff : wffs) {
                cout << "WFF: " << wff->to_string() << endl;
                {
                    ProofEngine engine(lib);
                    wff->get_truth_prover(tb)(engine);
                    if (engine.get_proof_labels().size() > 0) {
                        cout << "Truth proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
                        cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
                    }
                }
                {
                    ProofEngine engine(lib);
                    wff->get_falsity_prover(tb)(engine);
                    if (engine.get_proof_labels().size() > 0) {
                        cout << "Falsity proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
                        cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
                    }
                }
                cout << endl;
            }
        }

        if (true) {
            cout << "WFF imp_not normal form test" << endl;
            for (pwff &wff : wffs) {
                cout << "WFF: " << wff->to_string() << endl;
                {
                    ProofEngine engine(lib);
                    wff->get_imp_not_prover(tb)(engine);
                    if (engine.get_proof_labels().size() > 0) {
                        cout << "imp_not proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
                        cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
                    }
                }
                cout << endl;
            }
        }

        if (true) {
            cout << "WFF subst test" << endl;
            for (pwff &wff : wffs) {
                cout << "WFF: " << wff->to_string() << endl;
                {
                    ProofEngine engine(lib);
                    wff->get_subst_prover("ph", true, tb)(engine);
                    if (engine.get_proof_labels().size() > 0) {
                        cout << "subst ph proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
                        cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
                    }
                }
                {
                    ProofEngine engine(lib);
                    wff->get_subst_prover("ph", false, tb)(engine);
                    if (engine.get_proof_labels().size() > 0) {
                        cout << "subst -. ph proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
                        cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
                    }
                }
                {
                    ProofEngine engine(lib);
                    wff->get_subst_prover("ps", true, tb)(engine);
                    if (engine.get_proof_labels().size() > 0) {
                        cout << "subst ps proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
                        cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
                    }
                }
                {
                    ProofEngine engine(lib);
                    wff->get_subst_prover("ps", false, tb)(engine);
                    if (engine.get_proof_labels().size() > 0) {
                        cout << "subst -. ps proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
                        cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
                    }
                }
                cout << endl;
            }
        }

        vector< pwff > wffs2 = { pwff(new True()), pwff(new False()), pwff(new Not(pwff(new True()))), pwff(new Not(pwff(new False()))),
                                 pwff(new Imp(pwff(new Var("ph")), pwff(new Var("ph")))),
                                 pwff(new Or3(pwff(new Var("ph")), pwff(new True()), pwff(new False()))),
                                 pwff(new Imp(pwff(new Var("ph")), pwff(new And3(pwff(new Var("ph")), pwff(new True()), pwff(new Var("ph")))))),
                                 pwff(new Biimp(pwff(new Nand(pwff(new Var("ph")), pwff(new Nand(pwff(new Var("ch")), pwff(new Var("ps")))))),
                                                pwff(new Imp(pwff(new Var("ph")), pwff(new And(pwff(new Var("ch")), pwff(new Var("ps")))))))),
                               };

        if (true) {
            cout << "WFF adv_truth test" << endl;
            for (pwff &wff : wffs2) {
                cout << "WFF: " << wff->to_string() << endl;
                {
                    ProofEngine engine(lib);
                    wff->get_adv_truth_prover(tb)(engine);
                    if (engine.get_proof_labels().size() > 0) {
                        cout << "adv truth proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
                        cout << "stack top: " << tb.print_sentence(engine.get_stack().back()) << endl;
                        cout << "proof length: " << engine.get_proof_labels().size() << endl;
                        UncompressedProof proof = engine.get_proof();
                    }
                }
                cout << endl;
            }
        }
    }

    auto tests = get_tests();
    // Comment or uncomment the following line to disable or enable tests with metamath-test
    //tests = {};
    int problems = 0;
    for (auto test_pair : tests) {
        string filename = test_pair.first;
        bool expect_success = test_pair.second;
        cout << "Testing file " << filename << " from " << test_basename << ", which is expected to " << (expect_success ? "pass" : "fail" ) << "..." << endl;

        // Useful for debugging
        /*if (filename == "demo0.mm") {
            mmpp_abort = true;
        } else {
            continue;
            mmpp_abort = false;
        }*/
        /*if (filename == "nf.mm") {
            break;
        }*/

        bool success = test_one(filename, expect_success);
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

    cout << "Maximum memory usage: " << size_to_string(getPeakRSS()) << endl;

}
