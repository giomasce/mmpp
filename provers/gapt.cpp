
#include "gapt.h"

#include <giolib/main.h>
#include <giolib/static_block.h>
#include <giolib/utils.h>

#include "mm/setmm.h"
#include "fof.h"

typedef std::shared_ptr<const gio::mmpp::provers::fof::FOF> formula;
typedef std::pair<std::vector<formula>, std::vector<formula>> sequent;

formula parse_gapt_formula(std::istream &is) {
    using namespace gio::mmpp::provers::fof;
    std::string type;
    is >> type;
    if (type == "exists") {
        auto var = std::dynamic_pointer_cast<const Variable>(parse_gapt_formula(is));
        gio::assert_or_throw<std::runtime_error>(bool(var), "missing variable after quantifier");
        auto body = parse_gapt_formula(is);
        return Exists::create(var, body);
    } else if (type == "forall") {
        auto var = std::dynamic_pointer_cast<const Variable>(parse_gapt_formula(is));
        gio::assert_or_throw<std::runtime_error>(bool(var), "missing variable after quantifier");
        auto body = parse_gapt_formula(is);
        return Forall::create(var, body);
    } else if (type == "imp") {
        auto left = parse_gapt_formula(is);
        auto right = parse_gapt_formula(is);
        return Implies::create(left, right);
    } else if (type == "and") {
        auto left = parse_gapt_formula(is);
        auto right = parse_gapt_formula(is);
        return And::create(left, right);
    } else if (type == "or") {
        auto left = parse_gapt_formula(is);
        auto right = parse_gapt_formula(is);
        return Or::create(left, right);
    } else if (type == "not") {
        auto arg = parse_gapt_formula(is);
        return Not::create(arg);
    } else if (type == "false") {
        return False::create();
    } else if (type == "true") {
        return True::create();
    } else if (type == "unint") {
        std::string name;
        size_t num;
        is >> name >> num;
        std::vector<std::shared_ptr<const FOF>> args;
        for (size_t i = 0; i < num; i++) {
            args.push_back(parse_gapt_formula(is));
        }
        return Uninterpreted::create(name, args);
    } else if (type == "var") {
        std::string name;
        is >> name;
        return Variable::create(name);
    } else {
        throw std::runtime_error("invalid formula type " + type);
    }
}

sequent parse_gapt_sequent(std::istream &is) {
    size_t num;
    std::vector<formula> ants;
    std::vector<formula> succs;
    is >> num;
    for (size_t i = 0; i < num; i++) {
        ants.push_back(parse_gapt_formula(is));
    }
    is >> num;
    for (size_t i = 0; i < num; i++) {
        succs.push_back(parse_gapt_formula(is));
    }
    return std::make_pair(ants, succs);
}

class NDProof {
public:
    virtual ~NDProof() = default;
    virtual void print_to(std::ostream &s) const {
        s << "NDProof";
    }

protected:
    NDProof(const sequent &thesis) : thesis(thesis) {}

private:
    sequent thesis;
};

typedef std::shared_ptr<const NDProof> proof;

class LogicalAxiom : public NDProof, public gio::virtual_enable_create<LogicalAxiom> {
protected:
    LogicalAxiom(const sequent &thesis, const formula &form)
        : NDProof(thesis), form(form) {}

private:
    formula form;
};

class ExistsIntroRule : public NDProof, public gio::virtual_enable_create<ExistsIntroRule> {
protected:
    ExistsIntroRule(const sequent &thesis, const formula &form, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &var, const formula &term, const proof &subproof)
        : NDProof(thesis), form(form), var(var), term(term), subproof(subproof) {}

private:
    formula form;
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> var;
    formula term;
    std::shared_ptr<const NDProof> subproof;
};

class ImpIntroRule : public NDProof, public gio::virtual_enable_create<ImpIntroRule> {
protected:
    ImpIntroRule(const sequent &thesis, ssize_t ant_idx, const proof &subproof)
        : NDProof(thesis), ant_idx(ant_idx), subproof(subproof) {}

private:
    ssize_t ant_idx;
    proof subproof;
};

class WeakeningRule : public NDProof, public gio::virtual_enable_create<WeakeningRule> {
protected:
    WeakeningRule(const sequent &thesis, const formula &form, const proof &subproof)
        : NDProof(thesis), form(form), subproof(subproof) {}

private:
    formula form;
    proof subproof;
};

class BottomElimRule : public NDProof, public gio::virtual_enable_create<BottomElimRule> {
protected:
    BottomElimRule(const sequent &thesis, const formula &form, const proof &subproof)
        : NDProof(thesis), form(form), subproof(subproof) {}

private:
    formula form;
    proof subproof;
};

class AndElim1Rule : public NDProof, public gio::virtual_enable_create<AndElim1Rule> {
protected:
    AndElim1Rule(const sequent &thesis, const proof &subproof)
        : NDProof(thesis), subproof(subproof) {}

private:
    proof subproof;
};

class AndElim2Rule : public NDProof, public gio::virtual_enable_create<AndElim2Rule> {
protected:
    AndElim2Rule(const sequent &thesis, const proof &subproof)
        : NDProof(thesis), subproof(subproof) {}

private:
    proof subproof;
};

class ForallIntroRule : public NDProof, public gio::virtual_enable_create<ForallIntroRule> {
protected:
    ForallIntroRule(const sequent &thesis, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &var, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &eigenvar, const proof &subproof)
        : NDProof(thesis), var(var), eigenvar(eigenvar), subproof(subproof) {}

private:
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> var, eigenvar;
    proof subproof;
};

