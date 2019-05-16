
#include "gapt.h"

#include <giolib/main.h>
#include <giolib/static_block.h>
#include <giolib/utils.h>
#include <giolib/containers.h>

#include "mm/setmm.h"
#include "fof.h"

typedef std::shared_ptr<const gio::mmpp::provers::fof::FOT> term;
typedef std::shared_ptr<const gio::mmpp::provers::fof::FOF> formula;
typedef std::pair<std::vector<formula>, std::vector<formula>> sequent;
typedef std::pair<std::vector<formula>, formula> ndsequent;

term parse_gapt_term(std::istream &is) {
    using namespace gio::mmpp::provers::fof;
    std::string type;
    is >> type;
    if (type == "var") {
        std::string name;
        is >> name;
        return Variable::create(name);
    } else if (type == "unint") {
        std::string name;
        size_t num;
        is >> name >> num;
        std::vector<term> args;
        for (size_t i = 0; i < num; i++) {
            args.push_back(parse_gapt_term(is));
        }
        return Functor::create(name, args);
    } else {
        throw std::runtime_error("invalid formula type " + type);
    }
}

formula parse_gapt_formula(std::istream &is) {
    using namespace gio::mmpp::provers::fof;
    std::string type;
    is >> type;
    if (type == "exists") {
        auto var = std::dynamic_pointer_cast<const Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::runtime_error>(bool(var), "missing variable after quantifier");
        auto body = parse_gapt_formula(is);
        return Exists::create(var, body);
    } else if (type == "forall") {
        auto var = std::dynamic_pointer_cast<const Variable>(parse_gapt_term(is));
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
        std::vector<term> args;
        for (size_t i = 0; i < num; i++) {
            args.push_back(parse_gapt_term(is));
        }
        return Predicate::create(name, args);
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

ndsequent parse_gapt_ndsequent(std::istream &is) {
    sequent seq = parse_gapt_sequent(is);
    if (seq.second.size() != 1) {
        throw std::invalid_argument("sequent does not have exactly one succedent");
    }
    ndsequent ret;
    ret.first = std::move(seq.first);
    ret.second = std::move(seq.second.front());
    return ret;
}

void print_sequent(std::ostream &os, const sequent &seq) {
    using namespace gio;
    bool first = true;
    for (const auto &ant : seq.first) {
        if (!first) {
            os << ", ";
        }
        first = false;
        os << *ant;
    }
    if (!seq.first.empty()) {
        os << ' ';
    }
    os << "⊢";
    if (!seq.second.empty()) {
        os << ' ';
    }
    first = true;
    for (const auto &suc : seq.second) {
        if (!first) {
            os << ", ";
        }
        first = false;
        os << *suc;
    }
}

void print_ndsequent(std::ostream &os, const ndsequent &seq) {
    using namespace gio;
    bool first = true;
    for (const auto &ant : seq.first) {
        if (!first) {
            os << ", ";
        }
        first = false;
        os << *ant;
    }
    if (!seq.first.empty()) {
        os << ' ';
    }
    os << "⊢ ";
    os << *seq.second;
}

static std::pair<bool, size_t> decode_idx(ssize_t idx, bool is_suc) {
    // This should be implemented properly with boost::safe_numerics
    size_t ret;
    if (idx >= 0) {
        if (!is_suc) {
            return std::make_pair(false, 0);
        }
        ret = static_cast<size_t>(idx);
        if (static_cast<ssize_t>(ret) != idx) {
            return std::make_pair(false, 0);
        }
    } else {
        if (is_suc) {
            return std::make_pair(false, 0);
        }
        ret = static_cast<size_t>(-idx);
        if (static_cast<ssize_t>(ret) != -idx) {
            return std::make_pair(false, 0);
        }
        ret--;
    }
    return std::make_pair(true, ret);
}

class NDProof {
public:
    virtual ~NDProof() = default;

    virtual void print_to(std::ostream &s) const {
        s << "NDProof for ";
        print_ndsequent(s, this->thesis);
    }

    virtual bool check() const {
        //return true;
        throw std::runtime_error("not implemented in " + boost::typeindex::type_id_runtime(*this).pretty_name());
    }

    const ndsequent &get_thesis() const {
        return this->thesis;
    }

protected:
    NDProof(const ndsequent &thesis) : thesis(thesis) {}

private:
    ndsequent thesis;
};

typedef std::shared_ptr<const NDProof> proof;

class LogicalAxiom : public NDProof, public gio::virtual_enable_create<LogicalAxiom> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        const auto &th = this->get_thesis();
        if (th.first.size() != 1) return false;
        if (!gio::eq_cmp(fof_cmp())(*th.first[0], *this->form)) return false;
        if (!gio::eq_cmp(fof_cmp())(*th.second, *this->form)) return false;
        return true;
    }

protected:
    LogicalAxiom(const ndsequent &thesis, const formula &form)
        : NDProof(thesis), form(form) {}

private:
    formula form;
};

