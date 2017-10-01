#include "test.h"

#include <ostream>
#include <functional>
#include <fstream>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "wff.h"
#include "reader.h"
#include "unification.h"
#include "memory.h"
#include "utils.h"
#include "earley.h"
#include "lr.h"
#include "utils.h"
#include "platform.h"
#include "unif.h"

using namespace std;

const boost::filesystem::path test_basename = "../metamath-test";
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

bool test_verification_one(string filename, bool advanced_tests) {
    bool success = true;
    try {
        cout << "Memory usage when starting: " << size_to_string(platform_get_current_rss()) << endl;
        FileTokenizer ft(test_basename / filename);
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

        cout << "Finished. Memory usage: " << size_to_string(platform_get_current_rss()) << endl;
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

void test_all_verifications() {
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

        bool success = test_verification_one(filename, expect_success);
        if (success) {
            if (expect_success) {
                cout << "Good, it worked!" << endl;
            } else {
                cout << "Bad, it should have failed!" << endl;
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
}

template< typename SymType, typename LabType >
void test_parsers(const std::vector<SymType> &sent, SymType type, const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations) {
    const auto ders_by_lab = compute_derivations_by_label(derivations);

    cout << "Earley parser" << endl;
    EarleyParser< SymType, LabType > earley_parser(derivations);
    ParsingTree< SymType, LabType > earley_pt = earley_parser.parse(sent, type);
    assert(earley_pt.label != 0);
    assert(reconstruct_sentence(earley_pt, derivations, ders_by_lab) == sent);

    cout << "LR parser" << endl;
    LRParser lr_parser(derivations);
    lr_parser.initialize();

    ParsingTree< SymType, LabType > lr_pt = lr_parser.parse(sent, type);
    assert(lr_pt.label != 0);
    assert(reconstruct_sentence(lr_pt, derivations, ders_by_lab) == sent);

    assert(earley_pt == lr_pt);
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

void test_lr_set() {
    cout << "LR parsing on set.mm" << endl;
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;
    std::function< std::ostream&(std::ostream&, SymTok) > sym_printer = [&](ostream &os, SymTok sym)->ostream& { return os << lib.resolve_symbol(sym); };
    std::function< std::ostream&(std::ostream&, LabTok) > lab_printer = [&](ostream &os, LabTok lab)->ostream& { return os << lib.resolve_label(lab); };
    const auto &derivations = tb.get_derivations();
    const auto ders_by_lab = compute_derivations_by_label(derivations);
    //EarleyParser earley_parser(derivations);
    auto &lr_parser = tb.get_parser();

    Tic t = tic();
    int reps = 0;
    LabTok quartfull_lab = lib.get_label("quartfull");
    for (const Assertion &ass : lib.get_assertions()) {
        if (!ass.is_valid() || !ass.is_theorem()) {
            continue;
        }
        if (ass.get_thesis() == quartfull_lab) {
            continue;
        }
        //cout << lib.resolve_label(ass.get_thesis()) << endl;
        reps++;
        const Sentence &sent = lib.get_sentence(ass.get_thesis());
        Sentence sent2;
        copy(sent.begin() + 1, sent.end(), back_inserter(sent2));
        /*auto earley_pt = earley_parser.parse(sent2, lib.get_symbol("wff"));
        assert(earley_pt.label != 0);
        assert(reconstruct_sentence(earley_pt, derivations, ders_by_lab) == sent2);*/
        auto lr_pt = lr_parser.parse(sent2, lib.get_symbol("wff"));
        assert(lr_pt.label != 0);
        assert(reconstruct_sentence(lr_pt, derivations, ders_by_lab) == sent2);
        //assert(earley_pt == lr_pt);
    }
    toc(t, reps);
}

void test_small_stuff() {
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

    cout << "Finished. Memory usage: " << size_to_string(platform_get_current_rss()) << endl << endl;
}

void test_parsers() {
    cout << "Generic parser test" << endl;
    test_grammar1();
    test_grammar2();
    test_grammar3();
    test_grammar4();
}

void test_unification() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;
    cout << "Generic unification test" << endl;
    vector< SymTok > sent = tb.read_sentence("wff ( ph -> ( ps -> ch ) )");
    vector< SymTok > templ = tb.read_sentence("wff ( th -> et )");
    auto res = unify(sent, templ, lib, false);
    cout << "Matching:         " << tb.print_sentence(sent) << endl << "against template: " << tb.print_sentence(templ) << endl;
    for (auto &match : res) {
        cout << "  *";
        for (auto &var: match) {
            cout << " " << tb.print_sentence({var.first}) << " => " << tb.print_sentence(var.second) << "  ";
        }
        cout << endl;
    }
    cout << "Memory usage after test: " << size_to_string(platform_get_current_rss()) << endl << endl;
}

void test_statement_unification() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;
    cout << "Statement unification test" << endl;
    //auto res = lib.unify_assertion({ parse_sentence("|- ( ch -> th )", lib), parse_sentence("|- ch", lib) }, parse_sentence("|- th", lib));
    auto res = tb.unify_assertion({ tb.read_sentence("|- ( ch -> ( ph -> ps ) )"), tb.read_sentence("|- ch") }, tb.read_sentence("|- ( ph -> ps )"));
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
    cout << "Memory usage after test: " << size_to_string(platform_get_current_rss()) << endl << endl;
}

void test_tree_unification() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;

    std::function< bool(LabTok) > is_var = [&lib](LabTok x)->bool { return !lib.is_constant(lib.get_sentence(x).at(1)); };

    auto pt_templ = tb.parse_sentence(tb.read_sentence("( ps -> ph )"), lib.get_symbol("wff"));
    auto pt_sent = tb.parse_sentence(tb.read_sentence("( ph -> ( ps -> ch ) )"), lib.get_symbol("wff"));

    bool res;
    unordered_map< LabTok, ParsingTree< SymTok, LabTok > > subst;
    tie(res, subst) = unify(pt_templ, pt_sent, is_var);

    for (auto &i : subst) {
        cout << lib.resolve_symbol(lib.get_sentence(i.first).at(1)) << ": " << tb.print_sentence(tb.reconstruct_sentence(i.second)) << endl;
    }
}

void test_type_proving() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;
    cout << "Type proving test" << endl;
    auto sent = tb.read_sentence(  "wff ( [_ suc z / z ]_ ( rec ( f , q ) ` z ) e. x <-> A. z ( z = suc z -> ( rec ( f , q ) ` z ) e. x ) )");
    cout << "Sentence is " << tb.print_sentence(sent) << endl;
    cout << "HTML sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_HTML) << endl;
    cout << "Alt HTML sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_ALTHTML) << endl;
    cout << "LaTeX sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_LATEX) << endl;
    ProofEngine engine(lib);
    tb.build_type_prover(sent)(engine);
    auto res = engine.get_proof_labels();
    cout << "Found type proof (classical): " << tb.print_proof(res) << endl;
    ProofEngine engine2(lib);
    tb.build_earley_type_prover(sent)(engine2);
    res = engine2.get_proof_labels();
    cout << "Found type proof (Earley):    " << tb.print_proof(res) << endl;
    cout << "Memory usage after test: " << size_to_string(platform_get_current_rss()) << endl << endl;
}

void test_wffs_trivial() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;
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
}

