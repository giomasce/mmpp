
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

const std::string VAR_LETTERS = "QWERTYUIOPASDFGHJKLZXCVBNM";
const std::string ID_LETTERS = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890_$.@";
const std::string SPACE_LETTERS = " \t\n\r";

Token char_tok(char c) { return Token(TokenType::CHAR, c); }
Token sym_tok(TokenType type) { return Token(type); }

char letter_to_char(const ParsingTree<Token, Rule> &pt) {
    gio_assert(pt.type == sym_tok(TokenType::LETTER));
    int diff = static_cast<int>(pt.label) - static_cast<int>(Rule::CHAR_IS_LETTER);
    gio_assert(0 <= diff && diff < 0x100);
    return static_cast<char>(diff);
}

std::string reconstruct_id(const ParsingTree<Token, Rule> &pt) {
    std::ostringstream ss;
    const auto *cur = &pt;
    while (true) {
        gio_assert(cur->type == sym_tok(TokenType::ID));
        if (cur->label == Rule::LETTER_IS_ID) {
            gio_assert(cur->children.size() == 1);
            ss << letter_to_char(cur->children[0]);
            return ss.str();
        }
        gio_assert(cur->label == Rule::LETTER_AND_ID_IS_ID);
        gio_assert(cur->children.size() == 2);
        ss << letter_to_char(cur->children[0]);
        cur = &cur->children[1];
    }
}

char reconstrct_id_first_char(const ParsingTree<Token, Rule> &pt) {
    gio_assert(pt.type == sym_tok(TokenType::ID));
    if (pt.label == Rule::LETTER_IS_ID) {
        gio_assert(pt.children.size() == 1);
        return letter_to_char(pt.children[0]);
    }
    gio_assert(pt.label == Rule::LETTER_AND_ID_IS_ID);
    gio_assert(pt.children.size() == 2);
    return letter_to_char(pt.children[0]);
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
    make_rule(ders, TokenType::ARGLIST, Rule::TERM_AND_ARGLIST_IS_ARGLIST, {sym_tok(TokenType::TERM), char_tok(','), sym_tok(TokenType::ARGLIST)});
    make_rule(ders, TokenType::TERM, Rule::FUNC_APP_IS_TERM, {sym_tok(TokenType::ID), char_tok('('), sym_tok(TokenType::ARGLIST), char_tok(')')});
    make_rule(ders, TokenType::ATOM, Rule::PRED_APP_IS_ATOM, {sym_tok(TokenType::ID), char_tok('('), sym_tok(TokenType::ARGLIST), char_tok(')')});
    make_rule(ders, TokenType::ATOM, Rule::ID_IS_ATOM, {sym_tok(TokenType::ID)});
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
    make_rule(ders, TokenType::CNF_LINE, Rule::CNF_IS_CNF_LINE, {char_tok('c'), char_tok('n'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::CLAUSE), char_tok(')'), char_tok('.')});
    make_rule(ders, TokenType::CNF_LINE, Rule::CNF_WITH_DERIV_IS_CNF_LINE, {char_tok('c'), char_tok('n'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::CLAUSE), char_tok(','), sym_tok(TokenType::EXPR), char_tok(')'), char_tok('.')});
    make_rule(ders, TokenType::CNF_LINE, Rule::CNF_WITH_DERIV_AND_ANNOT_IS_CNF_LINE, {char_tok('c'), char_tok('n'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::CLAUSE), char_tok(','), sym_tok(TokenType::EXPR), char_tok(','), sym_tok(TokenType::EXPR), char_tok(')'), char_tok('.')});
    make_rule(ders, TokenType::FOF_LINE, Rule::FOF_IS_FOF_LINE, {char_tok('f'), char_tok('o'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::UNIT_FOF), char_tok(')'), char_tok('.')});
    make_rule(ders, TokenType::FOF_LINE, Rule::FOF_WITH_DERIV_IS_FOF_LINE, {char_tok('f'), char_tok('o'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::UNIT_FOF), char_tok(','), sym_tok(TokenType::EXPR), char_tok(')'), char_tok('.')});
    make_rule(ders, TokenType::FOF_LINE, Rule::FOF_WITH_DERIV_AND_ANNOT_IS_FOF_LINE, {char_tok('f'), char_tok('o'), char_tok('f'), char_tok('('), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::ID), char_tok(','), sym_tok(TokenType::UNIT_FOF), char_tok(','), sym_tok(TokenType::EXPR), char_tok(','), sym_tok(TokenType::EXPR), char_tok(')'), char_tok('.')});

    // Rules for many lines
    make_rule(ders, TokenType::CNF_LINES, Rule::CNF_LINE_IS_CNF_LINES, {sym_tok(TokenType::CNF_LINE)});
    make_rule(ders, TokenType::CNF_LINES, Rule::CNF_LINE_AND_CNF_LINES_IS_CNF_LINES, {sym_tok(TokenType::CNF_LINE), sym_tok(TokenType::CNF_LINES)});
    make_rule(ders, TokenType::FOF_LINES, Rule::FOF_LINE_IS_FOF_LINES, {sym_tok(TokenType::FOF_LINE)});
    make_rule(ders, TokenType::FOF_LINES, Rule::FOF_LINE_AND_FOF_LINES_IS_FOF_LINES, {sym_tok(TokenType::FOF_LINE), sym_tok(TokenType::FOF_LINES)});

    return ders;
}

