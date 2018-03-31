
#include <ostream>
#include <functional>
#include <fstream>
#include <chrono>

#include "provers/wff.h"
#include "mm/reader.h"
#include "old/unification.h"
#include "utils/utils.h"
#include "parsing/earley.h"
#include "parsing/lr.h"
#include "utils/utils.h"
#include "platform.h"
#include "parsing/unif.h"
#include "mm/setmm.h"
#include "provers/wffsat.h"
#include "test.h"

#ifdef ENABLE_TEST_CODE

BOOST_AUTO_TEST_CASE(test_lr_on_setmm) {
    //std::cout << "LR parsing on set.mm" << std::endl;
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;
    std::function< std::ostream&(std::ostream&, SymTok) > sym_printer = [&](std::ostream &os, SymTok sym)->std::ostream& { return os << lib.resolve_symbol(sym); };
    std::function< std::ostream&(std::ostream&, LabTok) > lab_printer = [&](std::ostream &os, LabTok lab)->std::ostream& { return os << lib.resolve_label(lab); };
    const auto &derivations = tb.get_derivations();
    const auto ders_by_lab = compute_derivations_by_label(derivations);
    //EarleyParser< SymTok, LabTok > earley_parser(derivations);
    auto &lr_parser = tb.get_parser();

    for (const Assertion &ass : lib.get_assertions()) {
        if (!ass.is_valid() || !ass.is_theorem()) {
            continue;
        }
        //cout << lib.resolve_label(ass.get_thesis()) << endl;
        const Sentence &sent = lib.get_sentence(ass.get_thesis());
        Sentence sent2;
        copy(sent.begin() + 1, sent.end(), back_inserter(sent2));
        // The Earley parser is very slow and is not actually used in the code, so we avoid testing it
        /*auto earley_pt = earley_parser.parse(sent2, lib.get_symbol("wff"));
        BOOST_TEST(earley_pt.label != LabTok{});
        BOOST_TEST(reconstruct_sentence(earley_pt, derivations, ders_by_lab) == sent2);*/
        auto lr_pt = lr_parser.parse(sent2, lib.get_symbol("wff"));
        BOOST_TEST(lr_pt.label != LabTok{});
        BOOST_TEST(reconstruct_sentence(lr_pt, derivations, ders_by_lab) == sent2);
        //assert(earley_pt == lr_pt);
    }
}

BOOST_AUTO_TEST_CASE(test_tree_unification) {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;

    auto is_var = tb.get_standard_is_var();

    auto pt_templ = tb.parse_sentence(tb.read_sentence("( ps -> ph )"), lib.get_symbol("wff"));
    auto pt_sent = tb.parse_sentence(tb.read_sentence("( ph -> ( ps -> ch ) )"), lib.get_symbol("wff"));

    bool res;
    SubstMap< SymTok, LabTok > subst;
    UnilateralUnificator< SymTok, LabTok > unif(is_var);
    unif.add_parsing_trees(pt_templ, pt_sent);
    std::tie(res, subst) = unif.unify();
    BOOST_TEST(res);

    /*for (auto &i : subst) {
        std::cout << lib.resolve_symbol(lib.get_sentence(i.first).at(1)) << ": " << tb.print_sentence(tb.reconstruct_sentence(i.second), SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << std::endl;
    }*/
}

std::vector< std::pair< bool, std::string > > unification_data = {
    { true, "|- ( ( A e. CC /\\ B e. CC /\\ N e. NN0 ) -> ( ( A + B ) ^ N ) = sum_ k e. ( 0 ... N ) ( ( N _C k ) x. ( ( A ^ ( N - k ) ) x. ( B ^ k ) ) ) )" },
    { true, "|- ( ph -> ( ps <-> ps ) )" },
    { true, "|- ( ph -> ph )" },
    { false, "|-" },
    //{ false, "" },
    { false, "|- ph -> ph" },
};

BOOST_DATA_TEST_CASE(test_unification, boost::unit_test::data::make(unification_data)) {
    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;
    auto positive = sample.first;
    auto &str = sample.second;
    Sentence sent = tb.read_sentence(str);
    auto res = tb.unify_assertion({}, sent, false, true);
    BOOST_TEST(res.empty() != positive);
    /*std::cout << "Trying to unify " << test << std::endl;
    std::cout << "Found " << res.size() << " matching assertions:" << std::endl;
    for (auto &match : res) {
        auto &label = std::get<0>(match);
        const Assertion &ass = lib.get_assertion(label);
        std::cout << " * " << lib.resolve_label(label) << ":";
        for (auto &hyp : ass.get_ess_hyps()) {
            auto &hyp_sent = lib.get_sentence(hyp);
            std::cout << " & " << tb.print_sentence(hyp_sent, SentencePrinter::STYLE_ANSI_COLORS_SET_MM);
        }
        auto &thesis_sent = lib.get_sentence(ass.get_thesis());
        std::cout << " => " << tb.print_sentence(thesis_sent, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << std::endl;
    }*/
}

