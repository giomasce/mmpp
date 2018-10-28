
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
const std::string ID_LETTERS = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890_.@";
const std::string SPACE_LETTERS = " \t\n\r";

const std::string EQUAL_NAME = "$equal";
const std::string FALSE_NAME = "$false";

Token char_tok(char c) { return Token(TokenType::CHAR, c); }
Token sym_tok(TokenType type) { return Token(type); }

void check_pt(const PT& pt, const TokenType &type, const Rule &label, size_t children_num, const std::string &msg = "invalid arg") {
    assert_or_throw<std::invalid_argument>(pt.type == sym_tok(type), msg);
    assert_or_throw<std::invalid_argument>(pt.label == label, msg);
    assert_or_throw<std::invalid_argument>(pt.children.size() == children_num, msg);
}

char letter_to_char(const ParsingTree<Token, Rule> &pt) {
    gio_assert(pt.type == sym_tok(TokenType::LETTER));
    int diff = static_cast<int>(pt.label) - static_cast<int>(Rule::CHAR_IS_LETTER);
    gio_assert(0 <= diff && diff < 0x100);
    return static_cast<char>(diff);
}

std::string reconstruct_id(const ParsingTree<Token, Rule> &pt) {
    if (pt.type == sym_tok(TokenType::ID)) {
        if (pt.label == Rule::RESTR_ID_IS_ID) {
            gio_assert(pt.children.size() == 1);
            return reconstruct_id(pt.children[0]);
        } else {
            gio_assert(pt.label == Rule::DOLLAR_AND_RESTR_ID_IS_ID);
            gio_assert(pt.children.size() == 1);
            return std::string("$") + reconstruct_id(pt.children[0]);
        }
    }
    std::ostringstream ss;
    const auto *cur = &pt;
    while (true) {
        gio_assert(cur->type == sym_tok(TokenType::RESTR_ID));
        if (cur->label == Rule::LETTER_IS_RESTR_ID) {
            gio_assert(cur->children.size() == 1);
            ss << letter_to_char(cur->children[0]);
            return ss.str();
        }
        gio_assert(cur->label == Rule::LETTER_AND_ID_IS_RESTR_ID);
        gio_assert(cur->children.size() == 2);
        ss << letter_to_char(cur->children[0]);
        cur = &cur->children[1];
    }
}

