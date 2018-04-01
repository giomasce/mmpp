
#include <iostream>

#include "mm/setmm.h"
#include "parsing/earley.h"
#include "parsing/lr.h"
#include "test.h"

#ifdef ENABLE_TEST_CODE

typedef ParsingTree<std::string, size_t> t1;
typedef ParsingTree2<std::string, size_t> t2;
typedef ParsingTree<char, size_t> t3;
typedef ParsingTree2<char, size_t> t4;

BOOST_TEST_DONT_PRINT_LOG_VALUE(t1);
BOOST_TEST_DONT_PRINT_LOG_VALUE(t2);
BOOST_TEST_DONT_PRINT_LOG_VALUE(t3);
BOOST_TEST_DONT_PRINT_LOG_VALUE(t4);

template< typename SymType, typename LabType >
void test_parsers(const std::vector<SymType> &sent, SymType type, const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations) {
    const auto ders_by_lab = compute_derivations_by_label(derivations);

    //std::cout << "Earley parser" << std::endl;
    EarleyParser< SymType, LabType > earley_parser(derivations);
    ParsingTree< SymType, LabType > earley_pt = earley_parser.parse(sent, type);
    BOOST_TEST(earley_pt.label != LabType{});
    BOOST_TEST(reconstruct_sentence(earley_pt, derivations, ders_by_lab) == sent);

    //std::cout << "LR parser" << std::endl;
    LRParser< SymType, LabType > lr_parser(derivations);
    lr_parser.initialize();

    ParsingTree< SymType, LabType > lr_pt = lr_parser.parse(sent, type);
    BOOST_TEST(lr_pt.label != LabType{});
    BOOST_TEST(reconstruct_sentence(lr_pt, derivations, ders_by_lab) == sent);

    BOOST_TEST(earley_pt == lr_pt);

    //std::cout << "PT and PT2" << std::endl;
    ParsingTree2< SymType, LabType > pt2 = pt_to_pt2(lr_pt);
    ParsingTree< SymType, LabType > pt = pt2_to_pt(pt2);
    ParsingTree2< SymType, LabType > pt2_2 = pt_to_pt2(pt);
    BOOST_TEST(pt == lr_pt);
    BOOST_TEST(pt2 == pt2_2);
}

BOOST_AUTO_TEST_CASE(test_parsing1) {
    /* Describe the grammar at http://loup-vaillant.fr/tutorials/earley-parsing/recogniser.
     * Only digit up to 4 are used to keep tables small and debugging easy.
     */
    std::unordered_map<std::string, std::vector<std::pair< size_t, std::vector<std::string> > > > derivations;
    derivations["Sum"].push_back(std::make_pair(100, std::vector< std::string >({ "Sum", "+", "Product" })));
    derivations["Sum"].push_back(std::make_pair(101, std::vector< std::string >({ "Sum", "-", "Product" })));
    derivations["Sum"].push_back(std::make_pair(102, std::vector< std::string >({ "Product" })));
    derivations["Product"].push_back(std::make_pair(103, std::vector< std::string >({ "Product", "*", "Product" })));
    derivations["Product"].push_back(std::make_pair(104, std::vector< std::string >({ "Product", "/", "Product" })));
    derivations["Product"].push_back(std::make_pair(105, std::vector< std::string >({ "Factor" })));
    derivations["Factor"].push_back(std::make_pair(106, std::vector< std::string >({ "(", "Sum", ")" })));
    derivations["Factor"].push_back(std::make_pair(107, std::vector< std::string >({ "Number" })));
    derivations["Number"].push_back(std::make_pair(108, std::vector< std::string >({ "0", "Number" })));
    derivations["Number"].push_back(std::make_pair(109, std::vector< std::string >({ "1", "Number" })));
    derivations["Number"].push_back(std::make_pair(110, std::vector< std::string >({ "2", "Number" })));
    derivations["Number"].push_back(std::make_pair(111, std::vector< std::string >({ "3", "Number" })));
    derivations["Number"].push_back(std::make_pair(112, std::vector< std::string >({ "4", "Number" })));
    derivations["Number"].push_back(std::make_pair(113, std::vector< std::string >({ "0" })));
    derivations["Number"].push_back(std::make_pair(114, std::vector< std::string >({ "1" })));
    derivations["Number"].push_back(std::make_pair(115, std::vector< std::string >({ "2" })));
    derivations["Number"].push_back(std::make_pair(116, std::vector< std::string >({ "3" })));
    derivations["Number"].push_back(std::make_pair(117, std::vector< std::string >({ "4" })));
    std::vector< std::string > sent = { "1", "+", "(", "2", "*", "3", "-", "4", ")" };
    test_parsers< std::string, size_t >(sent, "Sum", derivations);
}

