#pragma once

#include <memory>
#include <ostream>
#include <limits>
#include <functional>
#include <cstddef>
#include <type_traits>

#include <giolib/containers.h>

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
    RESTR_ID,
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
    LETTER_IS_RESTR_ID,
    LETTER_AND_ID_IS_RESTR_ID,
    RESTR_ID_IS_ID,
    DOLLAR_AND_RESTR_ID_IS_ID,

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
    NEG_PARENS_ATOM_IS_LITERAL,
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
    EXPR_AND_ARGLIST_IS_ARGLIST,
    FUNC_APP_IS_EXPR,
    EMPTY_LIST_IS_EXPR,
    LIST_IS_EXPR,
    CNF_IS_EXPR,
    TERM_IS_EXPR,
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
    static std::shared_ptr<const Term> reconstruct(const PT &pt);

    void print_to(std::ostream &s) const;
    bool operator<(const Term &x) const;
    std::shared_ptr<const Term> substitute(const std::map<std::string, std::shared_ptr<const Term>> &subst) const;
    std::pair<std::shared_ptr<const Term>, std::shared_ptr<const Term>> replace(std::vector<size_t>::const_iterator path_begin, std::vector<size_t>::const_iterator path_end, const std::shared_ptr<const Term> &term) const;

protected:
    Term(const std::string &functor, const std::vector<std::shared_ptr<const Term>> &args);

private:
    std::string functor;
    std::vector<std::shared_ptr<const Term>> args;
};

class Atom : public enable_create<Atom> {
public:
    static std::pair<bool, std::shared_ptr<const Atom>> reconstruct(const PT &pt);

    void print_to(std::ostream &s) const;
    bool operator<(const Atom &x) const;
    std::shared_ptr<const Atom> substitute(const std::map<std::string, std::shared_ptr<const Term>> &subst) const;
    std::pair<std::shared_ptr<const Term>, std::shared_ptr<const Atom>> replace(std::vector<size_t>::const_iterator path_begin, std::vector<size_t>::const_iterator path_end, const std::shared_ptr<const Term> &term) const;

protected:
    Atom(const std::string &predicate, const std::vector<std::shared_ptr<const Term>> &args);

private:
    std::string predicate;
    std::vector<std::shared_ptr<const Term>> args;
};

class Literal : public enable_create<Literal> {
public:
    static std::shared_ptr<const Literal> reconstruct(const PT &pt);

    void print_to(std::ostream &s) const;
    bool operator<(const Literal &x) const;
    std::shared_ptr<const Literal> opposite() const;
    std::shared_ptr<const Literal> substitute(const std::map<std::string, std::shared_ptr<const Term>> &subst) const;
    std::pair<std::shared_ptr<const Term>, std::shared_ptr<const Literal>> replace(std::vector<size_t>::const_iterator path_begin, std::vector<size_t>::const_iterator path_end, const std::shared_ptr<const Term> &term) const;

protected:
    Literal(bool sign, const std::shared_ptr<const Atom> &atom);

private:
    bool sign;
    std::shared_ptr<const Atom> atom;
};

class Clause : public enable_create<Clause> {
public:
    static std::shared_ptr<const Clause> reconstruct(const PT &pt);

    void print_to(std::ostream &s) const;
    bool operator<(const Clause &x) const;
    std::shared_ptr<const Clause> substitute(const std::map<std::string, std::shared_ptr<const Term>> &subst) const;
    std::shared_ptr<const Clause> resolve(const Clause &other, const std::shared_ptr<const Literal> &lit) const;

protected:
    explicit Clause(const std::set<std::shared_ptr<const Literal>, gio::star_less<std::shared_ptr<const Literal>>> &literals);

private:
    std::set<std::shared_ptr<const Literal>, gio::star_less<std::shared_ptr<const Literal>>> literals;
};

// See https://stackoverflow.com/q/53022868/807307
template<typename T, typename = decltype(std::declval<T>().print_to(std::declval<std::ostream&>()))>
std::ostream &operator<<(std::ostream &s, const T &t) {
    t.print_to(s);
    return s;
}

class Inference {
public:
    virtual ~Inference();
    virtual std::shared_ptr<const Clause> compute_thesis(const std::vector<std::shared_ptr<const Clause>> &hyps) const = 0;
};

class Axiom : public Inference, public enable_create<Axiom> {
public:
    std::shared_ptr<const Clause> compute_thesis(const std::vector<std::shared_ptr<const Clause>> &hyps) const;

protected:
    Axiom(const std::shared_ptr<const Clause> &clause);

private:
    std::shared_ptr<const Clause> clause;
};

class Assume : public Inference, public enable_create<Assume> {
public:
    std::shared_ptr<const Clause> compute_thesis(const std::vector<std::shared_ptr<const Clause>> &hyps) const;

protected:
    Assume(const std::shared_ptr<const Literal> &literal);

private:
    std::shared_ptr<const Literal> literal;
};

class Subst : public Inference, public enable_create<Subst> {
public:
    std::shared_ptr<const Clause> compute_thesis(const std::vector<std::shared_ptr<const Clause>> &hyps) const;

    static std::pair<std::shared_ptr<const Subst>, std::vector<std::string>> reconstruct(const PT &pt);

protected:
    Subst(const std::map<std::string, std::shared_ptr<const Term>> &subst);

private:
    std::map<std::string, std::shared_ptr<const Term>> subst;
};

class Resolve : public Inference, public enable_create<Resolve> {
public:
    std::shared_ptr<const Clause> compute_thesis(const std::vector<std::shared_ptr<const Clause>> &hyps) const;

    static std::pair<std::shared_ptr<const Resolve>, std::vector<std::string>> reconstruct(const PT &pt);

protected:
    Resolve(const std::shared_ptr<const Literal> &literal);

private:
    std::shared_ptr<const Literal> literal;
};

class Refl : public Inference, public enable_create<Refl> {
public:
    std::shared_ptr<const Clause> compute_thesis(const std::vector<std::shared_ptr<const Clause>> &hyps) const;

    static std::pair<std::shared_ptr<const Refl>, std::vector<std::string>> reconstruct(const PT &pt);

protected:
    Refl(const std::shared_ptr<const Term> &term);

private:
    std::shared_ptr<const Term> term;
};

class Equality : public Inference, public enable_create<Equality> {
public:
    std::shared_ptr<const Clause> compute_thesis(const std::vector<std::shared_ptr<const Clause>> &hyps) const;

    static std::pair<std::shared_ptr<const Equality>, std::vector<std::string>> reconstruct(const PT &pt);

protected:
    Equality(const std::shared_ptr<const Literal> &literal, const std::vector<size_t> &path, const std::shared_ptr<const Term> &term);

private:
    std::shared_ptr<const Literal> literal;
    std::vector<size_t> path;
    std::shared_ptr<const Term> term;
};

}
}
}