class ExistsIntroRule : public NDProof, public gio::virtual_enable_create<ExistsIntroRule> {
protected:
    ExistsIntroRule(const ndsequent &thesis, const formula &form, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &var, const term &subst_term, const proof &subproof)
        : NDProof(thesis), form(form), var(var), subst_term(subst_term), subproof(subproof) {}

private:
    formula form;
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> var;
    term subst_term;
    std::shared_ptr<const NDProof> subproof;
};

class ImpIntroRule : public NDProof, public gio::virtual_enable_create<ImpIntroRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        size_t ant_idx;
        bool valid;
        std::tie(valid, ant_idx) = decode_idx(this->ant_idx, false);
        if (!valid) return false;
        if (ant_idx >= sub_th.first.size()) return false;
        const auto &suc = th.second->mapped_dynamic_cast<const Implies>();
        if (!suc) return false;
        if (sub_th.first.size() < 1) return false;
        if (!gio::eq_cmp(fof_cmp())(*suc->get_left(), *sub_th.first[ant_idx])) return false;
        if (!gio::eq_cmp(fof_cmp())(*suc->get_right(), *sub_th.second)) return false;
        if (!gio::is_equal(gio::skipping_iterator(sub_th.first.begin(), sub_th.first.end(), {ant_idx}),
                           gio::skipping_iterator(sub_th.first.end(), sub_th.first.end(), {}),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

protected:
    ImpIntroRule(const ndsequent &thesis, ssize_t ant_idx, const proof &subproof)
        : NDProof(thesis), ant_idx(ant_idx), subproof(subproof) {}

private:
    ssize_t ant_idx;
    proof subproof;
};

class WeakeningRule : public NDProof, public gio::virtual_enable_create<WeakeningRule> {
protected:
    WeakeningRule(const ndsequent &thesis, const formula &form, const proof &subproof)
        : NDProof(thesis), form(form), subproof(subproof) {}

private:
    formula form;
    proof subproof;
};

class BottomElimRule : public NDProof, public gio::virtual_enable_create<BottomElimRule> {
protected:
    BottomElimRule(const ndsequent &thesis, const formula &form, const proof &subproof)
        : NDProof(thesis), form(form), subproof(subproof) {}

private:
    formula form;
    proof subproof;
};

class ForallElimRule : public NDProof, public gio::virtual_enable_create<ForallElimRule> {
protected:
    ForallElimRule(const ndsequent &thesis, const term &subst_term, const proof &subproof)
        : NDProof(thesis), subst_term(subst_term), subproof(subproof) {}

private:
    term subst_term;
    proof subproof;
};

class AndElim1Rule : public NDProof, public gio::virtual_enable_create<AndElim1Rule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        const auto &ant = sub_th.second->mapped_dynamic_cast<const And>();
        if (!ant) return false;
        if (!gio::eq_cmp(fof_cmp())(*ant->get_left(), *th.second)) return false;
        if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

protected:
    AndElim1Rule(const ndsequent &thesis, const proof &subproof)
        : NDProof(thesis), subproof(subproof) {}

private:
    proof subproof;
};

class AndElim2Rule : public NDProof, public gio::virtual_enable_create<AndElim2Rule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        const auto &ant = sub_th.second->mapped_dynamic_cast<const And>();
        if (!ant) return false;
        if (!gio::eq_cmp(fof_cmp())(*ant->get_right(), *th.second)) return false;
        if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

protected:
    AndElim2Rule(const ndsequent &thesis, const proof &subproof)
        : NDProof(thesis), subproof(subproof) {}

private:
    proof subproof;
};

class ForallIntroRule : public NDProof, public gio::virtual_enable_create<ForallIntroRule> {
protected:
    ForallIntroRule(const ndsequent &thesis, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &var, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &eigenvar, const proof &subproof)
        : NDProof(thesis), var(var), eigenvar(eigenvar), subproof(subproof) {}

private:
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> var, eigenvar;
    proof subproof;
};

class NegElimRule : public NDProof, public gio::virtual_enable_create<NegElimRule> {
protected:
    NegElimRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

private:
    proof left_proof, right_proof;
};

