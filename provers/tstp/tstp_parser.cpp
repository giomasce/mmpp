
#include "tstp_parser.h"

#include <string>
#include <ostream>
#include <limits>
#include <vector>
#include <utility>
#include <unordered_map>
#include <iostream>

#include "parsing/lr.h"
#include "parsing/earley.h"
#include "utils/utils.h"

namespace gio {
namespace mmpp {
namespace tstp {

const std::string ID_LETTERS = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890_$'.";
const std::string SPACE_LETTERS = " \t\n\r";

Token char_tok(char c) { return Token(TokenType::CHAR, c); }
Token sym_tok(TokenType type) { return Token(type); }

char letter_to_char(const ParsingTree<Token, Rule> &pt) {
    assert(pt.type == sym_tok(TokenType::LETTER));
    int diff = static_cast<int>(pt.label) - static_cast<int>(Rule::CHAR_IS_LETTER);
    assert(0 <= diff && diff < 0x100);
    return static_cast<char>(diff);
}

std::string reconstruct_id(const ParsingTree<Token, Rule> &pt) {
    assert(pt.type == sym_tok(TokenType::ID));
    std::ostringstream ss;
    const auto *cur = &pt;
    while (true) {
        if (cur->label == Rule::LETTER_IS_ID) {
            assert(cur->children.size() == 1);
            ss << letter_to_char(cur->children[0]);
            return ss.str();
        }
        assert(cur->label == Rule::LETTER_AND_ID_IS_ID);
        assert(cur->children.size() == 2);
        ss << letter_to_char(cur->children[0]);
        cur = &cur->children[1];
    }
}

std::ostream &operator<<(std::ostream &stream, const TokenType &tt) {
    return stream << static_cast< int >(tt);
}

std::ostream &operator<<(std::ostream &stream, const Rule &rule) {
    return stream << static_cast< int >(rule);
}

std::ostream &operator<<(std::ostream &stream, const Token &tok) {
    return stream << "[" << tok.type << "," << tok.content << "]";
}

void make_rule(std::unordered_map<Token, std::vector<std::pair<Rule, std::vector<Token> > > > &ders, TokenType type, Rule rule, std::vector< Token > tokens) {
    ders[type].push_back(std::make_pair(rule, tokens));
}

std::unordered_map<Token, std::vector<std::pair<Rule, std::vector<Token> > > > create_derivations() {
    std::unordered_map<Token, std::vector<std::pair<Rule, std::vector<Token> > > > ders;
    for (char c : ID_LETTERS) {
        auto uc = static_cast<unsigned char>(c);
        make_rule(ders, TokenType::LETTER, static_cast<Rule>(static_cast<int>(Rule::CHAR_IS_LETTER) + uc), {char_tok(c)});
    }
    for (char c : SPACE_LETTERS) {
        auto uc = static_cast<unsigned char>(c);
        make_rule(ders, TokenType::LETTER, static_cast<Rule>(static_cast<int>(Rule::CHAR_IS_WHITESPACE) + uc), {char_tok(c)});
    }
    make_rule(ders, TokenType::ID, Rule::LETTER_IS_ID, {sym_tok(TokenType::LETTER)});
    make_rule(ders, TokenType::ID, Rule::LETTER_AND_ID_IS_ID, {sym_tok(TokenType::LETTER), sym_tok(TokenType::ID)});

    // Rules for CNFs
    make_rule(ders, TokenType::TERM, Rule::ID_IS_TERM, {sym_tok(TokenType::ID)});
    make_rule(ders, TokenType::ARGLIST, Rule::TERM_IS_ARGLIST, {sym_tok(TokenType::TERM)});
    make_rule(ders, TokenType::ARGLIST, Rule::ARGLIST_AND_TERM_IS_ARGLIST, {sym_tok(TokenType::ARGLIST), char_tok(','), sym_tok(TokenType::TERM)});
    make_rule(ders, TokenType::TERM, Rule::FUNC_APP_IS_TERM, {sym_tok(TokenType::ID), char_tok('('), sym_tok(TokenType::ARGLIST), char_tok(')')});
    make_rule(ders, TokenType::ATOM, Rule::TERM_IS_ATOM, {sym_tok(TokenType::TERM)});
    make_rule(ders, TokenType::ATOM, Rule::TERM_EQ_IS_ATOM, {sym_tok(TokenType::TERM), char_tok('='), sym_tok(TokenType::TERM)});
    make_rule(ders, TokenType::ATOM, Rule::TERM_NEQ_IS_ATOM, {sym_tok(TokenType::TERM), char_tok('!'), char_tok('='), sym_tok(TokenType::TERM)});
    make_rule(ders, TokenType::LITERAL, Rule::ATOM_IS_LITERAL, {sym_tok(TokenType::ATOM)});
    make_rule(ders, TokenType::LITERAL, Rule::NEG_ATOM_IS_LITERAL, {char_tok('~'), sym_tok(TokenType::ATOM)});
    make_rule(ders, TokenType::CLAUSE, Rule::LITERAL_IS_CLAUSE, {sym_tok(TokenType::LITERAL)});
    make_rule(ders, TokenType::CLAUSE, Rule::CLAUSE_AND_LITERAL_IS_CLAUSE, {sym_tok(TokenType::CLAUSE), char_tok('|'), sym_tok(TokenType::LITERAL)});
    make_rule(ders, TokenType::CLAUSE, Rule::PARENS_CLAUSE_IS_CLAUSE, {char_tok('('), sym_tok(TokenType::CLAUSE), char_tok(')')});

    // Rules for FOFs
    make_rule(ders, TokenType::FOF, Rule::UNIT_FOF_IS_FOF, {sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::UNIT_FOF, Rule::PARENS_FOF_IS_UNIT_FOF, {char_tok('('), sym_tok(TokenType::FOF), char_tok(')')});
    make_rule(ders, TokenType::UNIT_FOF, Rule::TERM_IS_UNIT_FOF, {sym_tok(TokenType::TERM)});
    make_rule(ders, TokenType::UNIT_FOF, Rule::NOT_UNIT_FOF_IS_UNIT_FOF, {char_tok('~'), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::UNIT_FOF, Rule::TERM_EQ_IS_UNIT_FOF, {sym_tok(TokenType::TERM), char_tok('='), sym_tok(TokenType::TERM)});
    make_rule(ders, TokenType::UNIT_FOF, Rule::TERM_NEQ_IS_UNIF_FOF, {sym_tok(TokenType::TERM), char_tok('!'), char_tok('='), sym_tok(TokenType::TERM)});
    make_rule(ders, TokenType::VARLIST, Rule::ID_IS_VARLIST, {sym_tok(TokenType::ID)});
    make_rule(ders, TokenType::VARLIST, Rule::VARLIST_AND_ID_IS_VARLIST, {sym_tok(TokenType::VARLIST), char_tok(','), sym_tok(TokenType::ID)});
    make_rule(ders, TokenType::UNIT_FOF, Rule::FORALL_IS_UNIT_FOF, {char_tok('!'), char_tok('['), sym_tok(TokenType::VARLIST), char_tok(']'), char_tok(':'), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::UNIT_FOF, Rule::EXISTS_IS_UNIT_FOF, {char_tok('?'), char_tok('['), sym_tok(TokenType::VARLIST), char_tok(']'), char_tok(':'), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::FOF, Rule::BIIMP_IS_FOF, {sym_tok(TokenType::UNIT_FOF), char_tok('<'), char_tok('='), char_tok('>'), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::FOF, Rule::IMP_IS_FOF, {sym_tok(TokenType::UNIT_FOF), char_tok('='), char_tok('>'), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::FOF, Rule::REV_IMP_IS_FOF, {sym_tok(TokenType::UNIT_FOF), char_tok('<'), char_tok('='), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::FOF, Rule::XOR_IS_FOF, {sym_tok(TokenType::UNIT_FOF), char_tok('<'), char_tok('~'), char_tok('>'), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::FOF, Rule::NOR_IS_FOF, {sym_tok(TokenType::UNIT_FOF), char_tok('~'), char_tok('|'), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::FOF, Rule::NAND_IS_FOF, {sym_tok(TokenType::UNIT_FOF), char_tok('~'), char_tok('&'), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::FOF, Rule::AND_IS_FOF, {sym_tok(TokenType::FOF_AND)});
    make_rule(ders, TokenType::FOF_AND, Rule::AND_IS_AND, {sym_tok(TokenType::UNIT_FOF), char_tok('&'), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::FOF_AND, Rule::AND_AND_UNIT_FOF_IS_AND, {sym_tok(TokenType::FOF_AND), char_tok('&'), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::FOF, Rule::OR_IS_FOF, {sym_tok(TokenType::FOF_OR)});
    make_rule(ders, TokenType::FOF_OR, Rule::OR_IS_OR, {sym_tok(TokenType::UNIT_FOF), char_tok('|'), sym_tok(TokenType::UNIT_FOF)});
    make_rule(ders, TokenType::FOF_OR, Rule::OR_AND_UNIT_FOF_IS_OR, {sym_tok(TokenType::FOF_OR), char_tok('|'), sym_tok(TokenType::UNIT_FOF)});

    // Rules for generic expressions
    make_rule(ders, TokenType::EXPR, Rule::ID_IS_EXPR, {sym_tok(TokenType::ID)});
    make_rule(ders, TokenType::EXPR_ARGLIST, Rule::EXPR_IS_ARGLIST, {sym_tok(TokenType::EXPR)});
    make_rule(ders, TokenType::EXPR_ARGLIST, Rule::ARGLIST_AND_EXPR_IS_ARGLIST, {sym_tok(TokenType::EXPR_ARGLIST), char_tok(','), sym_tok(TokenType::EXPR)});
    make_rule(ders, TokenType::EXPR, Rule::FUNC_APP_IS_EXPR, {sym_tok(TokenType::ID), char_tok('('), sym_tok(TokenType::EXPR_ARGLIST), char_tok(')')});
    make_rule(ders, TokenType::EXPR, Rule::EMPTY_LIST_IS_EXPR, {char_tok('['), char_tok(']')});
    make_rule(ders, TokenType::EXPR, Rule::LIST_IS_EXPR, {char_tok('['), sym_tok(TokenType::EXPR_ARGLIST), char_tok(']')});
    make_rule(ders, TokenType::EXPR_DICTLIST, Rule::COUPLE_IS_DICTLIST, {sym_tok(TokenType::ID), char_tok(':'), sym_tok(TokenType::EXPR)});
    make_rule(ders, TokenType::EXPR_DICTLIST, Rule::DICTLIST_AND_COUPLE_IS_DICTLIST, {sym_tok(TokenType::EXPR_DICTLIST), char_tok(','), sym_tok(TokenType::ID), char_tok(':'), sym_tok(TokenType::EXPR)});
    make_rule(ders, TokenType::EXPR, Rule::DICT_IS_EXPR, {char_tok('['), sym_tok(TokenType::EXPR_DICTLIST), char_tok(']')});

    // Rules for whole lines
    make_rule(ders, TokenType::LINE, Rule::CNF_IS_LINE, {char_tok('c'), char_tok('n'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::CLAUSE), char_tok(')'), char_tok('.')});
    make_rule(ders, TokenType::LINE, Rule::CNF_WITH_DERIV_IS_LINE, {char_tok('c'), char_tok('n'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::CLAUSE), char_tok(','), sym_tok(TokenType::EXPR), char_tok(')'), char_tok('.')});
    make_rule(ders, TokenType::LINE, Rule::CNF_WITH_DERIV_AND_ANNOT_IS_LINE, {char_tok('c'), char_tok('n'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::CLAUSE), char_tok(','), sym_tok(TokenType::EXPR), char_tok(','), sym_tok(TokenType::EXPR), char_tok(')'), char_tok('.')});
    make_rule(ders, TokenType::LINE, Rule::FOF_IS_LINE, {char_tok('f'), char_tok('o'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::UNIT_FOF), char_tok(')'), char_tok('.')});
    make_rule(ders, TokenType::LINE, Rule::FOF_WITH_DERIV_IS_LINE, {char_tok('f'), char_tok('o'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::UNIT_FOF), char_tok(','), sym_tok(TokenType::EXPR), char_tok(')'), char_tok('.')});
    make_rule(ders, TokenType::LINE, Rule::FOF_WITH_DERIV_AND_ANNOT_IS_LINE, {char_tok('f'), char_tok('o'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::UNIT_FOF), char_tok(','), sym_tok(TokenType::EXPR), char_tok(','), sym_tok(TokenType::EXPR), char_tok(')'), char_tok('.')});

    return ders;
}

std::vector<Token> trivial_lexer(std::string s) {
    std::vector < Token > ret;
    std::set< char > id_letters_set;
    for (char c : ID_LETTERS) {
        id_letters_set.insert(c);
    }
    bool last_was_id_letter = false;
    bool found_space = false;
    for (char c : s) {
        bool this_is_id_letter = id_letters_set.find(c) != id_letters_set.end();
        if (last_was_id_letter && this_is_id_letter && found_space) {
            throw "Whitespace cannot separate identifiers";
        }
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            found_space = true;
            continue;
        } else {
            found_space = false;
        }
        ret.push_back({TokenType::CHAR, c});
        last_was_id_letter = this_is_id_letter;
    }
    return ret;
}

std::shared_ptr<Term> Term::reconstruct(const PT &pt)
{

}

std::shared_ptr<Var> Var::reconstruct(const PT &pt)
{

}

std::shared_ptr<FunctorApp> FunctorApp::reconstruct(const PT &pt)
{

}

std::shared_ptr<Atom> Atom::reconstruct(const PT &pt)
{

}

std::shared_ptr<Equality> Equality::reconstruct(const PT &pt)
{

}

std::shared_ptr<PredicateApp> PredicateApp::reconstruct(const PT &pt)
{

}

std::shared_ptr<Clause> Clause::reconstruct(const PT &pt)
{

}

int parse_tstp_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto ders = create_derivations();
    LRParser< Token, Rule > parser(ders);
    //EarleyParser< TSTPToken, TSTPRule > parser(ders);
    parser.initialize();

    std::string line;
    while (std::getline(std::cin, line)) {
        auto lexes = trivial_lexer(line);
        auto pt = parser.parse(lexes.begin(), lexes.end(), Token(TokenType::LINE));
        std::cout << "Parsed as " << pt.label << ", with " << pt.children.size() << " children" << std::endl;
        std::cout << "Id is " << reconstruct_id(pt.children[0]) << "\n";
    }

    return 0;
}
static_block {
    register_main_function("parse_tstp", parse_tstp_main);
}

}
}
}
