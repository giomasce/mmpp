#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include <giolib/inheritance.h>

#include "fof.h"

namespace gio::mmpp::provers::ndproof {

typedef std::shared_ptr<const gio::mmpp::provers::fof::FOT> term;
typedef std::shared_ptr<const gio::mmpp::provers::fof::FOF> formula;
typedef std::pair<std::vector<formula>, std::vector<formula>> sequent;
typedef std::pair<std::vector<formula>, formula> ndsequent;
typedef std::make_signed_t<size_t> idx_t;

void print_sequent(std::ostream &os, const sequent &seq);
void print_ndsequent(std::ostream &os, const ndsequent &seq);
std::pair<bool, size_t> decode_idx(idx_t idx, bool is_suc);
size_t safe_decode_idx(idx_t idx, bool is_suc);

class NDProof;
class LogicalAxiom;
class WeakeningRule;
class ContractionRule;
class AndIntroRule;
class AndElim1Rule;
class AndElim2Rule;
class OrIntro1Rule;
class OrIntro2Rule;
class OrElimRule;
class NegElimRule;
class ImpIntroRule;
class ImpElimRule;
class BottomElimRule;
class ForallIntroRule;
class ForallElimRule;
class ExistsIntroRule;
class ExistsElimRule;
class EqualityIntroRule;
class EqualityElimRule;
class ExcludedMiddleRule;

struct ndproof_inheritance {
    typedef NDProof base;
    typedef boost::mpl::vector<LogicalAxiom, WeakeningRule, ContractionRule, AndIntroRule, AndElim1Rule, AndElim2Rule,
    OrIntro1Rule, OrIntro2Rule, OrElimRule, NegElimRule, ImpIntroRule, ImpElimRule, BottomElimRule,
    ForallIntroRule, ForallElimRule, ExistsIntroRule, ExistsElimRule, EqualityIntroRule, EqualityElimRule, ExcludedMiddleRule> subtypes;
};

class NDProof : public inheritance_base<ndproof_inheritance> {
public:
    virtual ~NDProof() = default;
    virtual void print_to(std::ostream &s) const;
    virtual bool check() const = 0;
    virtual std::vector<std::shared_ptr<const NDProof>> get_subproofs() const = 0;
    std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> collect_vars_functs_preds() const;
    void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const;
    const ndsequent &get_thesis() const;

protected:
    NDProof(const ndsequent &thesis);

private:
    ndsequent thesis;
};

typedef std::shared_ptr<const NDProof> proof;

class LogicalAxiom : public NDProof, public gio::virtual_enable_create<LogicalAxiom>, public inheritance_impl<LogicalAxiom, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const formula &get_formula() const;

protected:
    LogicalAxiom(const ndsequent &thesis, const formula &form);

private:
    formula form;
};

class WeakeningRule : public NDProof, public gio::virtual_enable_create<WeakeningRule>, public inheritance_impl<WeakeningRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const formula &get_new_conjunct() const;

protected:
    WeakeningRule(const ndsequent &thesis, const formula &form, const proof &subproof);

private:
    formula form;
    proof subproof;
};

class ContractionRule : public NDProof, public gio::virtual_enable_create<ContractionRule>, public inheritance_impl<ContractionRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    size_t get_contr_idx1() const;
    size_t get_contr_idx2() const;

protected:
    ContractionRule(const ndsequent &thesis, idx_t contr_idx1, idx_t contr_idx2, const proof &subproof);

private:
    idx_t contr_idx1, contr_idx2;
    proof subproof;
};

class AndIntroRule : public NDProof, public gio::virtual_enable_create<AndIntroRule>, public inheritance_impl<AndIntroRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;

protected:
    AndIntroRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof);

private:
    proof left_proof, right_proof;
};

class AndElim1Rule : public NDProof, public gio::virtual_enable_create<AndElim1Rule>, public inheritance_impl<AndElim1Rule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const formula &get_conjunct() const;

protected:
    AndElim1Rule(const ndsequent &thesis, const proof &subproof);

private:
    proof subproof;
};

class AndElim2Rule : public NDProof, public gio::virtual_enable_create<AndElim2Rule>, public inheritance_impl<AndElim2Rule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const formula &get_conjunct() const;

protected:
    AndElim2Rule(const ndsequent &thesis, const proof &subproof);

private:
    proof subproof;
};

class OrIntro1Rule : public NDProof, public gio::virtual_enable_create<OrIntro1Rule>, public inheritance_impl<OrIntro1Rule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const formula &get_disjunct() const;

protected:
    OrIntro1Rule(const ndsequent &thesis, const formula &disjunct, const proof &subproof);

private:
    formula disjunct;
    proof subproof;
};

class OrIntro2Rule : public NDProof, public gio::virtual_enable_create<OrIntro2Rule>, public inheritance_impl<OrIntro2Rule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const formula &get_disjunct() const;

protected:
    OrIntro2Rule(const ndsequent &thesis, const formula &disjunct, const proof &subproof);

private:
    formula disjunct;
    proof subproof;
};