void test_wffs_advanced() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;
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

void test() {
    /*test_small_stuff();
    test_parsers();
    test_lr_set();
    test_unification();
    test_statement_unification();*/
    test_tree_unification();
    /*test_type_proving();
    test_wffs_trivial();
    test_wffs_advanced();
    test_all_verifications();*/
    cout << "Maximum memory usage: " << size_to_string(platform_get_peak_rss()) << endl;
}

int test_all_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    test();
    return 0;
}
static_block {
    register_main_function("mmpp_test_all", test_all_main);
}

int test_one_main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Provide file name as argument, please" << endl;
        return 1;
    }
    string filename(argv[1]);
    return test_verification_one(filename, true) ? 0 : 1;
}
static_block {
    register_main_function("mmpp_test_one", test_one_main);
}

TestEnvironmentInner::TestEnvironmentInner(const string &filename, const string &cache_filename)
{
    cout << "Reading database from file " << filename << " using cache in file " << cache_filename << endl;
    FileTokenizer ft(filename);
    Reader p(ft, false, true);
    p.run();
    ft.compute_digest();
    this->lib_digest = new string(ft.get_digest());
    this->lib = new LibraryImpl(p.get_library());
    shared_ptr< ToolboxCache > cache = make_shared< FileToolboxCache >(cache_filename);
    this->tb = new LibraryToolbox(*this->lib, "|-", true, cache);
    cout << this->lib->get_symbols_num() << " symbols and " << this->lib->get_labels_num() << " labels" << endl;
    cout << "Memory usage after loading the library: " << size_to_string(platform_get_current_rss()) << endl << endl;
}

TestEnvironmentInner::~TestEnvironmentInner() {
    delete this->lib;
    delete this->tb;
    delete this->lib_digest;
}

TestEnvironment::TestEnvironment(const string &filename, const string &cache_filename) :
    inner(filename, cache_filename), lib(*inner.lib), tb(*inner.tb), lib_digest(*inner.lib_digest)
{
}

const TestEnvironment &get_set_mm() {
    static TestEnvironment data("../set.mm/set.mm", "../set.mm/set.mm.cache");
    return data;
}