char reconstrct_id_first_char(const ParsingTree<Token, Rule> &pt) {
    gio_assert(pt.type == sym_tok(TokenType::ID));
    if (pt.label == Rule::LETTER_IS_RESTR_ID) {
        gio_assert(pt.children.size() == 1);
        return letter_to_char(pt.children[0]);
    }
    gio_assert(pt.label == Rule::LETTER_AND_ID_IS_RESTR_ID);
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
    make_rule(ders, TokenType::RESTR_ID, Rule::LETTER_IS_RESTR_ID, {sym_tok(TokenType::LETTER)});
    make_rule(ders, TokenType::RESTR_ID, Rule::LETTER_AND_ID_IS_RESTR_ID, {sym_tok(TokenType::LETTER), sym_tok(TokenType::RESTR_ID)});
    make_rule(ders, TokenType::ID, Rule::RESTR_ID_IS_ID, {sym_tok(TokenType::RESTR_ID)});
    make_rule(ders, TokenType::ID, Rule::DOLLAR_AND_RESTR_ID_IS_ID, {char_tok('$'), sym_tok(TokenType::RESTR_ID)});

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
    make_rule(ders, TokenType::LITERAL, Rule::NEG_PARENS_ATOM_IS_LITERAL, {char_tok('~'), char_tok('('), sym_tok(TokenType::ATOM), char_tok(')')});
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
    make_rule(ders, TokenType::EXPR_ARGLIST, Rule::EXPR_AND_ARGLIST_IS_ARGLIST, {sym_tok(TokenType::EXPR), char_tok(','), sym_tok(TokenType::EXPR_ARGLIST)});
    make_rule(ders, TokenType::EXPR, Rule::FUNC_APP_IS_EXPR, {sym_tok(TokenType::ID), char_tok('('), sym_tok(TokenType::EXPR_ARGLIST), char_tok(')')});
    make_rule(ders, TokenType::EXPR, Rule::EMPTY_LIST_IS_EXPR, {char_tok('['), char_tok(']')});
    make_rule(ders, TokenType::EXPR, Rule::LIST_IS_EXPR, {char_tok('['), sym_tok(TokenType::EXPR_ARGLIST), char_tok(']')});
    make_rule(ders, TokenType::EXPR, Rule::CNF_IS_EXPR, {char_tok('$'), char_tok('c'), char_tok('n'), char_tok('f'), char_tok('('), sym_tok(TokenType::LITERAL), char_tok(')')});
    make_rule(ders, TokenType::EXPR, Rule::TERM_IS_EXPR, {char_tok('$'), char_tok('f'), char_tok('o'), char_tok('t'), char_tok('('), sym_tok(TokenType::TERM), char_tok(')')});
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

std::vector<std::shared_ptr<const Term>> reconstruct_arglist(const PT &pt) {
    std::vector<std::shared_ptr<const Term>> ret;
    const auto *cur = &pt;
    while (true) {
        gio_assert(cur->type == TokenType::ARGLIST);
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

std::shared_ptr<const Term> Term::reconstruct(const PT &pt)
{
    gio_assert(pt.type == TokenType::TERM);
    if (pt.label == Rule::ID_IS_TERM) {
        gio_assert(pt.children.size() == 1);
        return Term::create(reconstruct_id(pt.children[0]), std::vector<std::shared_ptr<const Term>>{});
    } else {
        gio_assert(pt.children.size() == 2);
        return Term::create(reconstruct_id(pt.children[0]), reconstruct_arglist(pt.children[1]));
    }
}

void Term::print_to(std::ostream &s) const
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

bool Term::operator<(const Term &x) const
{
    if (this->functor < x.functor) return true;
    if (x.functor < this->functor) return false;
    gio_assert(this->args.size() == x.args.size());
    return std::lexicographical_compare(this->args.begin(), this->args.end(), x.args.begin(), x.args.end(), star_less<std::shared_ptr<const Term>>());
}

std::shared_ptr<const Term> Term::substitute(const std::map<std::string, std::shared_ptr<const Term> > &subst) const {
    if (std::find(VAR_LETTERS.begin(), VAR_LETTERS.end(), this->functor[0]) != VAR_LETTERS.end()) {
        gio_assert(this->args.empty());
        auto it = subst.find(this->functor);
        if (it != subst.end()) {
            return it->second;
        } else {
            return this->shared_from_this();
        }
    } else {
        std::vector<std::shared_ptr<const Term>> new_args;
        for (const auto &arg : this->args) {
            new_args.push_back(arg->substitute(subst));
        }
        return Term::create(this->functor, new_args);
    }
}

std::pair<std::shared_ptr<const Term>, std::shared_ptr<const Term> > Term::replace(std::vector<size_t>::const_iterator path_begin, std::vector<size_t>::const_iterator path_end, const std::shared_ptr<const Term> &term) const {
    if (path_begin != path_end) {
        auto &idx = *path_begin;
        assert_or_throw<std::invalid_argument>(idx < this->args.size(), "invalid path");
        auto new_args = this->args;
        auto res = this->args[idx]->replace(path_begin+1, path_end, term);
        new_args[idx] = res.second;
        return std::make_pair(res.first, Term::create(this->functor, new_args));
    } else {
        return std::make_pair(this->shared_from_this(), term);
    }
}

Term::Term(const std::string &functor, const std::vector<std::shared_ptr<const Term> > &args) : functor(functor), args(args) {
    gio_assert(this->args.empty() || std::find(VAR_LETTERS.begin(), VAR_LETTERS.end(), this->functor[0]) == VAR_LETTERS.end());
}

std::pair<bool, std::shared_ptr<const Atom>> Atom::reconstruct(const PT &pt)
{
    gio_assert(pt.type == sym_tok(TokenType::ATOM));
    if (pt.label == Rule::PRED_APP_IS_ATOM) {
        gio_assert(pt.children.size() == 2);
        return std::make_pair(true, Atom::create(reconstruct_id(pt.children[0]), reconstruct_arglist(pt.children[1])));
    } else if (pt.label == Rule::ID_IS_ATOM) {
        gio_assert(pt.children.size() == 1);
        std::string name = reconstruct_id(pt.children[0]);
        return std::make_pair(true, Atom::create(std::move(name), std::vector<std::shared_ptr<const Term>>{}));
    } else {
        gio_assert(pt.label == Rule::TERM_EQ_IS_ATOM || pt.label == Rule::TERM_NEQ_IS_ATOM);
        gio_assert(pt.children.size() == 2);
        return std::make_pair(pt.label == Rule::TERM_EQ_IS_ATOM, Atom::create(EQUAL_NAME, std::vector<std::shared_ptr<const Term>>{Term::reconstruct(pt.children[0]), Term::reconstruct(pt.children[1])}));
    }
}

void Atom::print_to(std::ostream &s) const
{
    if (this->predicate == EQUAL_NAME) {
        gio_assert(this->args.size() == 2);
        s << *this->args[0] << "=" << *this->args[1];
    } else {
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
}

bool Atom::operator<(const Atom &x) const
{
    if (this->predicate < x.predicate) return true;
    if (x.predicate < this->predicate) return false;
    gio_assert(this->args.size() == x.args.size());
    return std::lexicographical_compare(this->args.begin(), this->args.end(), x.args.begin(), x.args.end(), star_less<std::shared_ptr<const Term>>());
}

std::shared_ptr<const Atom> Atom::substitute(const std::map<std::string, std::shared_ptr<const Term> > &subst) const {
    std::vector<std::shared_ptr<const Term>> new_args;
    for (const auto &arg : this->args) {
        new_args.push_back(arg->substitute(subst));
    }
    return Atom::create(this->predicate, new_args);
}

std::pair<std::shared_ptr<const Term>, std::shared_ptr<const Atom> > Atom::replace(std::vector<size_t>::const_iterator path_begin, std::vector<size_t>::const_iterator path_end, const std::shared_ptr<const Term> &term) const {
    assert_or_throw<std::invalid_argument>(path_begin != path_end, "empty path");
    auto &idx = *path_begin;
    assert_or_throw<std::invalid_argument>(idx < this->args.size(), "invalid path");
    auto new_args = this->args;
    auto res = this->args[idx]->replace(path_begin+1, path_end, term);
    new_args[idx] = res.second;
    return std::make_pair(res.first, Atom::create(this->predicate, new_args));
}

Atom::Atom(const std::string &predicate, const std::vector<std::shared_ptr<const Term> > &args) : predicate(predicate), args(args) {
    gio_assert(std::find(VAR_LETTERS.begin(), VAR_LETTERS.end(), this->predicate[0]) == VAR_LETTERS.end());
}

std::shared_ptr<const Literal> Literal::reconstruct(const PT &pt)
{
    gio_assert(pt.type == sym_tok(TokenType::LITERAL));
    gio_assert(pt.children.size() == 1);
    auto ret = Atom::reconstruct(pt.children[0]);
    if (pt.label == Rule::NEG_ATOM_IS_LITERAL || pt.label == Rule::NEG_PARENS_ATOM_IS_LITERAL) {
        ret.first = !ret.first;
    } else {
        gio_assert(pt.label == Rule::ATOM_IS_LITERAL);
    }
    return Literal::create(ret.first, ret.second);
}

void Literal::print_to(std::ostream &s) const
{
    if (!this->sign) {
        s << '~';
    }
    s << *this->atom;
}

bool Literal::operator<(const Literal &x) const
{
    if (this->sign < x.sign) return true;
    if (x.sign < this->sign) return false;
    return *this->atom < *x.atom;
}

std::shared_ptr<const Literal> Literal::opposite() const
{
    return Literal::create(!this->sign, this->atom);
}

std::shared_ptr<const Literal> Literal::substitute(const std::map<std::string, std::shared_ptr<const Term> > &subst) const {
    return Literal::create(this->sign, this->atom->substitute(subst));
}

std::pair<std::shared_ptr<const Term>, std::shared_ptr<const Literal> > Literal::replace(std::vector<size_t>::const_iterator path_begin, std::vector<size_t>::const_iterator path_end, const std::shared_ptr<const Term> &term) const {
    auto res = this->atom->replace(path_begin, path_end, term);
    return std::make_pair(res.first, Literal::create(this->sign, res.second));
}

Literal::Literal(bool sign, const std::shared_ptr<const Atom> &atom) : sign(sign), atom(atom) {}

std::shared_ptr<const Clause> Clause::reconstruct(const PT &pt)
{
    gio_assert(pt.type == sym_tok(TokenType::CLAUSE));
    if (pt.label == Rule::PARENS_CLAUSE_IS_CLAUSE) {
        gio_assert(pt.children.size() == 1);
        return Clause::reconstruct(pt.children[0]);
    }
    std::set<std::shared_ptr<const Literal>, star_less<std::shared_ptr<const Literal>>> literals;
    const auto *cur = &pt;
    while (true) {
        gio_assert(cur->type == sym_tok(TokenType::CLAUSE));
        if (cur->label == Rule::LITERAL_IS_CLAUSE) {
            gio_assert(cur->children.size() == 1);
            literals.insert(Literal::reconstruct(cur->children[0]));
            return Clause::create(literals);
        }
        gio_assert(cur->label == Rule::CLAUSE_AND_LITERAL_IS_CLAUSE);
        gio_assert(pt.children.size() == 2);
        literals.insert(Literal::reconstruct(cur->children[1]));
        cur = &cur->children[0];
    }
}

void Clause::print_to(std::ostream &s) const
{
    if (this->literals.empty()) {
        s << "$false";
    } else {
        bool first = true;
        for (const auto &lit : this->literals) {
            if (!first) {
                s << '|';
            }
            first = false;
            s << *lit;
        }
    }
}

bool Clause::operator<(const Clause &x) const
{
    return std::lexicographical_compare(this->literals.begin(), this->literals.end(), x.literals.begin(), x.literals.end(), star_less<std::shared_ptr<const Literal>>());
}

std::shared_ptr<const Clause> Clause::substitute(const std::map<std::string, std::shared_ptr<const Term> > &subst) const {
    std::set<std::shared_ptr<const Literal>, star_less<std::shared_ptr<const Literal>>> new_literals;
    for (const auto &lit : this->literals) {
        new_literals.insert(lit->substitute(subst));
    }
    return Clause::create(new_literals);
}

std::shared_ptr<const Clause> Clause::resolve(const Clause &other, const std::shared_ptr<const Literal> &lit) const
{
    std::set<std::shared_ptr<const Literal>, star_less<std::shared_ptr<const Literal>>> new_literals;
    bool found = false;

    // Process first clause
    for (const auto &lit2 : this->literals) {
        if (*lit2 < *lit || *lit < *lit2) {
            new_literals.insert(lit2);
        } else {
            found = true;
        }
    }
    assert_or_throw<std::invalid_argument>(found, "literal does not appear in first clause");

    // Process second clause
    const auto opp_lit = lit->opposite();
    for (const auto &lit2 : other.literals) {
        if (*lit2 < *opp_lit || *opp_lit < *lit2) {
            new_literals.insert(lit2);
        } else {
            found = true;
        }
    }
    assert_or_throw<std::invalid_argument>(found, "opposite literal does not appear in second clause");

    return Clause::create(new_literals);
}

Clause::Clause(const std::set<std::shared_ptr<const Literal>, star_less<std::shared_ptr<const Literal> > > &literals) : literals(literals) {
    const static auto false_lit = Literal::create(true, Atom::create("$false", std::vector<std::shared_ptr<const Term>>{}));
    this->literals.erase(false_lit);
}

Inference::~Inference() {}

std::shared_ptr<const Clause> Axiom::compute_thesis(const std::vector<std::shared_ptr<const Clause> > &hyps) const {
    gio_assert(hyps.empty());
    return this->clause;
}

Axiom::Axiom(const std::shared_ptr<const Clause> &clause) : clause(clause) {}

std::shared_ptr<const Clause> Assume::compute_thesis(const std::vector<std::shared_ptr<const Clause> > &hyps) const {
    gio_assert(hyps.empty());
    return Clause::create(std::set<std::shared_ptr<const Literal>, star_less<std::shared_ptr<const Literal>>>{literal, literal->opposite()});
}

Assume::Assume(const std::shared_ptr<const Literal> &literal) : literal(literal) {}

std::shared_ptr<const Clause> Subst::compute_thesis(const std::vector<std::shared_ptr<const Clause> > &hyps) const {
    gio_assert(hyps.size() == 1);
    return hyps[0]->substitute(this->subst);
}

std::pair<std::shared_ptr<const Subst>, std::vector<std::string> > Subst::reconstruct(const PT &pt)
{
    check_pt(pt, TokenType::EXPR_ARGLIST, Rule::EXPR_AND_ARGLIST_IS_ARGLIST, 2);
    check_pt(pt.children[0], TokenType::EXPR, Rule::EMPTY_LIST_IS_EXPR, 0);
    const auto &pt2 = pt.children[1];
    check_pt(pt2, TokenType::EXPR_ARGLIST, Rule::EXPR_IS_ARGLIST, 1);
    const auto &pt3 = pt2.children[0];
    check_pt(pt3, TokenType::EXPR, Rule::DICT_IS_EXPR, 1);
    const auto &pt4 = pt3.children[0];
    check_pt(pt4, TokenType::EXPR_DICTLIST, Rule::COUPLE_IS_DICTLIST, 2);
    const auto hyp_name = reconstruct_id(pt4.children[0]);
    const auto &pt5 = pt4.children[1];
    check_pt(pt5, TokenType::EXPR, Rule::LIST_IS_EXPR, 1);
    const auto *cur = &pt5.children[0];
    std::map<std::string, std::shared_ptr<const Term>> subst;
    auto process_entry = [&subst](const PT &pt) {
        check_pt(pt, TokenType::EXPR, Rule::FUNC_APP_IS_EXPR, 2);
        const auto bind_name = reconstruct_id(pt.children[0]);
        assert_or_throw<std::invalid_argument>(bind_name == "bind", "invalid subst inference");
        const auto &pt2 = pt.children[1];
        check_pt(pt2, TokenType::EXPR_ARGLIST, Rule::EXPR_AND_ARGLIST_IS_ARGLIST, 2);
        const auto &pt3 = pt2.children[0];
        check_pt(pt3, TokenType::EXPR, Rule::ID_IS_EXPR, 1);
        const auto var_name = reconstruct_id(pt3.children[0]);
        const auto &pt4 = pt2.children[1];
        check_pt(pt4, TokenType::EXPR_ARGLIST, Rule::EXPR_IS_ARGLIST, 1);
        const auto &pt5 = pt4.children[0];
        check_pt(pt5, TokenType::EXPR, Rule::TERM_IS_EXPR, 1);
        subst[var_name] = Term::reconstruct(pt5.children[0]);
    };
    while (true) {
        assert_or_throw<std::invalid_argument>(cur->type == sym_tok(TokenType::EXPR_ARGLIST), "invalid subst inference");
        if (cur->label == Rule::EXPR_IS_ARGLIST) {
            assert_or_throw<std::invalid_argument>(cur->children.size() == 1, "invalid subst inference");
            process_entry(cur->children[0]);
            return {Subst::create(subst), {hyp_name}};
        }
        assert_or_throw<std::invalid_argument>(cur->label == Rule::EXPR_AND_ARGLIST_IS_ARGLIST, "invalid subst inference");
        assert_or_throw<std::invalid_argument>(cur->children.size() == 2, "invalid subst inference");
        process_entry(cur->children[0]);
        cur = &cur->children[1];
    }
}

Subst::Subst(const std::map<std::string, std::shared_ptr<const Term> > &subst) : subst(subst) {}

std::shared_ptr<const Clause> Resolve::compute_thesis(const std::vector<std::shared_ptr<const Clause> > &hyps) const {
    gio_assert(hyps.size() == 2);
    return hyps[0]->resolve(*hyps[1], this->literal);
}

std::pair<std::shared_ptr<const Resolve>, std::vector<std::string> > Resolve::reconstruct(const PT &pt)
{
    check_pt(pt, TokenType::EXPR_ARGLIST, Rule::EXPR_AND_ARGLIST_IS_ARGLIST, 2);
    const auto &pt2 = pt.children[0];
    check_pt(pt2, TokenType::EXPR, Rule::LIST_IS_EXPR, 1);
    const auto &pt3 = pt2.children[0];
    check_pt(pt3, TokenType::EXPR_ARGLIST, Rule::EXPR_IS_ARGLIST, 1);
    const auto &pt4 = pt3.children[0];
    check_pt(pt4, TokenType::EXPR, Rule::CNF_IS_EXPR, 1);
    const auto lit = Literal::reconstruct(pt4.children[0]);
    const auto &pt5 = pt.children[1];
    check_pt(pt5, TokenType::EXPR_ARGLIST, Rule::EXPR_IS_ARGLIST, 1);
    const auto &pt6 = pt5.children[0];
    check_pt(pt6, TokenType::EXPR, Rule::LIST_IS_EXPR, 1);
    const auto &pt7 = pt6.children[0];
    check_pt(pt7, TokenType::EXPR_ARGLIST, Rule::EXPR_AND_ARGLIST_IS_ARGLIST, 2);
    const auto &pt8 = pt7.children[0];
    check_pt(pt8, TokenType::EXPR, Rule::ID_IS_EXPR, 1);
    const auto hyp1_name = reconstruct_id(pt8.children[0]);
    const auto &pt9 = pt7.children[1];
    check_pt(pt9, TokenType::EXPR_ARGLIST, Rule::EXPR_IS_ARGLIST, 1);
    const auto &pt10 = pt9.children[0];
    check_pt(pt10, TokenType::EXPR, Rule::ID_IS_EXPR, 1);
    const auto hyp2_name = reconstruct_id(pt10.children[0]);
    return {Resolve::create(lit), {hyp1_name, hyp2_name}};
}

Resolve::Resolve(const std::shared_ptr<const Literal> &literal) : literal(literal) {}

std::shared_ptr<const Clause> Refl::compute_thesis(const std::vector<std::shared_ptr<const Clause> > &hyps) const {
    gio_assert(hyps.empty());
    return Clause::create(std::set<std::shared_ptr<const Literal>, star_less<std::shared_ptr<const Literal>>>{Literal::create(true, Atom::create(EQUAL_NAME, std::vector<std::shared_ptr<const Term>>{this->term, this->term}))});
}

std::pair<std::shared_ptr<const Refl>, std::vector<std::string> > Refl::reconstruct(const PT &pt)
{
    check_pt(pt, TokenType::EXPR_ARGLIST, Rule::EXPR_IS_ARGLIST, 1);
    const auto &pt2 = pt.children[0];
    check_pt(pt2, TokenType::EXPR, Rule::LIST_IS_EXPR, 1);
    const auto &pt3 = pt2.children[0];
    check_pt(pt3, TokenType::EXPR_ARGLIST, Rule::EXPR_IS_ARGLIST, 1);
    const auto &pt4 = pt3.children[0];
    check_pt(pt4, TokenType::EXPR, Rule::TERM_IS_EXPR, 1);
    auto term = Term::reconstruct(pt4.children[0]);
    return {Refl::create(term), {}};
}

Refl::Refl(const std::shared_ptr<const Term> &term) : term(term) {}

std::shared_ptr<const Clause> Equality::compute_thesis(const std::vector<std::shared_ptr<const Clause> > &hyps) const {
    gio_assert(hyps.empty());
    auto res = this->literal->replace(this->path.begin(), this->path.end(), this->term);
    auto eq_lit = Literal::create(false, Atom::create(EQUAL_NAME, std::vector<std::shared_ptr<const Term>>{res.first, this->term}));
    return Clause::create(std::set<std::shared_ptr<const Literal>, star_less<std::shared_ptr<const Literal>>>{eq_lit, this->literal->opposite(), res.second});
}

uint64_t reconstruct_int(const PT &pt) {
    auto str = reconstruct_id(pt);
    size_t pos;
    uint64_t ret = stoull(str, &pos);
    assert_or_throw<std::invalid_argument>(pos == str.size(), "invalid characters in numeral");
    return ret;
}

std::vector<size_t> reconstruct_path(const PT &pt) {
    std::vector<size_t> ret;
    const auto *cur = &pt;
    auto process_entry = [&ret](const PT &pt) {
        check_pt(pt, TokenType::EXPR, Rule::ID_IS_EXPR, 1);
        ret.push_back(static_cast<size_t>(reconstruct_int(pt.children[0])));
    };
    while (true) {
        assert_or_throw<std::invalid_argument>(cur->type == sym_tok(TokenType::EXPR_ARGLIST), "invalid arg");
        if (cur->label == Rule::EXPR_IS_ARGLIST) {
            assert_or_throw<std::invalid_argument>(cur->children.size() == 1, "invalid arg");
            process_entry(cur->children[0]);
            return ret;
        }
        assert_or_throw<std::invalid_argument>(cur->label == Rule::EXPR_AND_ARGLIST_IS_ARGLIST, "invalid arg");
        assert_or_throw<std::invalid_argument>(cur->children.size() == 2, "invalid arg");
        process_entry(cur->children[0]);
        cur = &cur->children[1];
    }
}

std::pair<std::shared_ptr<const Equality>, std::vector<std::string> > Equality::reconstruct(const PT &pt)
{
    check_pt(pt, TokenType::EXPR_ARGLIST, Rule::EXPR_IS_ARGLIST, 1);
    const auto &pt2 = pt.children[0];
    check_pt(pt2, TokenType::EXPR, Rule::LIST_IS_EXPR, 1);
    const auto &pt3 = pt2.children[0];
    check_pt(pt3, TokenType::EXPR_ARGLIST, Rule::EXPR_AND_ARGLIST_IS_ARGLIST, 2);
    const auto &pt4 = pt3.children[0];
    check_pt(pt4, TokenType::EXPR, Rule::CNF_IS_EXPR, 1);
    auto literal = Literal::reconstruct(pt4.children[0]);
    const auto &pt5 = pt3.children[1];
    check_pt(pt5, TokenType::EXPR_ARGLIST, Rule::EXPR_AND_ARGLIST_IS_ARGLIST, 2);
    const auto &pt6 = pt5.children[0];
    check_pt(pt6, TokenType::EXPR, Rule::LIST_IS_EXPR, 1);
    const auto path = reconstruct_path(pt6.children[0]);
    const auto &pt7 = pt5.children[1];
    check_pt(pt7, TokenType::EXPR_ARGLIST, Rule::EXPR_IS_ARGLIST, 1);
    const auto &pt8 = pt7.children[0];
    check_pt(pt8, TokenType::EXPR, Rule::TERM_IS_EXPR, 1);
    const auto term = Term::reconstruct(pt8.children[0]);
    return {Equality::create(literal, path, term), {}};
}

Equality::Equality(const std::shared_ptr<const Literal> &literal, const std::vector<size_t> &path, const std::shared_ptr<const Term> &term) : literal(literal), path(path), term(term) {}

std::pair<std::shared_ptr<const Inference>, std::vector<std::string>> reconstruct_inference(const ParsingTree<Token, Rule> &pt) {
    check_pt(pt, TokenType::EXPR, Rule::FUNC_APP_IS_EXPR, 2);
    const auto inf_kind = reconstruct_id(pt.children[0]);
    const auto &pt2 = pt.children[1];
    check_pt(pt2, TokenType::EXPR_ARGLIST, Rule::EXPR_AND_ARGLIST_IS_ARGLIST, 2);
    const auto &pt3 = pt2.children[0];
    check_pt(pt3, TokenType::EXPR, Rule::ID_IS_EXPR, 1);
    const auto inf_name = reconstruct_id(pt3.children[0]);
    const auto &pt4 = pt2.children[1];
    if (inf_kind == "introduced") {
        if (inf_name == "tautology") {
            check_pt(pt4, TokenType::EXPR_ARGLIST, Rule::EXPR_IS_ARGLIST, 1);
            const auto &pt5 = pt4.children[0];
            check_pt(pt5, TokenType::EXPR, Rule::LIST_IS_EXPR, 1);
            const auto &pt6 = pt5.children[0];
            check_pt(pt6, TokenType::EXPR_ARGLIST, Rule::EXPR_AND_ARGLIST_IS_ARGLIST, 2);
            const auto &pt7 = pt6.children[0];
            check_pt(pt7, TokenType::EXPR, Rule::ID_IS_EXPR, 1);
            const auto taut_name = reconstruct_id(pt7.children[0]);
            const auto &pt8 = pt6.children[1];
            if (taut_name == "refl") {
                return Refl::reconstruct(pt8);
            } else if (taut_name == "equality") {
                return Equality::reconstruct(pt8);
            } else {
                std::invalid_argument("unknown tautology name");
            }
        } else {
            throw std::invalid_argument("unknown inference name");
        }
    } else if (inf_kind == "inference") {
        if (inf_name == "subst") {
            return Subst::reconstruct(pt4);
        } else if (inf_name == "resolve") {
            return Resolve::reconstruct(pt4);
        } else {
            throw std::invalid_argument("unknown inference name");
        }
    } else {
        throw std::invalid_argument("unknown inference kind");
    }

    return {};
}

struct RefutationLine {
    std::string name;
    std::shared_ptr<const Clause> clause;
    std::shared_ptr<const Inference> inference;
    std::vector<size_t> hyps;
};

struct Refutation {
    std::map<std::string, size_t> id_map;
    std::vector<RefutationLine> lines;

    const RefutationLine &reconstruct_clause(const ParsingTree<Token, Rule> &pt) {
        gio_assert(pt.type == sym_tok(TokenType::CNF_LINE));
        gio_assert(pt.children.size() >= 3);
        std::string name = reconstruct_id(pt.children[0]);
        std::shared_ptr<const Inference> inference;
        std::vector<size_t> hyps;
        if (pt.children.size() >= 4) {
            std::vector<std::string> hyps_str;
            std::tie(inference, hyps_str) = reconstruct_inference(pt.children[3]);
            for (const auto &hyp_str : hyps_str) {
                hyps.push_back(id_map.at(hyp_str));
            }
        }
        this->id_map[name] = this->lines.size();
        this->lines.push_back({std::move(name), Clause::reconstruct(pt.children[2]), std::move(inference), std::move(hyps)});
        return this->lines.back();
    }
};

Refutation reconstruct_clauses(const ParsingTree<Token, Rule> &pt) {
    Refutation ref;
    const auto *cur = &pt;
    while (true) {
        gio_assert(cur->type == sym_tok(TokenType::CNF_LINES));
        if (cur->label == Rule::CNF_LINE_IS_CNF_LINES) {
            gio_assert(cur->children.size() == 1);
            ref.reconstruct_clause(cur->children[0]);
            return ref;
        }
        gio_assert(cur->label == Rule::CNF_LINE_AND_CNF_LINES_IS_CNF_LINES);
        gio_assert(cur->children.size() == 2);
        ref.reconstruct_clause(cur->children[0]);
        cur = &cur->children[1];
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

struct SplitClauses {
    SplitClauses(std::istream &stream) : stream(stream) {}

    std::string operator()() {
        std::ostringstream ss;
        unsigned depth = 0;
        while (true) {
            char c;
            if (!this->stream) {
                return "";
            }
            this->stream >> c;
            ss << c;
            if (c == '(') {
                depth++;
            }
            if (c == ')') {
                depth--;
            }
            if (c == '.' && depth == 0) {
                return ss.str();
            }
        }
    }

    std::istream &stream;
};

int parse_tstp_file_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto parser = init_tstp_parser();

    //std::ifstream fin("/tmp/test");
    /*auto lexes = simple_lexer(istream_begin_end<char>(std::cin));
    auto pt = parser.parse(lexes.begin(), lexes.end(), Token(TokenType::CNF_LINES));
    std::cout << "Parsed as " << pt.label << ", with " << pt.children.size() << " children" << std::endl;
    auto ref = reconstruct_clauses(pt);
    std::cout << "Clauses:\n";
    for (const auto &line : ref.lines) {*/
    SplitClauses sc(std::cin);
    Refutation ref;
    std::string line_str;
    while ((line_str = sc()) != "") {
        auto lexes = simple_lexer(line_str);
        auto pt = parser.parse(lexes.begin(), lexes.end(), Token(TokenType::CNF_LINE));
        auto line = ref.reconstruct_clause(pt);
        std::cout << " * "  << line.name << ": " << *line.clause << "\n";
        if (!line.hyps.empty()) {
            std::cout << "   Proved from";
            for (const auto &hyp : line.hyps) {
                std::cout << " " << ref.lines[hyp].name;
            }
            std::cout << "\n";
        }
        if (line.inference) {
            std::vector<std::shared_ptr<const Clause>> hyps;
            for (const auto &hyp : line.hyps) {
                hyps.push_back(ref.lines[hyp].clause);
            }
            auto new_clause = line.inference->compute_thesis(hyps);
            std::cout << "   Inference gave " << *new_clause << "\n";
            gio_assert(!(*line.clause < *new_clause));
            gio_assert(!(*new_clause < *line.clause));
        }
        std::cout << "\n";
    }

    return 0;
}
static_block {
    register_main_function("parse_tstp_file", parse_tstp_file_main);
}

}
}
}
