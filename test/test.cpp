
#include <ostream>
#include <functional>
#include <fstream>
#include <chrono>

#include "wff.h"
#include "reader.h"
#include "old/unification.h"
#include "utils/utils.h"
#include "parsing/earley.h"
#include "parsing/lr.h"
#include "utils/utils.h"
#include "platform.h"
#include "parsing/unif.h"
#include "test/test_env.h"
#include "test/test_parsing.h"
#include "test/test_verification.h"
#include "test/test_minor.h"

using namespace std;
using namespace chrono;

void test_old_unification() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;
    cout << "Generic unification test" << endl;
    vector< SymTok > sent = tb.read_sentence("wff ( ph -> ( ps -> ch ) )");
    vector< SymTok > templ = tb.read_sentence("wff ( th -> et )");
    auto res = unify_old(sent, templ, lib, false);
    cout << "Matching:         " << tb.print_sentence(sent) << endl << "against template: " << tb.print_sentence(templ) << endl;
    for (auto &match : res) {
        cout << "  *";
        for (auto &var: match) {
            cout << " " << tb.print_sentence(Sentence({var.first})) << " => " << tb.print_sentence(var.second) << "  ";
        }
        cout << endl;
    }
    cout << "Memory usage after test: " << size_to_string(platform_get_current_rss()) << endl << endl;
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

    auto is_var = tb.get_standard_is_var();

    auto pt_templ = tb.parse_sentence(tb.read_sentence("( ps -> ph )"), lib.get_symbol("wff"));
    auto pt_sent = tb.parse_sentence(tb.read_sentence("( ph -> ( ps -> ch ) )"), lib.get_symbol("wff"));

    bool res;
    unordered_map< LabTok, ParsingTree< SymTok, LabTok > > subst;
    UnilateralUnificator< SymTok, LabTok > unif(is_var);
    unif.add_parsing_trees(pt_templ, pt_sent);
    tie(res, subst) = unif.unify();

    for (auto &i : subst) {
        cout << lib.resolve_symbol(lib.get_sentence(i.first).at(1)) << ": " << tb.print_sentence(tb.reconstruct_sentence(i.second)) << endl;
    }
}

void test_unification() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;

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
        Tic t = tic();
        for (int i = 0; i < reps; i++) {
            res2 = tb.unify_assertion({}, sent, false, true);
        }
        toc(t, reps);
    }
}

void test_type_proving() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;
    int reps = 10;
    cout << "Type proving test" << endl;
    auto sent = tb.read_sentence(  "wff ( [_ suc z / z ]_ ( rec ( f , q ) ` z ) e. x <-> A. z ( z = suc z -> ( rec ( f , q ) ` z ) e. x ) )");
    cout << "Sentence is " << tb.print_sentence(sent) << endl;
    cout << "HTML sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_HTML) << endl;
    cout << "Alt HTML sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_ALTHTML) << endl;
    cout << "LaTeX sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_LATEX) << endl;

    ProofEngine engine(lib);
    tb.build_type_prover(sent)(engine);
    auto res = engine.get_proof_labels();
    cout << "Found type proof: " << tb.print_proof(res) << endl;
    Tic t = tic();
    for (int i = 0; i < reps; i++) {
        ProofEngine engine(lib);
        tb.build_type_prover(sent)(engine);
    }
    toc(t, reps);

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
    vector< pwff > wffs = { pwff(new True()), pwff(new False()), pwff(new Not(pwff(new True()))), pwff(new Not(pwff(new False()))),
                             pwff(new Imp(pwff(new Var("ph")), pwff(new Var("ph")))),
                             pwff(new Or3(pwff(new Var("ph")), pwff(new True()), pwff(new False()))),
                             pwff(new Imp(pwff(new Var("ph")), pwff(new And3(pwff(new Var("ph")), pwff(new True()), pwff(new Var("ph")))))),
                             pwff(new Biimp(pwff(new Nand(pwff(new Var("ph")), pwff(new Nand(pwff(new Var("ch")), pwff(new Var("ps")))))),
                                            pwff(new Imp(pwff(new Var("ph")), pwff(new And(pwff(new Var("ch")), pwff(new Var("ps")))))))),
                           };

    if (true) {
        cout << "WFF adv_truth test" << endl;
        for (pwff &wff : wffs) {
            cout << "WFF: " << wff->to_string() << endl;
            {
                ProofEngine engine(lib);
                wff->get_adv_truth_prover(tb)(engine);
                if (engine.get_proof_labels().size() > 0) {
                    //cout << "adv truth proof: " << tb.print_proof(engine.get_proof_labels()) << endl;
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
    test_small_stuff();
    test_parsers();
    test_lr_set();
    test_old_unification();
    test_statement_unification();
    test_tree_unification();
    test_type_proving();
    test_unification();
    test_wffs_trivial();
    test_wffs_advanced();
    cout << "Maximum memory usage: " << size_to_string(platform_get_peak_rss()) << endl;
}

int test_setmm(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    test();
    return 0;
}
static_block {
    register_main_function("mmpp_test_setmm", test_setmm);
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
    register_main_function("mmpp_verify_one", test_one_main);
}

int test_simple_one_main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Provide file name as argument, please" << endl;
        return 1;
    }
    string filename(argv[1]);
    return test_verification_one(filename, false) ? 0 : 1;
}
static_block {
    register_main_function("mmpp_simple_verify_one", test_simple_one_main);
}

int test_all_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    test_all_verifications();

    return 0;
}
static_block {
    register_main_function("mmpp_verify_all", test_all_main);
}