BOOST_AUTO_TEST_CASE(test_parsing2) {
    /* Describe the grammar at https://web.cs.dal.ca/~sjackson/lalr1.html.
     */
    std::unordered_map<char, std::vector<std::pair< size_t, std::vector<char> > > > derivations;
    derivations['S'].push_back(std::make_pair(1, std::vector< char >({ 'N' })));
    derivations['N'].push_back(std::make_pair(2, std::vector< char >({ 'V', '=', 'E' })));
    derivations['N'].push_back(std::make_pair(3, std::vector< char >({ 'E' })));
    derivations['E'].push_back(std::make_pair(4, std::vector< char >({ 'V' })));
    derivations['V'].push_back(std::make_pair(5, std::vector< char >({ 'x' })));
    derivations['V'].push_back(std::make_pair(6, std::vector< char >({ '*', 'E' })));
    std::vector< char > sent = { 'x', '=', '*', 'x' };
    test_parsers< char, size_t >(sent, 'S', derivations);
}

BOOST_AUTO_TEST_CASE(test_parsing3) {
    /* Describe the grammar at https://en.wikipedia.org/wiki/LR_parser.
     */
    std::unordered_map<std::string, std::vector<std::pair< size_t, std::vector<std::string> > > > derivations;
    derivations["Goal"].push_back(std::make_pair(1, std::vector< std::string >({ "Sums" })));
    derivations["Sums"].push_back(std::make_pair(2, std::vector< std::string >({ "Sums", "+", "Products" })));
    derivations["Sums"].push_back(std::make_pair(3, std::vector< std::string >({ "Products" })));
    derivations["Products"].push_back(std::make_pair(4, std::vector< std::string >({ "Products", "*", "Value" })));
    derivations["Products"].push_back(std::make_pair(5, std::vector< std::string >({ "Value" })));
    derivations["Value"].push_back(std::make_pair(6, std::vector< std::string >({ "int" })));
    derivations["Value"].push_back(std::make_pair(7, std::vector< std::string >({ "id" })));
    std::vector< std::string > sent = { "id", "*", "int", "+", "int" };
    test_parsers< std::string, size_t >(sent, "Goal", derivations);
}

BOOST_AUTO_TEST_CASE(test_parsing4) {
    /* Describe the grammar at https://dickgrune.com/Books/PTAPG_1st_Edition/BookBody.pd, page 201.
     */
    std::unordered_map<char, std::vector<std::pair< size_t, std::vector<char> > > > derivations;
    derivations['S'].push_back(std::make_pair(1, std::vector< char >({ 'E' })));
    derivations['E'].push_back(std::make_pair(2, std::vector< char >({ 'E', '-', 'T' })));
    derivations['E'].push_back(std::make_pair(3, std::vector< char >({ 'T' })));
    derivations['T'].push_back(std::make_pair(4, std::vector< char >({ 'n' })));
    derivations['T'].push_back(std::make_pair(5, std::vector< char >({ '(', 'E', ')' })));
    std::vector< char > sent = { 'n', '-', 'n', '-', 'n' };
    test_parsers< char, size_t >(sent, 'S', derivations);
}

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

#endif
