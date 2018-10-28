#pragma once

#include <memory>
#include <ostream>
#include <limits>
#include <functional>
#include <cstddef>
#include <type_traits>

#include "parsing/parser.h"
#include "utils/utils.h"

namespace gio {
namespace mmpp {
namespace tstp {

enum class TokenType {
    CHAR,
    WHITESPACE,
    LETTER,
    ID,
    TERM,
    ARGLIST,
    ATOM,
    LITERAL,
    CLAUSE,
    VARLIST,
    UNIT_FOF,
    FOF,
    FOF_AND,
    FOF_OR,
    EXPR_ARGLIST,
    EXPR_DICTLIST,
    EXPR,
    CNF_LINE,
    FOF_LINE,
    CNF_LINES,
    FOF_LINES,
};

struct Token {
    TokenType type;
    char content;

    Token() = default;
    Token(TokenType type, char content = 0) : type(type), content(content) {}

    bool operator==(const Token &other) const {
        return this->type == other.type && this->content == other.content;
    }

    bool operator!=(const Token &other) const {
        return !this->operator==(other);
    }

    bool operator<(const Token &other) const {
        return (this->type < other.type) || (this->type == other.type && this->content < other.content);
    }
};

}
}
}

namespace std {
template< >
struct hash< gio::mmpp::tstp::Token > {
    typedef gio::mmpp::tstp::Token argument_type;
    typedef ::std::size_t result_type;
    result_type operator()(const argument_type &x) const noexcept {
        result_type res = 0;
        boost::hash_combine(res, x.type);
        boost::hash_combine(res, x.content);
        return res;
    }
};
}

namespace gio {
namespace mmpp {
namespace tstp {

static_assert(::std::numeric_limits<unsigned char>::max() == 0xff, "I need byte-sized unsigned char");

enum class Rule {
    NONE = 0,
    CHAR_IS_LETTER = 0x100,
    CHAR_IS_WHITESPACE = 0x200,
    LINE = 0x300,
    LETTER_IS_ID,
    LETTER_AND_ID_IS_ID,

    ID_IS_TERM,
    TERM_IS_ARGLIST,
    TERM_AND_ARGLIST_IS_ARGLIST,
    FUNC_APP_IS_TERM,
    PRED_APP_IS_ATOM,
    ID_IS_ATOM,
    TERM_EQ_IS_ATOM,
    TERM_NEQ_IS_ATOM,
    ATOM_IS_LITERAL,
    NEG_ATOM_IS_LITERAL,
    LITERAL_IS_CLAUSE,
    CLAUSE_AND_LITERAL_IS_CLAUSE,
    PARENS_CLAUSE_IS_CLAUSE,

    UNIT_FOF_IS_FOF,
    PARENS_FOF_IS_UNIT_FOF,
    TERM_IS_UNIT_FOF,
    NOT_UNIT_FOF_IS_UNIT_FOF,
    TERM_EQ_IS_UNIT_FOF,
    TERM_NEQ_IS_UNIF_FOF,
    ID_IS_VARLIST,
    VARLIST_AND_ID_IS_VARLIST,
    FORALL_IS_UNIT_FOF,
    EXISTS_IS_UNIT_FOF,
    BIIMP_IS_FOF,
    IMP_IS_FOF,
    REV_IMP_IS_FOF,
    XOR_IS_FOF,
    NOR_IS_FOF,
    NAND_IS_FOF,
    AND_IS_FOF,
    AND_IS_AND,
    AND_AND_UNIT_FOF_IS_AND,
    OR_IS_FOF,
    OR_IS_OR,
    OR_AND_UNIT_FOF_IS_OR,

    ID_IS_EXPR,
    EXPR_IS_ARGLIST,
    ARGLIST_AND_EXPR_IS_ARGLIST,
    FUNC_APP_IS_EXPR,
    EMPTY_LIST_IS_EXPR,
    LIST_IS_EXPR,
    CNF_IS_EXPR,
    COUPLE_IS_DICTLIST,
    DICTLIST_AND_COUPLE_IS_DICTLIST,
    DICT_IS_EXPR,

    CNF_IS_CNF_LINE,
    CNF_WITH_DERIV_IS_CNF_LINE,
    CNF_WITH_DERIV_AND_ANNOT_IS_CNF_LINE,
    FOF_IS_FOF_LINE,
    FOF_WITH_DERIV_IS_FOF_LINE,
    FOF_WITH_DERIV_AND_ANNOT_IS_FOF_LINE,

    CNF_LINE_IS_CNF_LINES,
    CNF_LINE_AND_CNF_LINES_IS_CNF_LINES,
    FOF_LINE_IS_FOF_LINES,
    FOF_LINE_AND_FOF_LINES_IS_FOF_LINES,
};

typedef ParsingTree<Token, Rule> PT;

class Term : public enable_create<Term> {
public:
    static std::shared_ptr<Term> reconstruct(const PT &pt);

    void print_to(std::ostream &s) const;
    bool operator<(const Term &x) const;

protected:
    Term(const std::string &functor, const std::vector<std::shared_ptr<Term>> &args);

private:
    std::string functor;
    std::vector<std::shared_ptr<Term>> args;
};

class Atom : public enable_create<Atom> {
public:
    static std::pair<bool, std::shared_ptr<Atom>> reconstruct(const PT &pt);

    void print_to(std::ostream &s) const;
    bool operator<(const Atom &x) const;

protected:
    Atom(const std::string &predicate, const std::vector<std::shared_ptr<Term>> &args);

private:
    std::string predicate;
    std::vector<std::shared_ptr<Term>> args;
};

class Literal : public enable_create<Literal> {
public:
    static std::shared_ptr<Literal> reconstruct(const PT &pt);

    void print_to(std::ostream &s) const;
    bool operator<(const Literal &x) const;

protected:
    Literal(bool sign, const std::shared_ptr<Atom> &atom);

private:
    bool sign;
    std::shared_ptr<Atom> atom;
};

class Clause : public enable_create<Clause> {
public:
    static std::shared_ptr<Clause> reconstruct(const PT &pt);

    void print_to(std::ostream &s) const;

protected:
    explicit Clause(const std::set<std::shared_ptr<Literal>, star_less<std::shared_ptr<Literal>>> &literals);

private:
    std::set<std::shared_ptr<Literal>, star_less<std::shared_ptr<Literal>>> literals;
};

template<typename T, typename = decltype(std::declval<T>().print_to(std::declval<std::ostream&>()))>
std::ostream &operator<<(std::ostream &s, const T &t) {
    t.print_to(s);
    return s;
}

}
}
}
