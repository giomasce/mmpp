
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

TSTPToken char_tok(char c) { return TSTPToken(TSTPTokenType::CHAR, c); }
TSTPToken sym_tok(TSTPTokenType type) { return TSTPToken(type); }

char letter_to_char(const ParsingTree<TSTPToken, TSTPRule> &pt) {
    assert(pt.type == sym_tok(TSTPTokenType::LETTER));
    int diff = static_cast<int>(pt.label) - static_cast<int>(TSTPRule::CHAR_IS_LETTER);
    assert(0 <= diff && diff < 0x100);
    return static_cast<char>(diff);
}

std::string reconstruct_id(const ParsingTree<TSTPToken, TSTPRule> &pt) {
    assert(pt.type == sym_tok(TSTPTokenType::ID));
    std::ostringstream ss;
    const auto *cur = &pt;
    while (true) {
        if (cur->label == TSTPRule::LETTER_IS_ID) {
            assert(cur->children.size() == 1);
            ss << letter_to_char(cur->children[0]);
            return ss.str();
        }
        assert(cur->label == TSTPRule::LETTER_AND_ID_IS_ID);
        assert(cur->children.size() == 2);
        ss << letter_to_char(cur->children[0]);
        cur = &cur->children[1];
    }
}

std::ostream &operator<<(std::ostream &stream, const TSTPTokenType &tt) {
    return stream << static_cast< int >(tt);
}

std::ostream &operator<<(std::ostream &stream, const TSTPRule &rule) {
    return stream << static_cast< int >(rule);
}

std::ostream &operator<<(std::ostream &stream, const TSTPToken &tok) {
    return stream << "[" << tok.type << "," << tok.content << "]";
}

void make_rule(std::unordered_map<TSTPToken, std::vector<std::pair<TSTPRule, std::vector<TSTPToken> > > > &ders, TSTPTokenType type, TSTPRule rule, std::vector< TSTPToken > tokens) {
    ders[type].push_back(std::make_pair(rule, tokens));
}

