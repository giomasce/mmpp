
#include "gapt.h"

#include <type_traits>

#include <giolib/main.h>
#include <giolib/static_block.h>
#include <giolib/utils.h>
#include <giolib/containers.h>
#include <giolib/std_printers.h>

#include "mm/setmm_loader.h"
#include "fof.h"
#include "fof_to_mm.h"

typedef std::shared_ptr<const gio::mmpp::provers::fof::FOT> term;
typedef std::shared_ptr<const gio::mmpp::provers::fof::FOF> formula;
typedef std::pair<std::vector<formula>, std::vector<formula>> sequent;
typedef std::pair<std::vector<formula>, formula> ndsequent;
typedef std::make_signed_t<size_t> idx_t;

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
    } else if (type == "equal") {
        auto left = parse_gapt_term(is);
        auto right = parse_gapt_term(is);
        return Equal::create(left, right);
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

static std::pair<bool, size_t> decode_idx(idx_t idx, bool is_suc) {
    // This should be implemented properly with boost::safe_numerics
    size_t ret;
    if (idx >= 0) {
        if (!is_suc) {
            return std::make_pair(false, 0);
        }
        ret = static_cast<size_t>(idx);
        if (static_cast<idx_t>(ret) != idx) {
            return std::make_pair(false, 0);
        }
    } else {
        if (is_suc) {
            return std::make_pair(false, 0);
        }
        ret = static_cast<size_t>(-idx);
        if (static_cast<idx_t>(ret) != -idx) {
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

    virtual bool check() const = 0;
    virtual std::vector<std::shared_ptr<const NDProof>> get_subproofs() const = 0;

    std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> collect_vars_functs_preds() const {
        std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> ret;
        this->collect_vars_functs_preds(ret);
        return ret;
    }

    void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const {
        for (const auto &ant : this->thesis.first) {
            ant->collect_vars_functs_preds(vars_functs_preds);
        }
        this->thesis.second->collect_vars_functs_preds(vars_functs_preds);
        for (const auto &subproof : this->get_subproofs()) {
            subproof->collect_vars_functs_preds(vars_functs_preds);
        }
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

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {};
    }

protected:
    LogicalAxiom(const ndsequent &thesis, const formula &form)
        : NDProof(thesis), form(form) {}

private:
    formula form;
};

class WeakeningRule : public NDProof, public gio::virtual_enable_create<WeakeningRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        if (th.first.size() < 1) return false;
        if (!gio::eq_cmp(fof_cmp())(*this->form, *th.first[0])) return false;
        if (!gio::eq_cmp(fof_cmp())(*sub_th.second, *th.second)) return false;
        if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                           th.first.begin()+1, th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->subproof};
    }

protected:
    WeakeningRule(const ndsequent &thesis, const formula &form, const proof &subproof)
        : NDProof(thesis), form(form), subproof(subproof) {}

private:
    formula form;
    proof subproof;
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

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->subproof};
    }

protected:
    ContractionRule(const ndsequent &thesis, idx_t contr_idx1, idx_t contr_idx2, const proof &subproof)
        : NDProof(thesis), contr_idx1(contr_idx1), contr_idx2(contr_idx2), subproof(subproof) {}

private:
    idx_t contr_idx1, contr_idx2;
    proof subproof;
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
        const auto suc = th.second->mapped_dynamic_cast<const And>();
        if (!suc) return false;
        if (!gio::eq_cmp(fof_cmp())(*suc->get_left(), *left_th.second)) return false;
        if (!gio::eq_cmp(fof_cmp())(*suc->get_right(), *right_th.second)) return false;
        if (!gio::is_union(left_th.first.begin(), left_th.first.end(), right_th.first.begin(), right_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->left_proof, this->right_proof};
    }

protected:
    AndIntroRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

private:
    proof left_proof, right_proof;
};