class NegElimRule : public NDProof, public gio::virtual_enable_create<NegElimRule> {
protected:
    NegElimRule(const sequent &thesis, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

private:
    proof left_proof, right_proof;
};

class ImpElimRule : public NDProof, public gio::virtual_enable_create<ImpElimRule> {
protected:
    ImpElimRule(const sequent &thesis, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

private:
    proof left_proof, right_proof;
};

class AndIntroRule : public NDProof, public gio::virtual_enable_create<AndIntroRule> {
protected:
    AndIntroRule(const sequent &thesis, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

private:
    proof left_proof, right_proof;
};

class ExistsElimRule : public NDProof, public gio::virtual_enable_create<ExistsElimRule> {
protected:
    ExistsElimRule(const sequent &thesis, ssize_t idx, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &eigenvar, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), idx(idx), eigenvar(eigenvar), left_proof(left_proof), right_proof(right_proof) {}

private:
    ssize_t idx;
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> eigenvar;
    proof left_proof, right_proof;
};

class ContractionRule : public NDProof, public gio::virtual_enable_create<ContractionRule> {
protected:
    ContractionRule(const sequent &thesis, ssize_t idx1, ssize_t idx2, const proof &subproof)
        : NDProof(thesis), idx1(idx1), idx2(idx2), subproof(subproof) {}

private:
    ssize_t idx1, idx2;
    proof subproof;
};

class ExcludedMiddleRule : public NDProof, public gio::virtual_enable_create<ExcludedMiddleRule> {
protected:
    ExcludedMiddleRule(const sequent &thesis, ssize_t left_idx, ssize_t right_idx, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_idx(left_idx), right_idx(right_idx), left_proof(left_proof), right_proof(right_proof) {}

private:
    ssize_t left_idx, right_idx;
    proof left_proof, right_proof;
};

std::shared_ptr<const NDProof> parse_gapt_proof(std::istream &is) {
    auto thesis = parse_gapt_sequent(is);
    std::string type;
    is >> type;
    if (type == "ExcludedMiddle") {
        ssize_t left_idx, right_idx;
        is >> left_idx >> right_idx;
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return ExcludedMiddleRule::create(thesis, left_idx, right_idx, left_proof, right_proof);
    } else if (type == "Contraction") {
        ssize_t idx1, idx2;
        is >> idx1 >> idx2;
        auto subproof = parse_gapt_proof(is);
        return ContractionRule::create(thesis, idx1, idx2, subproof);
    } else if (type == "AndElim1") {
        auto subproof = parse_gapt_proof(is);
        return AndElim1Rule::create(thesis, subproof);
    } else if (type == "AndElim2") {
        auto subproof = parse_gapt_proof(is);
        return AndElim2Rule::create(thesis, subproof);
    } else if (type == "NegElim") {
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return NegElimRule::create(thesis, left_proof, right_proof);
    } else if (type == "AndIntro") {
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return AndIntroRule::create(thesis, left_proof, right_proof);
    } else if (type == "ExistsElim") {
        ssize_t idx;
        is >> idx;
        auto eigenvar = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_formula(is));
        gio::assert_or_throw<std::invalid_argument>(bool(eigenvar), "missing eigenvariable");
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return ExistsElimRule::create(thesis, idx, eigenvar, left_proof, right_proof);
    } else if (type == "ImpElim") {
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return ImpElimRule::create(thesis, left_proof, right_proof);
    } else if (type == "LogicalAxiom") {
        auto form = parse_gapt_formula(is);
        return LogicalAxiom::create(thesis, form);
    } else if (type == "ExistsIntro") {
        auto form = parse_gapt_formula(is);
        auto var = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_formula(is));
        gio::assert_or_throw<std::invalid_argument>(bool(var), "missing substitution variable");
        auto term = parse_gapt_formula(is);
        auto subproof = parse_gapt_proof(is);
        return ExistsIntroRule::create(thesis, form, var, term, subproof);
    } else if (type == "ImpIntro") {
        ssize_t ant_idx;
        is >> ant_idx;
        auto subproof = parse_gapt_proof(is);
        return ImpIntroRule::create(thesis, ant_idx, subproof);
    } else if (type == "Weakening") {
        auto form = parse_gapt_formula(is);
        auto subproof = parse_gapt_proof(is);
        return WeakeningRule::create(thesis, form, subproof);
    } else if (type == "BottomElim") {
        auto form = parse_gapt_formula(is);
        auto subproof = parse_gapt_proof(is);
        return BottomElimRule::create(thesis, form, subproof);
    } else if (type == "ForallIntro") {
        auto var = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_formula(is));
        gio::assert_or_throw<std::invalid_argument>(bool(var), "missing quantified variable");
        auto eigenvar = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_formula(is));
        gio::assert_or_throw<std::invalid_argument>(bool(eigenvar), "missing eigenvariable");
        auto subproof = parse_gapt_proof(is);
        return ForallIntroRule::create(thesis, var, eigenvar, subproof);
    } else {
        throw std::runtime_error("invalid proof type " + type);
    }
}

int read_gapt_main(int argc, char *argv[]) {
    using namespace gio;

    (void) argc;
    (void) argv;

    //auto &data = get_set_mm();
    //auto &lib = data.lib;
    //auto &tb = data.tb;

    auto proof = parse_gapt_proof(std::cin);
    std::cout << *proof << "\n";

    return 0;
}
gio_static_block {
    gio::register_main_function("read_gapt", read_gapt_main);
}