std::vector< std::pair< bool, std::string > > type_proving_data = {
    { true, "wff ( [_ suc z / z ]_ ( rec ( f , q ) ` z ) e. x <-> A. z ( z = suc z -> ( rec ( f , q ) ` z ) e. x ) )" },
    { false, "wff ph -> ps"},
};

BOOST_DATA_TEST_CASE(test_type_proving, boost::unit_test::data::make(type_proving_data)) {
    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;
    auto positive = sample.first;
    auto &str = sample.second;
    //std::cout << "Type proving test" << std::endl;
    auto sent = tb.read_sentence(str);
    BOOST_TEST(tb.print_sentence(sent, SentencePrinter::STYLE_PLAIN).to_string() == str);
    /*std::cout << "Sentence is " << tb.print_sentence(sent) << std::endl;
    std::cout << "HTML sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_HTML) << std::endl;
    std::cout << "Alt HTML sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_ALTHTML) << std::endl;
    std::cout << "LaTeX sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_LATEX) << std::endl;
    std::cout << "ANSI colors sentence is " << tb.print_sentence(sent, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << std::endl;*/

    CreativeProofEngineImpl< Sentence > engine(tb);
    auto res = tb.build_type_prover(sent)(engine);
    BOOST_TEST(res == positive);
    //std::cout << "Found type proof: " << tb.print_proof(engine.get_proof_labels()) << std::endl;
}

std::vector< std::tuple< bool, bool, bool, pwff > > wff_data = {
    { true, false, true, True::create() },
    { false, true, false, False::create() },
    { false, true, false, Not::create(True::create()) },
    { true, false, true, Not::create(False::create()) },
    { false, false, false, Var::create("ph") },
    { false, false, false, Not::create(Var::create("ph")) },
    { false, false, false, Var::create("ps") },
    { false, false, false, Not::create(Var::create("ps")) },
    { true, false, true, Imp::create(True::create(), True::create()) },
    { false, true, false, Imp::create(True::create(), False::create()) },
    { true, false, true, Imp::create(False::create(), True::create()) },
    { true, false, true, Imp::create(False::create(), False::create()) },
    { true, false, true, Biimp::create(True::create(), True::create()) },
    { false, true, false, Biimp::create(True::create(), False::create()) },
    { false, true, false, Biimp::create(False::create(), True::create()) },
    { true, false, true, Biimp::create(False::create(), False::create()) },
    { true, false, true, And::create(True::create(), True::create()) },
    { false, true, false, And::create(True::create(), False::create()) },
    { false, true, false, And::create(False::create(), True::create()) },
    { false, true, false, And::create(False::create(), False::create()) },
    { true, false, true, And3::create(True::create(), True::create(), True::create()) },
    { false, true, false, And3::create(True::create(), False::create(), True::create()) },
    { false, true, false, And3::create(False::create(), True::create(), True::create()) },
    { false, true, false, And3::create(False::create(), False::create(), True::create()) },
    { false, true, false, And3::create(True::create(), True::create(), False::create()) },
    { false, true, false, And3::create(True::create(), False::create(), False::create()) },
    { false, true, false, And3::create(False::create(), True::create(), False::create()) },
    { false, true, false, And3::create(False::create(), False::create(), False::create()) },
    { true, false, true, Or::create(True::create(), True::create()) },
    { true, false, true, Or::create(True::create(), False::create()) },
    { true, false, true, Or::create(False::create(), True::create()) },
    { false, true, false, Or::create(False::create(), False::create()) },
    { false, true, false, Nand::create(True::create(), True::create()) },
    { true, false, true, Nand::create(True::create(), False::create()) },
    { true, false, true, Nand::create(False::create(), True::create()) },
    { true, false, true, Nand::create(False::create(), False::create()) },
    { false, true, false, Xor::create(True::create(), True::create()) },
    { true, false, true, Xor::create(True::create(), False::create()) },
    { true, false, true, Xor::create(False::create(), True::create()) },
    { false, true, false, Xor::create(False::create(), False::create()) },
    { false, false, false, Imp::create(Var::create("ph"), Var::create("ps")) },
    { false, false, false, And::create(Var::create("ph"), Var::create("ps")) },
    { false, true, false, And::create(True::create(), And::create(Var::create("ph"), False::create())) },
    { false, true, false, And::create(False::create(), And::create(True::create(), True::create())) },
    { false, false, true, Imp::create(Var::create("ph"), Var::create("ph")) },
    { true, false, true, Or3::create(Var::create("ph"), True::create(), False::create()) },
    { false, false, true, Imp::create(Var::create("ph"), And3::create(Var::create("ph"), True::create(), Var::create("ph"))) },
    { false, false, true, Biimp::create(Nand::create(Var::create("ph"), Nand::create(Var::create("ch"), Var::create("ps"))), Imp::create(Var::create("ph"), And::create(Var::create("ch"), Var::create("ps")))) },
    { false, false, true, Imp::create(Var::create("ph"), And::create(Var::create("ph"), True::create())) },
    { false, false, false, Imp::create(Var::create("ph"), And::create(Var::create("ph"), False::create())) },
};