class AndElim1Rule : public NDProof, public gio::virtual_enable_create<AndElim1Rule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        const auto ant = sub_th.second->mapped_dynamic_cast<const And>();
        if (!ant) return false;
        if (!gio::eq_cmp(fof_cmp())(*ant->get_left(), *th.second)) return false;
        if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->subproof};
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
        const auto ant = sub_th.second->mapped_dynamic_cast<const And>();
        if (!ant) return false;
        if (!gio::eq_cmp(fof_cmp())(*ant->get_right(), *th.second)) return false;
        if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->subproof};
    }

protected:
    AndElim2Rule(const ndsequent &thesis, const proof &subproof)
        : NDProof(thesis), subproof(subproof) {}

private:
    proof subproof;
};

class OrIntro1Rule : public NDProof, public gio::virtual_enable_create<OrIntro1Rule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        if (!gio::eq_cmp(fof_cmp())(*std::static_pointer_cast<const FOF>(Or::create(sub_th.second, this->disjunct)), *th.second)) return false;
        if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->subproof};
    }

protected:
    OrIntro1Rule(const ndsequent &thesis, const formula &disjunct, const proof &subproof)
        : NDProof(thesis), disjunct(disjunct), subproof(subproof) {}

private:
    formula disjunct;
    proof subproof;
};

class OrIntro2Rule : public NDProof, public gio::virtual_enable_create<OrIntro2Rule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        if (!gio::eq_cmp(fof_cmp())(*std::static_pointer_cast<const FOF>(Or::create(this->disjunct, sub_th.second)), *th.second)) return false;
        if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->subproof};
    }

protected:
    OrIntro2Rule(const ndsequent &thesis, const formula &disjunct, const proof &subproof)
        : NDProof(thesis), disjunct(disjunct), subproof(subproof) {}

private:
    formula disjunct;
    proof subproof;
};

class OrElimRule : public NDProof, public gio::virtual_enable_create<OrElimRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->left_proof->check()) return false;
        if (!this->middle_proof->check()) return false;
        if (!this->right_proof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &left_th = this->left_proof->get_thesis();
        const auto &middle_th = this->middle_proof->get_thesis();
        const auto &right_th = this->right_proof->get_thesis();
        bool valid;
        size_t middle_idx, right_idx;
        std::tie(valid, middle_idx) = decode_idx(this->middle_idx, false);
        if (!valid) return false;
        if (middle_idx >= middle_th.first.size()) return false;
        std::tie(valid, right_idx) = decode_idx(this->right_idx, false);
        if (!valid) return false;
        if (right_idx >= right_th.first.size()) return false;
        if (!gio::eq_cmp(fof_cmp())(*left_th.second, *std::static_pointer_cast<const FOF>(Or::create(middle_th.first[middle_idx], right_th.first[right_idx])))) return false;
        if (!gio::eq_cmp(fof_cmp())(*th.second, *middle_th.second)) return false;
        if (!gio::eq_cmp(fof_cmp())(*th.second, *right_th.second)) return false;
        if (!gio::is_union3(left_th.first.begin(), left_th.first.end(),
                            gio::skipping_iterator(middle_th.first.begin(), middle_th.first.end(), {middle_idx}),
                            gio::skipping_iterator(middle_th.first.end(), middle_th.first.end(), {}),
                            gio::skipping_iterator(right_th.first.begin(), right_th.first.end(), {right_idx}),
                            gio::skipping_iterator(right_th.first.end(), right_th.first.end(), {}),
                            th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->left_proof, this->middle_proof, this->right_proof};
    }

protected:
    OrElimRule(const ndsequent &thesis, idx_t middle_idx, idx_t right_idx, const proof &left_proof, const proof &middle_proof, const proof &right_proof)
        : NDProof(thesis), middle_idx(middle_idx), right_idx(right_idx), left_proof(left_proof), middle_proof(middle_proof), right_proof(right_proof) {}

private:
    idx_t middle_idx, right_idx;
    proof left_proof, middle_proof, right_proof;
};