template<typename T>
std::vector<Token> simple_lexer(const T &s) {
    std::vector < Token > ret;
    std::set< char > id_letters_set;
    for (char c : ID_LETTERS) {
        id_letters_set.insert(c);
    }
    bool last_was_id_letter = false;
    bool found_space = false;
    for (auto it = s.begin(); it != s.end(); it++) {
        char c = *it;
        if (c == '\'') {
            do {
                it++;
            } while (*it != '\'');
            // FIXME: represent properly strings instead of replacing them with @ out of lazyness
            c = '@';
        }
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

std::vector<std::shared_ptr<Term>> reconstruct_arglist(const PT &pt) {
    std::vector<std::shared_ptr<Term>> ret;
    const auto *cur = &pt;
    while (true) {
        gio_assert(pt.type == TokenType::ARGLIST);
        if (cur->label == Rule::TERM_IS_ARGLIST) {
            gio_assert(cur->children.size() == 1);
            ret.push_back(Term::reconstruct(cur->children[0]));
            return ret;
        }
        gio_assert(cur->label == Rule::TERM_AND_ARGLIST_IS_ARGLIST);
        gio_assert(cur->children.size() == 2);
        ret.push_back(Term::reconstruct(cur->children[0]));
        cur = &cur->children[1];
    }
}

std::shared_ptr<Term> Term::reconstruct(const PT &pt)
{
    gio_assert(pt.type == TokenType::TERM);
    if (pt.label == Rule::ID_IS_TERM) {
        if (std::find(VAR_LETTERS.begin(), VAR_LETTERS.end(), reconstrct_id_first_char(pt.children[0])) != VAR_LETTERS.end()) {
            return Var::reconstruct(pt);
        } else {
            return FunctorApp::reconstruct(pt);
        }
    } else {
        gio_assert(pt.label == Rule::FUNC_APP_IS_TERM);
        return FunctorApp::reconstruct(pt);
    }
}

Term::~Term()
{
}

std::shared_ptr<Var> Var::reconstruct(const PT &pt)
{
    gio_assert(pt.type == TokenType::TERM);
    gio_assert(pt.label == Rule::ID_IS_TERM);
    gio_assert(pt.children.size() == 1);
    std::string name = reconstruct_id(pt.children[0]);
    gio_assert(std::find(VAR_LETTERS.begin(), VAR_LETTERS.end(), name[0]) != VAR_LETTERS.end());
    return Var::create(std::move(name));
}

void Var::print_to(std::ostream &s) const
{
    s << this->name;
}

std::shared_ptr<FunctorApp> FunctorApp::reconstruct(const PT &pt)
{
    gio_assert(pt.type == TokenType::TERM);
    if (pt.label == Rule::FUNC_APP_IS_TERM) {
        gio_assert(pt.children.size() == 2);
        return FunctorApp::create(reconstruct_id(pt.children[0]), reconstruct_arglist(pt.children[1]));
    } else {
        gio_assert(pt.label == Rule::ID_IS_TERM);
        gio_assert(pt.children.size() == 1);
        std::string name = reconstruct_id(pt.children[0]);
        gio_assert(std::find(VAR_LETTERS.begin(), VAR_LETTERS.end(), name[0]) == VAR_LETTERS.end());
        return FunctorApp::create(std::move(name), std::vector<std::shared_ptr<Term>>{});
    }
}

void FunctorApp::print_to(std::ostream &s) const
{
    s << this->functor;
    if (!this->args.empty()) {
        s << '(';
        bool first = true;
        for (const auto &arg : this->args) {
            if (!first) {
                s << ',';
            }
            first = false;
            s << *arg;
        }
        s << ')';
    }
}

Atom::~Atom()
{
}

std::pair<bool, std::shared_ptr<Atom>> Atom::reconstruct(const PT &pt)
{
    if (pt.type == sym_tok(TokenType::LITERAL)) {
        gio_assert(pt.children.size() == 1);
        auto ret = Atom::reconstruct(pt.children[0]);
        if (pt.label == Rule::NEG_ATOM_IS_LITERAL) {
            ret.first = !ret.first;
        } else {
            gio_assert(pt.label == Rule::ATOM_IS_LITERAL);
        }
        return ret;
    }
    gio_assert(pt.type == sym_tok(TokenType::ATOM));
    if (pt.label == Rule::PRED_APP_IS_ATOM || pt.label == Rule::ID_IS_ATOM) {
        return std::make_pair(true, PredicateApp::reconstruct(pt));
    } else {
        return Equality::reconstruct(pt);
    }
}

std::pair<bool, std::shared_ptr<Equality>> Equality::reconstruct(const PT &pt)
{
    gio_assert(pt.type == sym_tok(TokenType::ATOM));
    gio_assert(pt.label == Rule::TERM_EQ_IS_ATOM || pt.label == Rule::TERM_NEQ_IS_ATOM);
    gio_assert(pt.children.size() == 2);
    return std::make_pair(pt.label == Rule::TERM_EQ_IS_ATOM, Equality::create(Term::reconstruct(pt.children[0]), Term::reconstruct(pt.children[1])));
}

void Equality::print_to(std::ostream &s) const
{
    s << *this->first << '=' << *this->second;
}

std::shared_ptr<PredicateApp> PredicateApp::reconstruct(const PT &pt)
{
    gio_assert(pt.type == sym_tok(TokenType::ATOM));
    if (pt.label == Rule::PRED_APP_IS_ATOM) {
        gio_assert(pt.children.size() == 2);
        return PredicateApp::create(reconstruct_id(pt.children[0]), reconstruct_arglist(pt.children[1]));
    } else {
        gio_assert(pt.label == Rule::ID_IS_ATOM);
        gio_assert(pt.children.size() == 1);
        std::string name = reconstruct_id(pt.children[0]);
        gio_assert(std::find(VAR_LETTERS.begin(), VAR_LETTERS.end(), name[0]) == VAR_LETTERS.end());
        return PredicateApp::create(std::move(name), std::vector<std::shared_ptr<Term>>{});
    }
}

void PredicateApp::print_to(std::ostream &s) const
{
    s << this->predicate;
    if (!this->args.empty()) {
        s << '(';
        bool first = true;
        for (const auto &arg : this->args) {
            if (!first) {
                s << ',';
            }
            first = false;
            s << *arg;
        }
        s << ')';
    }
}

std::shared_ptr<Clause> Clause::reconstruct(const PT &pt)
{
    gio_assert(pt.type == sym_tok(TokenType::CLAUSE));
    if (pt.label == Rule::PARENS_CLAUSE_IS_CLAUSE) {
        gio_assert(pt.children.size() == 1);
        return Clause::reconstruct(pt.children[0]);
    }
    if (pt.label == Rule::LITERAL_IS_CLAUSE) {
        gio_assert(pt.children.size() == 1);
        return Clause::create(std::vector<std::pair<bool, std::shared_ptr<Atom>>>{Atom::reconstruct(pt.children[0])});
    }
    gio_assert(pt.label == Rule::CLAUSE_AND_LITERAL_IS_CLAUSE);
    gio_assert(pt.children.size() == 2);
    auto ret = Clause::reconstruct(pt.children[0]);
    ret->literals.push_back(Atom::reconstruct(pt.children[1]));
    return ret;
}

std::vector<std::shared_ptr<Clause>> reconstruct_clauses(const ParsingTree<Token, Rule> &pt) {
    auto line_to_clause = [](const ParsingTree<Token, Rule> &pt) {
        gio_assert(pt.type == sym_tok(TokenType::CNF_LINE));
        gio_assert(pt.children.size() >= 3);
        return Clause::reconstruct(pt.children[2]);
    };
    std::vector<std::shared_ptr<Clause>> ret;
    const auto *cur = &pt;
    while (true) {
        gio_assert(cur->type == sym_tok(TokenType::CNF_LINES));
        if (cur->label == Rule::CNF_LINE_IS_CNF_LINES) {
            gio_assert(cur->children.size() == 1);
            ret.push_back(line_to_clause(cur->children[0]));
            return ret;
        }
        gio_assert(cur->label == Rule::CNF_LINE_AND_CNF_LINES_IS_CNF_LINES);
        gio_assert(cur->children.size() == 2);
        ret.push_back(line_to_clause(cur->children[0]));
        cur = &cur->children[1];
    }
}

void Clause::print_to(std::ostream &s) const
{
    bool first = true;
    for (const auto &lit : this->literals) {
        if (!first) {
            s << '|';
        }
        first = false;
        if (!lit.first) {
            s << '~';
        }
        s << *lit.second;
    }
}

decltype(auto) init_tstp_parser() {
    auto ders = create_derivations();
    LRParser<Token, Rule> parser(ders);
    parser.initialize();
    return parser;
}

int parse_tstp_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto parser = init_tstp_parser();

    std::string line;
    while (std::getline(std::cin, line)) {
        auto lexes = simple_lexer(line);
        auto pt = parser.parse(lexes.begin(), lexes.end(), Token(TokenType::CNF_LINE));
        std::cout << "Parsed as " << pt.label << ", with " << pt.children.size() << " children" << std::endl;
        std::cout << "Id is " << reconstruct_id(pt.children[0]) << "\n";
        auto clause = Clause::reconstruct(pt.children[2]);
        std::cout << "Recostructed as " << *clause << "\n";
    }

    return 0;
}
static_block {
    register_main_function("parse_tstp", parse_tstp_main);
}

template<typename T>
struct istream_begin_end {
    istream_begin_end(std::istream &s) : s(s) {}

    decltype(auto) begin() const {
        return std::istream_iterator<T>(s);
    }

    decltype(auto) end() const {
        return std::istream_iterator<T>();
    }

    std::istream &s;
};

int parse_tstp_file_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto parser = init_tstp_parser();

    auto lexes = simple_lexer(istream_begin_end<char>(std::cin));
    auto pt = parser.parse(lexes.begin(), lexes.end(), Token(TokenType::CNF_LINES));
    std::cout << "Parsed as " << pt.label << ", with " << pt.children.size() << " children" << std::endl;
    auto clauses = reconstruct_clauses(pt);
    std::cout << "Clauses:\n";
    for (const auto &clause : clauses) {
        std::cout << " * " << *clause << "\n";
    }

    return 0;
}
static_block {
    register_main_function("parse_tstp_file", parse_tstp_file_main);
}

}
}
}