std::unordered_map<TSTPToken, std::vector<std::pair<TSTPRule, std::vector<TSTPToken> > > > create_derivations() {
    std::unordered_map<TSTPToken, std::vector<std::pair<TSTPRule, std::vector<TSTPToken> > > > ders;
    for (char c : ID_LETTERS) {
        auto uc = static_cast<unsigned char>(c);
        make_rule(ders, TSTPTokenType::LETTER, static_cast<TSTPRule>(static_cast<int>(TSTPRule::CHAR_IS_LETTER) + uc), {char_tok(c)});
    }
    for (char c : SPACE_LETTERS) {
        auto uc = static_cast<unsigned char>(c);
        make_rule(ders, TSTPTokenType::LETTER, static_cast<TSTPRule>(static_cast<int>(TSTPRule::CHAR_IS_WHITESPACE) + uc), {char_tok(c)});
    }
    make_rule(ders, TSTPTokenType::ID, TSTPRule::LETTER_IS_ID, {sym_tok(TSTPTokenType::LETTER)});
    make_rule(ders, TSTPTokenType::ID, TSTPRule::LETTER_AND_ID_IS_ID, {sym_tok(TSTPTokenType::LETTER), sym_tok(TSTPTokenType::ID)});

    // Rules for CNFs
    make_rule(ders, TSTPTokenType::TERM, TSTPRule::ID_IS_TERM, {sym_tok(TSTPTokenType::ID)});
    make_rule(ders, TSTPTokenType::ARGLIST, TSTPRule::TERM_IS_ARGLIST, {sym_tok(TSTPTokenType::TERM)});
    make_rule(ders, TSTPTokenType::ARGLIST, TSTPRule::ARGLIST_AND_TERM_IS_ARGLIST, {sym_tok(TSTPTokenType::ARGLIST), char_tok(','), sym_tok(TSTPTokenType::TERM)});
    make_rule(ders, TSTPTokenType::TERM, TSTPRule::FUNC_APP_IS_TERM, {sym_tok(TSTPTokenType::ID), char_tok('('), sym_tok(TSTPTokenType::ARGLIST), char_tok(')')});
    make_rule(ders, TSTPTokenType::ATOM, TSTPRule::TERM_IS_ATOM, {sym_tok(TSTPTokenType::TERM)});
    make_rule(ders, TSTPTokenType::ATOM, TSTPRule::TERM_EQ_IS_ATOM, {sym_tok(TSTPTokenType::TERM), char_tok('='), sym_tok(TSTPTokenType::TERM)});
    make_rule(ders, TSTPTokenType::ATOM, TSTPRule::TERM_NEQ_IS_ATOM, {sym_tok(TSTPTokenType::TERM), char_tok('!'), char_tok('='), sym_tok(TSTPTokenType::TERM)});
    make_rule(ders, TSTPTokenType::LITERAL, TSTPRule::ATOM_IS_LITERAL, {sym_tok(TSTPTokenType::ATOM)});
    make_rule(ders, TSTPTokenType::LITERAL, TSTPRule::NEG_ATOM_IS_LITERAL, {char_tok('~'), sym_tok(TSTPTokenType::ATOM)});
    make_rule(ders, TSTPTokenType::CLAUSE, TSTPRule::LITERAL_IS_CLAUSE, {sym_tok(TSTPTokenType::LITERAL)});
    make_rule(ders, TSTPTokenType::CLAUSE, TSTPRule::CLAUSE_AND_LITERAL_IS_CLAUSE, {sym_tok(TSTPTokenType::CLAUSE), char_tok('|'), sym_tok(TSTPTokenType::LITERAL)});
    make_rule(ders, TSTPTokenType::CLAUSE, TSTPRule::PARENS_CLAUSE_IS_CLAUSE, {char_tok('('), sym_tok(TSTPTokenType::CLAUSE), char_tok(')')});

    // Rules for FOFs
    make_rule(ders, TSTPTokenType::FOF, TSTPRule::UNIT_FOF_IS_FOF, {sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::UNIT_FOF, TSTPRule::PARENS_FOF_IS_UNIT_FOF, {char_tok('('), sym_tok(TSTPTokenType::FOF), char_tok(')')});
    make_rule(ders, TSTPTokenType::UNIT_FOF, TSTPRule::TERM_IS_UNIT_FOF, {sym_tok(TSTPTokenType::TERM)});
    make_rule(ders, TSTPTokenType::UNIT_FOF, TSTPRule::NOT_UNIT_FOF_IS_UNIT_FOF, {char_tok('~'), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::UNIT_FOF, TSTPRule::TERM_EQ_IS_UNIT_FOF, {sym_tok(TSTPTokenType::TERM), char_tok('='), sym_tok(TSTPTokenType::TERM)});
    make_rule(ders, TSTPTokenType::UNIT_FOF, TSTPRule::TERM_NEQ_IS_UNIF_FOF, {sym_tok(TSTPTokenType::TERM), char_tok('!'), char_tok('='), sym_tok(TSTPTokenType::TERM)});
    make_rule(ders, TSTPTokenType::VARLIST, TSTPRule::ID_IS_VARLIST, {sym_tok(TSTPTokenType::ID)});
    make_rule(ders, TSTPTokenType::VARLIST, TSTPRule::VARLIST_AND_ID_IS_VARLIST, {sym_tok(TSTPTokenType::VARLIST), char_tok(','), sym_tok(TSTPTokenType::ID)});
    make_rule(ders, TSTPTokenType::UNIT_FOF, TSTPRule::FORALL_IS_UNIT_FOF, {char_tok('!'), char_tok('['), sym_tok(TSTPTokenType::VARLIST), char_tok(']'), char_tok(':'), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::UNIT_FOF, TSTPRule::EXISTS_IS_UNIT_FOF, {char_tok('?'), char_tok('['), sym_tok(TSTPTokenType::VARLIST), char_tok(']'), char_tok(':'), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::FOF, TSTPRule::BIIMP_IS_FOF, {sym_tok(TSTPTokenType::UNIT_FOF), char_tok('<'), char_tok('='), char_tok('>'), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::FOF, TSTPRule::IMP_IS_FOF, {sym_tok(TSTPTokenType::UNIT_FOF), char_tok('='), char_tok('>'), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::FOF, TSTPRule::REV_IMP_IS_FOF, {sym_tok(TSTPTokenType::UNIT_FOF), char_tok('<'), char_tok('='), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::FOF, TSTPRule::XOR_IS_FOF, {sym_tok(TSTPTokenType::UNIT_FOF), char_tok('<'), char_tok('~'), char_tok('>'), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::FOF, TSTPRule::NOR_IS_FOF, {sym_tok(TSTPTokenType::UNIT_FOF), char_tok('~'), char_tok('|'), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::FOF, TSTPRule::NAND_IS_FOF, {sym_tok(TSTPTokenType::UNIT_FOF), char_tok('~'), char_tok('&'), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::FOF, TSTPRule::AND_IS_FOF, {sym_tok(TSTPTokenType::FOF_AND)});
    make_rule(ders, TSTPTokenType::FOF_AND, TSTPRule::AND_IS_AND, {sym_tok(TSTPTokenType::UNIT_FOF), char_tok('&'), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::FOF_AND, TSTPRule::AND_AND_UNIT_FOF_IS_AND, {sym_tok(TSTPTokenType::FOF_AND), char_tok('&'), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::FOF, TSTPRule::OR_IS_FOF, {sym_tok(TSTPTokenType::FOF_OR)});
    make_rule(ders, TSTPTokenType::FOF_OR, TSTPRule::OR_IS_OR, {sym_tok(TSTPTokenType::UNIT_FOF), char_tok('|'), sym_tok(TSTPTokenType::UNIT_FOF)});
    make_rule(ders, TSTPTokenType::FOF_OR, TSTPRule::OR_AND_UNIT_FOF_IS_OR, {sym_tok(TSTPTokenType::FOF_OR), char_tok('|'), sym_tok(TSTPTokenType::UNIT_FOF)});

    // Rules for generic expressions
    make_rule(ders, TSTPTokenType::EXPR, TSTPRule::ID_IS_EXPR, {sym_tok(TSTPTokenType::ID)});
    make_rule(ders, TSTPTokenType::EXPR_ARGLIST, TSTPRule::EXPR_IS_ARGLIST, {sym_tok(TSTPTokenType::EXPR)});
    make_rule(ders, TSTPTokenType::EXPR_ARGLIST, TSTPRule::ARGLIST_AND_EXPR_IS_ARGLIST, {sym_tok(TSTPTokenType::EXPR_ARGLIST), char_tok(','), sym_tok(TSTPTokenType::EXPR)});
    make_rule(ders, TSTPTokenType::EXPR, TSTPRule::FUNC_APP_IS_EXPR, {sym_tok(TSTPTokenType::ID), char_tok('('), sym_tok(TSTPTokenType::EXPR_ARGLIST), char_tok(')')});
    make_rule(ders, TSTPTokenType::EXPR, TSTPRule::EMPTY_LIST_IS_EXPR, {char_tok('['), char_tok(']')});
    make_rule(ders, TSTPTokenType::EXPR, TSTPRule::LIST_IS_EXPR, {char_tok('['), sym_tok(TSTPTokenType::EXPR_ARGLIST), char_tok(']')});
    make_rule(ders, TSTPTokenType::EXPR_DICTLIST, TSTPRule::COUPLE_IS_DICTLIST, {sym_tok(TSTPTokenType::ID), char_tok(':'), sym_tok(TSTPTokenType::EXPR)});
    make_rule(ders, TSTPTokenType::EXPR_DICTLIST, TSTPRule::DICTLIST_AND_COUPLE_IS_DICTLIST, {sym_tok(TSTPTokenType::EXPR_DICTLIST), char_tok(','), sym_tok(TSTPTokenType::ID), char_tok(':'), sym_tok(TSTPTokenType::EXPR)});
    make_rule(ders, TSTPTokenType::EXPR, TSTPRule::DICT_IS_EXPR, {char_tok('['), sym_tok(TSTPTokenType::EXPR_DICTLIST), char_tok(']')});

    // Rules for whole lines
    make_rule(ders, TSTPTokenType::LINE, TSTPRule::CNF_IS_LINE, {char_tok('c'), char_tok('n'), char_tok('f'), char_tok('('), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::CLAUSE), char_tok(')'), char_tok('.')});
    make_rule(ders, TSTPTokenType::LINE, TSTPRule::CNF_WITH_DERIV_IS_LINE, {char_tok('c'), char_tok('n'), char_tok('f'), char_tok('('), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::CLAUSE), char_tok(','), sym_tok(TSTPTokenType::EXPR), char_tok(')'), char_tok('.')});
    make_rule(ders, TSTPTokenType::LINE, TSTPRule::CNF_WITH_DERIV_AND_ANNOT_IS_LINE, {char_tok('c'), char_tok('n'), char_tok('f'), char_tok('('), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::CLAUSE), char_tok(','), sym_tok(TSTPTokenType::EXPR), char_tok(','), sym_tok(TSTPTokenType::EXPR), char_tok(')'), char_tok('.')});
    make_rule(ders, TSTPTokenType::LINE, TSTPRule::FOF_IS_LINE, {char_tok('f'), char_tok('o'), char_tok('f'), char_tok('('), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::UNIT_FOF), char_tok(')'), char_tok('.')});
    make_rule(ders, TSTPTokenType::LINE, TSTPRule::FOF_WITH_DERIV_IS_LINE, {char_tok('f'), char_tok('o'), char_tok('f'), char_tok('('), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::UNIT_FOF), char_tok(','), sym_tok(TSTPTokenType::EXPR), char_tok(')'), char_tok('.')});
    make_rule(ders, TSTPTokenType::LINE, TSTPRule::FOF_WITH_DERIV_AND_ANNOT_IS_LINE, {char_tok('f'), char_tok('o'), char_tok('f'), char_tok('('), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::ID), char_tok(','), sym_tok(TSTPTokenType::UNIT_FOF), char_tok(','), sym_tok(TSTPTokenType::EXPR), char_tok(','), sym_tok(TSTPTokenType::EXPR), char_tok(')'), char_tok('.')});

    return ders;
}

std::vector<TSTPToken> trivial_lexer(std::string s) {
    std::vector < TSTPToken > ret;
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
        ret.push_back({TSTPTokenType::CHAR, c});
        last_was_id_letter = this_is_id_letter;
    }
    return ret;
}

std::shared_ptr<Clause> Clause::reconstruct(const PT &pt)
{

}

int parse_tstp_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto ders = create_derivations();
    LRParser< TSTPToken, TSTPRule > parser(ders);
    //EarleyParser< TSTPToken, TSTPRule > parser(ders);
    parser.initialize();

    std::string line;
    while (std::getline(std::cin, line)) {
        auto lexes = trivial_lexer(line);
        auto pt = parser.parse(lexes.begin(), lexes.end(), TSTPToken(TSTPTokenType::LINE));
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