BOOST_DATA_TEST_CASE(test_wff_type_prover, boost::unit_test::data::make(wff_data), trivially_true, trivially_false, actually_true, wff) {
    (void) trivially_true;
    (void) trivially_false;
    (void) actually_true;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    wff->set_library_toolbox(tb);

    CreativeProofEngineImpl< Sentence > engine(tb);
    auto res = wff->get_type_prover(tb)(engine);
    BOOST_TEST(res);
    BOOST_TEST(engine.get_stack().size() == (size_t) 1);
    BOOST_TEST(engine.get_stack().back() == tb.reconstruct_sentence(pt2_to_pt(wff->to_parsing_tree(tb)), tb.get_turnstile_alias()));
}

BOOST_DATA_TEST_CASE(test_wff_truth_prover, boost::unit_test::data::make(wff_data), trivially_true, trivially_false, actually_true, wff) {
    (void) trivially_false;
    (void) actually_true;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    wff->set_library_toolbox(tb);

    CreativeProofEngineImpl< Sentence > engine(tb);
    auto res = wff->get_truth_prover(tb)(engine);
    BOOST_TEST(res == trivially_true);
    if (res) {
        BOOST_TEST(engine.get_stack().size() == (size_t) 1);
        BOOST_TEST(engine.get_stack().back() == tb.reconstruct_sentence(pt2_to_pt(wff->to_parsing_tree(tb)), tb.get_turnstile()));
    }
}

BOOST_DATA_TEST_CASE(test_wff_falsity_prover, boost::unit_test::data::make(wff_data), trivially_true, trivially_false, actually_true, wff) {
    (void) trivially_true;
    (void) actually_true;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    wff->set_library_toolbox(tb);

    CreativeProofEngineImpl< Sentence > engine(tb);
    auto res = wff->get_falsity_prover(tb)(engine);
    BOOST_TEST(res == trivially_false);
    if (res) {
        BOOST_TEST(engine.get_stack().size() == (size_t) 1);
        BOOST_TEST(engine.get_stack().back() == tb.reconstruct_sentence(pt2_to_pt(Not::create(wff)->to_parsing_tree(tb)), tb.get_turnstile()));
    }
}

BOOST_DATA_TEST_CASE(test_wff_adv_truth_prover, boost::unit_test::data::make(wff_data), trivially_true, trivially_false, actually_true, wff) {
    (void) trivially_true;
    (void) trivially_false;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    wff->set_library_toolbox(tb);

    CreativeProofEngineImpl< Sentence > engine(tb);
    auto res = wff->get_adv_truth_prover(tb);
    BOOST_TEST(res.first == actually_true);
    auto res2 = res.second(engine);
    BOOST_TEST(res2 == actually_true);
    if (res2) {
        BOOST_TEST(engine.get_stack().size() == (size_t) 1);
        BOOST_TEST(engine.get_stack().back() == tb.reconstruct_sentence(pt2_to_pt(wff->to_parsing_tree(tb)), tb.get_turnstile()));
    }
}

BOOST_DATA_TEST_CASE(test_wff_minisat_prover, boost::unit_test::data::make(wff_data), trivially_true, trivially_false, actually_true, wff) {
    (void) trivially_true;
    (void) trivially_false;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    wff->set_library_toolbox(tb);

    CreativeProofEngineImpl< Sentence > engine(tb);
    auto res = get_sat_prover(wff, tb);
    BOOST_TEST(res.first == actually_true);
    auto res2 = res.second(engine);
    BOOST_TEST(res2 == actually_true);
    if (res2) {
        BOOST_TEST(engine.get_stack().size() == (size_t) 1);
        BOOST_TEST(engine.get_stack().back() == tb.reconstruct_sentence(pt2_to_pt(wff->to_parsing_tree(tb)), tb.get_turnstile()));
    }
}

std::vector< std::string > wff_from_pt_data = {
    "wff T.",
    "wff F.",
    "wff ph",
    "wff ps",
    "wff -. ph",
    "wff ( ph -> ps )",
    "wff ( ph /\\ ps )",
    "wff ( ph \\/ ps )",
    "wff ( ph <-> F. )",
    "wff ( ph -/\\ ps )",
    "wff ( ph \\/_ ps )",
    "wff ( ph /\\ ps /\\ ch )",
    "wff ( ph \\/ ps \\/ ch )",
    "wff A. x ph",
    "wff -. A. x ph",
    "wff -. A. x ( ph <-> F. )",
    "wff ( A. x ( ps -> ph ) <-> -. F. )",
};

typedef ParsingTree<SymTok, LabTok> t1;
BOOST_TEST_DONT_PRINT_LOG_VALUE(t1);

BOOST_DATA_TEST_CASE(test_wff_from_pt, boost::unit_test::data::make(wff_from_pt_data)) {
    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto sent = tb.read_sentence(sample);
    auto pt = tb.parse_sentence(sent);
    auto parsed = wff_from_pt(pt, tb);
    BOOST_TEST(pt2_to_pt(parsed->to_parsing_tree(tb)) == pt);
}

#endif
