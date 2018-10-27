#pragma once

#include <memory>

#include "parsing/parser.h"
#include "utils/utils.h"

namespace gio {
namespace mmpp {
namespace tstp {

class Term {
};

class Var : public Term, public enable_create<Var> {
protected:
    explicit Var(const std::string &name) : name(name) {}

private:
    std::string name;
};

class FunctorApp : public Term, public enable_create<FunctorApp> {
protected:
    FunctorApp(const std::string &functor, const std::vector<std::shared_ptr<Term>> &args) : functor(functor), args(args) {}

private:
    std::string functor;
    std::vector<std::shared_ptr<Term>> args;
};

class Atom {
};

class Equality : public Atom, public enable_create<Equality> {
protected:
    Equality(const std::shared_ptr<Term> &first, const std::shared_ptr<Term> &second) : first(first), second(second) {}

private:
    std::shared_ptr<Term> first;
    std::shared_ptr<Term> second;
};

class PredicateApp : public Atom, public enable_create<PredicateApp> {
protected:
    PredicateApp(const std::string &predicate, const std::vector<std::shared_ptr<Term>> &args) : predicate(predicate), args(args) {}

private:
    std::string predicate;
    std::vector<std::shared_ptr<Term>> args;
};

class Clause : public enable_create<Clause> {
public:
    //static std::shared_ptr<Clause> parse_from(const ParsingTree)

protected:
    explicit Clause(const std::vector<std::pair<bool, std::shared_ptr<Atom>>> &literals) : literals(literals) {}

private:
    std::vector<std::pair<bool, std::shared_ptr<Atom>>> literals;
};

}
}
}
