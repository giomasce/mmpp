
#include <iostream>

#include "mm/setmm.h"
#include "parsing/earley.h"
#include "parsing/lr.h"

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

    BOOST_TEST((bool)(earley_pt == lr_pt));

    //std::cout << "PT and PT2" << std::endl;
    ParsingTree2< SymType, LabType > pt2 = pt_to_pt2(lr_pt);
    ParsingTree< SymType, LabType > pt = pt2_to_pt(pt2);
    ParsingTree2< SymType, LabType > pt2_2 = pt_to_pt2(pt);
    BOOST_TEST((bool)(pt == lr_pt));
    BOOST_TEST((bool)(pt2 == pt2_2));
}

void test_grammar1() {
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

void test_grammar2() {
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

void test_grammar3() {
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

void test_grammar4() {
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

BOOST_AUTO_TEST_CASE(test_parsing) {
    test_grammar1();
    test_grammar2();
    test_grammar3();
    test_grammar4();
}