class NegElimRule : public NDProof, public gio::virtual_enable_create<NegElimRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->left_proof->check()) return false;
        if (!this->right_proof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &left_th = this->left_proof->get_thesis();
        const auto &right_th = this->right_proof->get_thesis();
        const auto neg = left_th.second->mapped_dynamic_cast<const Not>();
        if (!neg) return false;
        if (!gio::eq_cmp(fof_cmp())(*neg->get_arg(), *right_th.second)) return false;
        if (!gio::eq_cmp(fof_cmp())(*th.second, *std::static_pointer_cast<const FOF>(False::create()))) return false;
        if (!gio::is_union(left_th.first.begin(), left_th.first.end(), right_th.first.begin(), right_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->left_proof, this->right_proof};
    }

protected:
    NegElimRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

private:
    proof left_proof, right_proof;
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
        const auto suc = th.second->mapped_dynamic_cast<const Implies>();
        if (!suc) return false;
        if (sub_th.first.size() < 1) return false;
        if (!gio::eq_cmp(fof_cmp())(*suc->get_left(), *sub_th.first[ant_idx])) return false;
        if (!gio::eq_cmp(fof_cmp())(*suc->get_right(), *sub_th.second)) return false;
        if (!gio::is_equal(gio::skipping_iterator(sub_th.first.begin(), sub_th.first.end(), {ant_idx}),
                           gio::skipping_iterator(sub_th.first.end(), sub_th.first.end(), {}),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->subproof};
    }

protected:
    ImpIntroRule(const ndsequent &thesis, idx_t ant_idx, const proof &subproof)
        : NDProof(thesis), ant_idx(ant_idx), subproof(subproof) {}

private:
    idx_t ant_idx;
    proof subproof;
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
        const auto imp = left_th.second->mapped_dynamic_cast<const Implies>();
        if (!imp) return false;
        if (!gio::eq_cmp(fof_cmp())(*imp->get_left(), *right_th.second)) return false;
        if (!gio::eq_cmp(fof_cmp())(*imp->get_right(), *th.second)) return false;
        if (!gio::is_union(left_th.first.begin(), left_th.first.end(), right_th.first.begin(), right_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->left_proof, this->right_proof};
    }

protected:
    ImpElimRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

private:
    proof left_proof, right_proof;
};

class BottomElimRule : public NDProof, public gio::virtual_enable_create<BottomElimRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        if (!gio::eq_cmp(fof_cmp())(*sub_th.second, *std::static_pointer_cast<const FOF>(False::create()))) return false;
        if (!gio::eq_cmp(fof_cmp())(*th.second, *this->form)) return false;
        if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->subproof};
    }

protected:
    BottomElimRule(const ndsequent &thesis, const formula &form, const proof &subproof)
        : NDProof(thesis), form(form), subproof(subproof) {}

private:
    formula form;
    proof subproof;
};

class ForallIntroRule : public NDProof, public gio::virtual_enable_create<ForallIntroRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        const auto all = th.second->mapped_dynamic_cast<const Forall>();
        if (!all) return false;
        const auto &var = all->get_var();
        if (!gio::eq_cmp(fot_cmp())(*var, *this->var)) return false;
        for (const auto &ant : sub_th.first) {
            if (ant->has_free_var(this->eigenvar->get_name())) return false;
        }
        if (!gio::eq_cmp(fot_cmp())(*var, *this->var)) {
            if (all->get_arg()->has_free_var(this->eigenvar->get_name())) return false;
        }
        if (!gio::eq_cmp(fof_cmp())(*all->get_arg()->replace(var->get_name(), this->eigenvar), *sub_th.second)) return false;
        if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->subproof};
    }

protected:
    ForallIntroRule(const ndsequent &thesis, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &var, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &eigenvar, const proof &subproof)
        : NDProof(thesis), var(var), eigenvar(eigenvar), subproof(subproof) {}

private:
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> var, eigenvar;
    proof subproof;
};

class ForallElimRule : public NDProof, public gio::virtual_enable_create<ForallElimRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        const auto all = sub_th.second->mapped_dynamic_cast<const Forall>();
        if (!all) return false;
        const auto &var = all->get_var();
        if (!gio::eq_cmp(fof_cmp())(*all->get_arg()->replace(var->get_name(), this->subst_term), *th.second)) return false;
        if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->subproof};
    }

