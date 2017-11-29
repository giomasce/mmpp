
#include <iostream>

#include "test/test_parsing.h"

#include "test/test_env.h"
#include "parsing/earley.h"
#include "parsing/lr.h"

using namespace std;

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

    cout << "PT and PT2" << endl;
    ParsingTree2< SymType, LabType > pt2 = pt_to_pt2(lr_pt);
    ParsingTree< SymType, LabType > pt = pt2_to_pt(pt2);
    ParsingTree2< SymType, LabType > pt2_2 = pt_to_pt2(pt);
    assert(pt == lr_pt);
    assert(pt2 == pt2_2);
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

void test_parsers() {
    cout << "Generic parser test" << endl;
    test_grammar1();
    test_grammar2();
    test_grammar3();
    test_grammar4();
}