class OrElimRule : public NDProof, public gio::virtual_enable_create<OrElimRule>, public inheritance_impl<OrElimRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    size_t get_middle_idx() const;
    size_t get_right_idx() const;

protected:
    OrElimRule(const ndsequent &thesis, idx_t middle_idx, idx_t right_idx, const proof &left_proof, const proof &middle_proof, const proof &right_proof);

private:
    idx_t middle_idx, right_idx;
    proof left_proof, middle_proof, right_proof;
};

class NegElimRule : public NDProof, public gio::virtual_enable_create<NegElimRule>, public inheritance_impl<NegElimRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;

protected:
    NegElimRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof);

private:
    proof left_proof, right_proof;
};

class ImpIntroRule : public NDProof, public gio::virtual_enable_create<ImpIntroRule>, public inheritance_impl<ImpIntroRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    size_t get_ant_idx() const;

protected:
    ImpIntroRule(const ndsequent &thesis, idx_t ant_idx, const proof &subproof);

private:
    idx_t ant_idx;
    proof subproof;
};

class ImpElimRule : public NDProof, public gio::virtual_enable_create<ImpElimRule>, public inheritance_impl<ImpElimRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;

protected:
    ImpElimRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof);

private:
    proof left_proof, right_proof;
};

class BottomElimRule : public NDProof, public gio::virtual_enable_create<BottomElimRule>, public inheritance_impl<BottomElimRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const formula &get_succedent() const;

protected:
    BottomElimRule(const ndsequent &thesis, const formula &form, const proof &subproof);

private:
    formula form;
    proof subproof;
};

class ForallIntroRule : public NDProof, public gio::virtual_enable_create<ForallIntroRule>, public inheritance_impl<ForallIntroRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &get_var() const;
    const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &get_eigenvar() const;
    const formula &get_predicate() const;

protected:
    ForallIntroRule(const ndsequent &thesis, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &var, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &eigenvar, const proof &subproof);

private:
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> var, eigenvar;
    proof subproof;
};

class ForallElimRule : public NDProof, public gio::virtual_enable_create<ForallElimRule>, public inheritance_impl<ForallElimRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const term &get_subst_term() const;
    const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &get_var() const;
    const formula &get_predicate() const;

protected:
    ForallElimRule(const ndsequent &thesis, const term &subst_term, const proof &subproof);

private:
    term subst_term;
    proof subproof;
};

class ExistsIntroRule : public NDProof, public gio::virtual_enable_create<ExistsIntroRule>, public inheritance_impl<ExistsIntroRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const term &get_subst_term() const;
    const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &get_var() const;
    const formula &get_predicate() const;

protected:
    ExistsIntroRule(const ndsequent &thesis, const formula &form, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &var, const term &subst_term, const proof &subproof);

private:
    formula form;
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> var;
    term subst_term;
    std::shared_ptr<const NDProof> subproof;
};

class ExistsElimRule : public NDProof, public gio::virtual_enable_create<ExistsElimRule>, public inheritance_impl<ExistsElimRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    size_t get_right_idx() const;
    const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &get_eigenvar() const;
    const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &get_var() const;
    const formula &get_predicate() const;

protected:
    ExistsElimRule(const ndsequent &thesis, idx_t idx, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &eigenvar, const proof &left_proof, const proof &right_proof);

private:
    idx_t idx;
    std::shared_ptr<const gio::mmpp::provers::fof::Variable> eigenvar;
    proof left_proof, right_proof;
};

class EqualityIntroRule : public NDProof, public gio::virtual_enable_create<EqualityIntroRule>, public inheritance_impl<EqualityIntroRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const term &get_term() const;

protected:
    EqualityIntroRule(const ndsequent &thesis, const term &t);

private:
    term t;
};

class EqualityElimRule : public NDProof, public gio::virtual_enable_create<EqualityElimRule>, public inheritance_impl<EqualityElimRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    const formula &get_subst_formula() const;
    const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &get_var() const;
    bool is_reversed() const;
    const term &get_left_term() const;
    const term &get_right_term() const;

protected:
    EqualityElimRule(const ndsequent &thesis, const std::shared_ptr<const gio::mmpp::provers::fof::Variable> &var, const formula &form, const proof &left_proof, const proof &right_proof);

private:
    std::pair<bool, bool> check_reversed() const;

    std::shared_ptr<const gio::mmpp::provers::fof::Variable> var;
    formula form;
    proof left_proof, right_proof;
};

class ExcludedMiddleRule : public NDProof, public gio::virtual_enable_create<ExcludedMiddleRule>, public inheritance_impl<ExcludedMiddleRule, ndproof_inheritance> {
public:
    bool check() const override;
    std::vector<std::shared_ptr<const NDProof>> get_subproofs() const override;
    size_t get_left_idx() const;
    size_t get_right_idx() const;

protected:
    ExcludedMiddleRule(const ndsequent &thesis, idx_t left_idx, idx_t right_idx, const proof &left_proof, const proof &right_proof);

private:
    idx_t left_idx, right_idx;
    proof left_proof, right_proof;
};

}