protected:
    ForallElimRule(const ndsequent &thesis, const term &subst_term, const proof &subproof)
        : NDProof(thesis), subst_term(subst_term), subproof(subproof) {}

private:
    term subst_term;
    proof subproof;
};

class ExistsIntroRule : public NDProof, public gio::virtual_enable_create<ExistsIntroRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->subproof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &sub_th = this->subproof->get_thesis();
        const auto ex = th.second->mapped_dynamic_cast<const Exists>();
        if (!ex) return false;
        const auto &var = ex->get_var();
        if (!gio::eq_cmp(fot_cmp())(*var, *this->var)) return false;
        if (!gio::eq_cmp(fof_cmp())(*ex->get_arg(), *this->form)) return false;
        if (!gio::eq_cmp(fof_cmp())(*ex->get_arg()->replace(var->get_name(), this->subst_term), *sub_th.second)) return false;
        if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->subproof};
    }

protected:
    ExistsIntroRule(const ndsequent &thesis, const formula &form, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &var, const term &subst_term, const proof &subproof)
        : NDProof(thesis), form(form), var(var), subst_term(subst_term), subproof(subproof) {}

private:
    formula form;
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> var;
    term subst_term;
    std::shared_ptr<const NDProof> subproof;
};

class ExistsElimRule : public NDProof, public gio::virtual_enable_create<ExistsElimRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->left_proof->check()) return false;
        if (!this->right_proof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &left_th = this->left_proof->get_thesis();
        const auto &right_th = this->right_proof->get_thesis();
        size_t idx;
        bool valid;
        std::tie(valid, idx) = decode_idx(this->idx, false);
        if (!valid) return false;
        if (idx >= right_th.first.size()) return false;
        const auto ex = left_th.second->mapped_dynamic_cast<const Exists>();
        if (!ex) return false;
        const auto &var = ex->get_var();
        for (const auto &ant : gio::iterator_pair(gio::skipping_iterator(right_th.first.begin(), right_th.first.end(), {idx}),
                                                  gio::skipping_iterator(right_th.first.end(), right_th.first.end(), {}))) {
            if (ant->has_free_var(this->eigenvar->get_name())) return false;
        }
        if (right_th.second->has_free_var(this->eigenvar->get_name())) return false;
        if (!gio::eq_cmp(fot_cmp())(*var, *this->eigenvar)) {
            if (ex->get_arg()->has_free_var(this->eigenvar->get_name())) return false;
        }
        if (!gio::eq_cmp(fof_cmp())(*ex->get_arg()->replace(var->get_name(), this->eigenvar), *right_th.first[idx])) return false;
        if (!gio::eq_cmp(fof_cmp())(*right_th.second, *th.second)) return false;
        if (!gio::is_union(left_th.first.begin(), left_th.first.end(),
                           gio::skipping_iterator(right_th.first.begin(), right_th.first.end(), {idx}),
                           gio::skipping_iterator(right_th.first.end(), right_th.first.end(), {}),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->left_proof, this->right_proof};
    }

protected:
    ExistsElimRule(const ndsequent &thesis, idx_t idx, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &eigenvar, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), idx(idx), eigenvar(eigenvar), left_proof(left_proof), right_proof(right_proof) {}

private:
    idx_t idx;
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> eigenvar;
    proof left_proof, right_proof;
};

class EqualityIntroRule : public NDProof, public gio::virtual_enable_create<EqualityIntroRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        const auto &th = this->get_thesis();
        if (!th.first.empty()) return false;
        if (!gio::eq_cmp(fof_cmp())(*th.second, *std::static_pointer_cast<const FOF>(Equal::create(this->t, this->t)))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {};
    }

protected:
    EqualityIntroRule(const ndsequent &thesis, const term &t)
        : NDProof(thesis), t(t) {}

private:
    term t;
};

