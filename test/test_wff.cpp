
#include "test/test.h"
#include "provers/wff.h"
#include "provers/wffsat.h"
#include "mm/setmm.h"

#ifdef ENABLE_TEST_CODE

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

using WffDataTuple = std::tuple< bool, bool, bool, pwff >;

std::vector< WffDataTuple > wff_data = {
    WffDataTuple{ true, false, true, True::create() },
    WffDataTuple{ false, true, false, False::create() },
    WffDataTuple{ false, true, false, Not::create(True::create()) },
    WffDataTuple{ true, false, true, Not::create(False::create()) },
    WffDataTuple{ false, false, false, Var::create("ph") },
    WffDataTuple{ false, false, false, Not::create(Var::create("ph")) },
    WffDataTuple{ false, false, false, Var::create("ps") },
    WffDataTuple{ false, false, false, Not::create(Var::create("ps")) },
    WffDataTuple{ true, false, true, Imp::create(True::create(), True::create()) },
    WffDataTuple{ false, true, false, Imp::create(True::create(), False::create()) },
    WffDataTuple{ true, false, true, Imp::create(False::create(), True::create()) },
    WffDataTuple{ true, false, true, Imp::create(False::create(), False::create()) },
    WffDataTuple{ true, false, true, Biimp::create(True::create(), True::create()) },
    WffDataTuple{ false, true, false, Biimp::create(True::create(), False::create()) },
    WffDataTuple{ false, true, false, Biimp::create(False::create(), True::create()) },
    WffDataTuple{ true, false, true, Biimp::create(False::create(), False::create()) },
    WffDataTuple{ true, false, true, And::create(True::create(), True::create()) },
    WffDataTuple{ false, true, false, And::create(True::create(), False::create()) },
    WffDataTuple{ false, true, false, And::create(False::create(), True::create()) },
    WffDataTuple{ false, true, false, And::create(False::create(), False::create()) },
    WffDataTuple{ true, false, true, And3::create(True::create(), True::create(), True::create()) },
    WffDataTuple{ false, true, false, And3::create(True::create(), False::create(), True::create()) },
    WffDataTuple{ false, true, false, And3::create(False::create(), True::create(), True::create()) },
    WffDataTuple{ false, true, false, And3::create(False::create(), False::create(), True::create()) },
    WffDataTuple{ false, true, false, And3::create(True::create(), True::create(), False::create()) },
    WffDataTuple{ false, true, false, And3::create(True::create(), False::create(), False::create()) },
    WffDataTuple{ false, true, false, And3::create(False::create(), True::create(), False::create()) },
    WffDataTuple{ false, true, false, And3::create(False::create(), False::create(), False::create()) },
    WffDataTuple{ true, false, true, Or::create(True::create(), True::create()) },
    WffDataTuple{ true, false, true, Or::create(True::create(), False::create()) },
    WffDataTuple{ true, false, true, Or::create(False::create(), True::create()) },
    WffDataTuple{ false, true, false, Or::create(False::create(), False::create()) },
    WffDataTuple{ false, true, false, Nand::create(True::create(), True::create()) },
    WffDataTuple{ true, false, true, Nand::create(True::create(), False::create()) },
    WffDataTuple{ true, false, true, Nand::create(False::create(), True::create()) },
    WffDataTuple{ true, false, true, Nand::create(False::create(), False::create()) },
    WffDataTuple{ false, true, false, Xor::create(True::create(), True::create()) },
    WffDataTuple{ true, false, true, Xor::create(True::create(), False::create()) },
    WffDataTuple{ true, false, true, Xor::create(False::create(), True::create()) },
    WffDataTuple{ false, true, false, Xor::create(False::create(), False::create()) },
    WffDataTuple{ false, false, false, Imp::create(Var::create("ph"), Var::create("ps")) },
    WffDataTuple{ false, false, false, And::create(Var::create("ph"), Var::create("ps")) },
    WffDataTuple{ false, true, false, And::create(True::create(), And::create(Var::create("ph"), False::create())) },
    WffDataTuple{ false, true, false, And::create(False::create(), And::create(True::create(), True::create())) },
    WffDataTuple{ false, false, true, Imp::create(Var::create("ph"), Var::create("ph")) },
    WffDataTuple{ true, false, true, Or3::create(Var::create("ph"), True::create(), False::create()) },
    WffDataTuple{ false, false, true, Imp::create(Var::create("ph"), And3::create(Var::create("ph"), True::create(), Var::create("ph"))) },
    WffDataTuple{ false, false, true, Biimp::create(Nand::create(Var::create("ph"), Nand::create(Var::create("ch"), Var::create("ps"))), Imp::create(Var::create("ph"), And::create(Var::create("ch"), Var::create("ps")))) },
    WffDataTuple{ false, false, true, Imp::create(Var::create("ph"), And::create(Var::create("ph"), True::create())) },
    WffDataTuple{ false, false, false, Imp::create(Var::create("ph"), And::create(Var::create("ph"), False::create())) },
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
    auto parsed = wff_from_pt<PropTag>(pt, tb);
    BOOST_TEST(pt2_to_pt(parsed->to_parsing_tree(tb)) == pt);
}

BOOST_DATA_TEST_CASE(test_pred_wff_from_pt, boost::unit_test::data::make(wff_from_pt_data)) {
    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto sent = tb.read_sentence(sample);
    auto pt = tb.parse_sentence(sent);
    auto parsed = wff_from_pt<PredTag>(pt, tb);
    BOOST_TEST(pt2_to_pt(parsed->to_parsing_tree(tb)) == pt);
}

#endif