class ImpElimRule : public NDProof, public gio::virtual_enable_create<ImpElimRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->left_proof->check()) return false;
        if (!this->right_proof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &left_th = this->left_proof->get_thesis();
        const auto &right_th = this->right_proof->get_thesis();
        const auto &imp = left_th.second->mapped_dynamic_cast<const Implies>();
        if (!imp) return false;
        if (!gio::eq_cmp(fof_cmp())(*imp->get_left(), *right_th.second)) return false;
        if (!gio::eq_cmp(fof_cmp())(*imp->get_right(), *th.second)) return false;
        if (!gio::is_union(left_th.first.begin(), left_th.first.end(), right_th.first.begin(), right_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

protected:
    ImpElimRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

private:
    proof left_proof, right_proof;
};

class AndIntroRule : public NDProof, public gio::virtual_enable_create<AndIntroRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->left_proof->check()) return false;
        if (!this->right_proof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &left_th = this->left_proof->get_thesis();
        const auto &right_th = this->right_proof->get_thesis();
        const auto &suc = th.second->mapped_dynamic_cast<const And>();
        if (!suc) return false;
        if (!gio::eq_cmp(fof_cmp())(*suc->get_left(), *left_th.second)) return false;
        if (!gio::eq_cmp(fof_cmp())(*suc->get_right(), *right_th.second)) return false;
        if (!gio::is_union(left_th.first.begin(), left_th.first.end(), right_th.first.begin(), right_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

protected:
    AndIntroRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

private:
    proof left_proof, right_proof;
};

class ExistsElimRule : public NDProof, public gio::virtual_enable_create<ExistsElimRule> {
public:
    /*bool check() const override {
        // FIXME
        return true;
    }*/

protected:
    ExistsElimRule(const ndsequent &thesis, ssize_t idx, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &eigenvar, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), idx(idx), eigenvar(eigenvar), left_proof(left_proof), right_proof(right_proof) {}

private:
    ssize_t idx;
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> eigenvar;
    proof left_proof, right_proof;
};

class ContractionRule : public NDProof, public gio::virtual_enable_create<ContractionRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        bool valid;
        size_t contr_idx1, contr_idx2;
        std::tie(valid, contr_idx1) = decode_idx(this->contr_idx1, false);
        if (!valid) return false;
        if (contr_idx1 >= sub_th.first.size()) return false;
        std::tie(valid, contr_idx2) = decode_idx(this->contr_idx2, false);
        if (!valid) return false;
        if (contr_idx2 >= sub_th.first.size()) return false;
        if (th.first.size() < 1) return false;
        if (sub_th.first.size() < 2) return false;
        if (!gio::eq_cmp(fof_cmp())(*th.second, *sub_th.second)) return false;
        if (!gio::eq_cmp(fof_cmp())(*th.first.front(), *sub_th.first[contr_idx1])) return false;
        if (!gio::eq_cmp(fof_cmp())(*th.first.front(), *sub_th.first[contr_idx2])) return false;
        if (!gio::is_equal(gio::skipping_iterator(sub_th.first.begin(), sub_th.first.end(), {contr_idx1, contr_idx2}),
                           gio::skipping_iterator(sub_th.first.end(), sub_th.first.end(), {}),
                           th.first.begin()+1, th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

protected:
    ContractionRule(const ndsequent &thesis, ssize_t contr_idx1, ssize_t contr_idx2, const proof &subproof)
        : NDProof(thesis), contr_idx1(contr_idx1), contr_idx2(contr_idx2), subproof(subproof) {}

private:
    ssize_t contr_idx1, contr_idx2;
    proof subproof;
};

class ExcludedMiddleRule : public NDProof, public gio::virtual_enable_create<ExcludedMiddleRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->left_proof->check()) return false;
        if (!this->right_proof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &left_th = this->left_proof->get_thesis();
        const auto &right_th = this->right_proof->get_thesis();
        bool valid;
        size_t left_idx, right_idx;
        std::tie(valid, left_idx) = decode_idx(this->left_idx, false);
        if (!valid) return false;
        if (left_idx >= left_th.first.size()) return false;
        std::tie(valid, right_idx) = decode_idx(this->right_idx, false);
        if (!valid) return false;
        if (right_idx >= right_th.first.size()) return false;
        if (!gio::eq_cmp(fof_cmp())(*left_th.second, *right_th.second)) return false;
        if (!gio::eq_cmp(fof_cmp())(*left_th.second, *th.second)) return false;
        const auto &absurd = right_th.first[right_idx]->mapped_dynamic_cast<const Not>();
        if (!absurd) return false;
        if (!gio::eq_cmp(fof_cmp())(*absurd->get_arg(), *left_th.first[left_idx])) return false;
        if (!gio::is_union(gio::skipping_iterator(left_th.first.begin(), left_th.first.end(), {left_idx}),
                           gio::skipping_iterator(left_th.first.end(), left_th.first.end(), {}),
                           gio::skipping_iterator(right_th.first.begin(), right_th.first.end(), {right_idx}),
                           gio::skipping_iterator(right_th.first.end(), right_th.first.end(), {}),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

protected:
    ExcludedMiddleRule(const ndsequent &thesis, ssize_t left_idx, ssize_t right_idx, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_idx(left_idx), right_idx(right_idx), left_proof(left_proof), right_proof(right_proof) {}

private:
    ssize_t left_idx, right_idx;
    proof left_proof, right_proof;
};

std::shared_ptr<const NDProof> parse_gapt_proof(std::istream &is) {
    auto thesis = parse_gapt_ndsequent(is);
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
        auto eigenvar = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
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
        auto var = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(var), "missing substitution variable");
        auto term = parse_gapt_term(is);
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
    } else if (type == "ForallElim") {
        auto term = parse_gapt_term(is);
        auto subproof = parse_gapt_proof(is);
        return ForallElimRule::create(thesis, term, subproof);
    } else if (type == "ForallIntro") {
        auto var = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(var), "missing quantified variable");
        auto eigenvar = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
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
    bool valid = proof->check();
    gio::assert_or_throw<std::runtime_error>(valid, "invalid proof!");

    return 0;
}
gio_static_block {
    gio::register_main_function("read_gapt", read_gapt_main);
}