class EqualityElimRule : public NDProof, public gio::virtual_enable_create<EqualityElimRule> {
public:
    bool check() const override {
        using namespace gio::mmpp::provers::fof;
        if (!this->left_proof->check()) return false;
        if (!this->right_proof->check()) return false;
        const auto &th = this->get_thesis();
        const auto &left_th = this->left_proof->get_thesis();
        const auto &right_th = this->right_proof->get_thesis();
        const auto eq = left_th.second->mapped_dynamic_cast<const Equal>();
        if (!eq) return false;
        // The equality might be used in a sense or in the other, and we don't know it, so we have to check both possibilities
        bool ok_direct = true;
        bool ok_inverted = true;
        if (!gio::eq_cmp(fof_cmp())(*right_th.second, *this->form->replace(this->var->get_name(), eq->get_left()))) ok_direct = false;
        if (!gio::eq_cmp(fof_cmp())(*th.second, *this->form->replace(this->var->get_name(), eq->get_right()))) ok_direct = false;
        if (!gio::eq_cmp(fof_cmp())(*right_th.second, *this->form->replace(this->var->get_name(), eq->get_right()))) ok_inverted = false;
        if (!gio::eq_cmp(fof_cmp())(*th.second, *this->form->replace(this->var->get_name(), eq->get_left()))) ok_inverted = false;
        if (!ok_direct && !ok_inverted) return false;
        if (!gio::is_union(left_th.first.begin(), left_th.first.end(), right_th.first.begin(), right_th.first.end(),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->left_proof, this->right_proof};
    }

protected:
    EqualityElimRule(const ndsequent &thesis, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &var, const formula &form, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), var(var), form(form), left_proof(left_proof), right_proof(right_proof) {}

private:
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> var;
    formula form;
    proof left_proof, right_proof;
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
        const auto absurd = right_th.first[right_idx]->mapped_dynamic_cast<const Not>();
        if (!absurd) return false;
        if (!gio::eq_cmp(fof_cmp())(*absurd->get_arg(), *left_th.first[left_idx])) return false;
        if (!gio::is_union(gio::skipping_iterator(left_th.first.begin(), left_th.first.end(), {left_idx}),
                           gio::skipping_iterator(left_th.first.end(), left_th.first.end(), {}),
                           gio::skipping_iterator(right_th.first.begin(), right_th.first.end(), {right_idx}),
                           gio::skipping_iterator(right_th.first.end(), right_th.first.end(), {}),
                           th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
        return true;
    }

    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override {
        return {this->left_proof, this->right_proof};
    }

protected:
    ExcludedMiddleRule(const ndsequent &thesis, idx_t left_idx, idx_t right_idx, const proof &left_proof, const proof &right_proof)
        : NDProof(thesis), left_idx(left_idx), right_idx(right_idx), left_proof(left_proof), right_proof(right_proof) {}

private:
    idx_t left_idx, right_idx;
    proof left_proof, right_proof;
};

std::shared_ptr<const NDProof> parse_gapt_proof(std::istream &is) {
    auto thesis = parse_gapt_ndsequent(is);
    std::string type;
    is >> type;
    if (type == "LogicalAxiom") {
        auto form = parse_gapt_formula(is);
        return LogicalAxiom::create(thesis, form);
    } else if (type == "Weakening") {
        auto form = parse_gapt_formula(is);
        auto subproof = parse_gapt_proof(is);
        return WeakeningRule::create(thesis, form, subproof);
    } else if (type == "Contraction") {
        idx_t idx1, idx2;
        is >> idx1 >> idx2;
        auto subproof = parse_gapt_proof(is);
        return ContractionRule::create(thesis, idx1, idx2, subproof);
    } else if (type == "AndIntro") {
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return AndIntroRule::create(thesis, left_proof, right_proof);
    } else if (type == "AndElim1") {
        auto subproof = parse_gapt_proof(is);
        return AndElim1Rule::create(thesis, subproof);
    } else if (type == "AndElim2") {
        auto subproof = parse_gapt_proof(is);
        return AndElim2Rule::create(thesis, subproof);
    } else if (type == "OrIntro1") {
        auto disjunct = parse_gapt_formula(is);
        auto subproof = parse_gapt_proof(is);
        return OrIntro1Rule::create(thesis, disjunct, subproof);
    } else if (type == "OrIntro2") {
        auto disjunct = parse_gapt_formula(is);
        auto subproof = parse_gapt_proof(is);
        return OrIntro2Rule::create(thesis, disjunct, subproof);
    } else if (type == "OrElim") {
        idx_t middle_idx, right_idx;
        is >> middle_idx >> right_idx;
        auto left_proof = parse_gapt_proof(is);
        auto middle_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return OrElimRule::create(thesis, middle_idx, right_idx, left_proof, middle_proof, right_proof);
    } else if (type == "NegElim") {
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return NegElimRule::create(thesis, left_proof, right_proof);
    } else if (type == "ImpIntro") {
        idx_t ant_idx;
        is >> ant_idx;
        auto subproof = parse_gapt_proof(is);
        return ImpIntroRule::create(thesis, ant_idx, subproof);
    } else if (type == "ImpElim") {
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return ImpElimRule::create(thesis, left_proof, right_proof);
    } else if (type == "BottomElim") {
        auto form = parse_gapt_formula(is);
        auto subproof = parse_gapt_proof(is);
        return BottomElimRule::create(thesis, form, subproof);
    } else if (type == "ForallIntro") {
        auto var = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(var), "missing quantified variable");
        auto eigenvar = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(eigenvar), "missing eigenvariable");
        auto subproof = parse_gapt_proof(is);
        return ForallIntroRule::create(thesis, var, eigenvar, subproof);
    } else if (type == "ForallElim") {
        auto term = parse_gapt_term(is);
        auto subproof = parse_gapt_proof(is);
        return ForallElimRule::create(thesis, term, subproof);
    } else if (type == "ExistsIntro") {
        auto form = parse_gapt_formula(is);
        auto var = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(var), "missing substitution variable");
        auto term = parse_gapt_term(is);
        auto subproof = parse_gapt_proof(is);
        return ExistsIntroRule::create(thesis, form, var, term, subproof);
    } else if (type == "ExistsElim") {
        idx_t idx;
        is >> idx;
        auto eigenvar = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(eigenvar), "missing eigenvariable");
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return ExistsElimRule::create(thesis, idx, eigenvar, left_proof, right_proof);
    } else if (type == "EqualityIntro") {
        auto t = parse_gapt_term(is);
        return EqualityIntroRule::create(thesis, t);
    } else if (type == "EqualityElim") {
        auto var = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(var), "missing substitution variable");
        auto form = parse_gapt_formula(is);
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return EqualityElimRule::create(thesis, var, form, left_proof, right_proof);
    } else if (type == "ExcludedMiddle") {
        idx_t left_idx, right_idx;
        is >> left_idx >> right_idx;
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return ExcludedMiddleRule::create(thesis, left_idx, right_idx, left_proof, right_proof);
    } else {
        throw std::runtime_error("invalid proof type " + type);
    }
}

int read_gapt_main(int argc, char *argv[]) {
    using namespace gio;
    using namespace gio::std_printers;
    using namespace gio::mmpp::provers::fof;

    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto proof = parse_gapt_proof(std::cin);
    std::cout << *proof << "\n";
    bool valid = proof->check();
    gio::assert_or_throw<std::runtime_error>(valid, "invalid proof!");

    auto vars_functs_preds = proof->collect_vars_functs_preds();
    std::cout << vars_functs_preds << "\n";

    fof_to_mm_ctx ctx(tb);
    ctx.alloc_vars(std::get<0>(vars_functs_preds));
    ctx.alloc_functs(std::get<1>(vars_functs_preds));
    ctx.alloc_preds(std::get<2>(vars_functs_preds));
    /*auto pt = ctx.convert(proof->get_thesis().second);
    std::cout << tb.print_sentence(pt) << "\n";*/

    return 0;
}
gio_static_block {
    gio::register_main_function("read_gapt", read_gapt_main);
}
