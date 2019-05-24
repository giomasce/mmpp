
#include "ndproof.h"

namespace gio::mmpp::provers::ndproof {

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

std::pair<bool, size_t> decode_idx(idx_t idx, bool is_suc) {
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

size_t safe_decode_idx(idx_t idx, bool is_suc) {
    auto res = decode_idx(idx, is_suc);
    gio::assert_or_throw<std::invalid_argument>(res.first, "invalid index");
    return res.second;
}

void NDProof::print_to(std::ostream &s) const {
    s << "NDProof for ";
    print_ndsequent(s, this->thesis);
}

std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t> > NDProof::collect_vars_functs_preds() const {
    std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> ret;
    this->collect_vars_functs_preds(ret);
    return ret;
}

void NDProof::collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t> > &vars_functs_preds) const {
    for (const auto &ant : this->thesis.first) {
        ant->collect_vars_functs_preds(vars_functs_preds);
    }
    this->thesis.second->collect_vars_functs_preds(vars_functs_preds);
    for (const auto &subproof : this->get_subproofs()) {
        subproof->collect_vars_functs_preds(vars_functs_preds);
    }
}

const ndsequent &NDProof::get_thesis() const {
    return this->thesis;
}

NDProof::NDProof(const ndsequent &thesis) : thesis(thesis) {}

bool LogicalAxiom::check() const {
    using namespace gio::mmpp::provers::fof;
    const auto &th = this->get_thesis();
    if (th.first.size() != 1) return false;
    if (!gio::eq_cmp(fof_cmp())(*th.first[0], *this->form)) return false;
    if (!gio::eq_cmp(fof_cmp())(*th.second, *this->form)) return false;
    return true;
}

std::vector<std::shared_ptr<const NDProof> > LogicalAxiom::get_subproofs() const {
    return {};
}

const formula &LogicalAxiom::get_formula() const {
    return this->form;
}

LogicalAxiom::LogicalAxiom(const ndsequent &thesis, const formula &form)
    : NDProof(thesis), form(form) {}

bool WeakeningRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > WeakeningRule::get_subproofs() const {
    return {this->subproof};
}

const formula &WeakeningRule::get_new_conjunct() const {
    return this->form;
}

WeakeningRule::WeakeningRule(const ndsequent &thesis, const formula &form, const proof &subproof)
    : NDProof(thesis), form(form), subproof(subproof) {}

bool ContractionRule::check() const {
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
    if (contr_idx1 == contr_idx2) return false;
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

std::vector<std::shared_ptr<const NDProof> > ContractionRule::get_subproofs() const {
    return {this->subproof};
}

size_t ContractionRule::get_contr_idx1() const {
    return safe_decode_idx(this->contr_idx1, false);
}

size_t ContractionRule::get_contr_idx2() const {
    return safe_decode_idx(this->contr_idx2, false);
}

ContractionRule::ContractionRule(const ndsequent &thesis, idx_t contr_idx1, idx_t contr_idx2, const proof &subproof)
    : NDProof(thesis), contr_idx1(contr_idx1), contr_idx2(contr_idx2), subproof(subproof) {}

bool AndIntroRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > AndIntroRule::get_subproofs() const {
    return {this->left_proof, this->right_proof};
}

AndIntroRule::AndIntroRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof)
    : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

bool AndElim1Rule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > AndElim1Rule::get_subproofs() const {
    return {this->subproof};
}

const formula &AndElim1Rule::get_conjunct() const {
    using namespace gio::mmpp::provers::fof;
    return this->subproof->get_thesis().second->safe_mapped_dynamic_cast<const And>()->get_right();
}

AndElim1Rule::AndElim1Rule(const ndsequent &thesis, const proof &subproof)
    : NDProof(thesis), subproof(subproof) {}

bool AndElim2Rule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > AndElim2Rule::get_subproofs() const {
    return {this->subproof};
}

const formula &AndElim2Rule::get_conjunct() const {
    using namespace gio::mmpp::provers::fof;
    return this->subproof->get_thesis().second->safe_mapped_dynamic_cast<const And>()->get_left();
}

AndElim2Rule::AndElim2Rule(const ndsequent &thesis, const proof &subproof)
    : NDProof(thesis), subproof(subproof) {}

bool OrIntro1Rule::check() const {
    using namespace gio::mmpp::provers::fof;
    if (!this->subproof->check()) return false;
    const auto &th = this->get_thesis();
    const auto &sub_th = this->subproof->get_thesis();
    if (!gio::eq_cmp(fof_cmp())(*std::static_pointer_cast<const FOF>(Or::create(sub_th.second, this->disjunct)), *th.second)) return false;
    if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                       th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
    return true;
}

std::vector<std::shared_ptr<const NDProof> > OrIntro1Rule::get_subproofs() const {
    return {this->subproof};
}

const formula &OrIntro1Rule::get_disjunct() const {
    return this->disjunct;
}

OrIntro1Rule::OrIntro1Rule(const ndsequent &thesis, const formula &disjunct, const proof &subproof)
    : NDProof(thesis), disjunct(disjunct), subproof(subproof) {}

bool OrIntro2Rule::check() const {
    using namespace gio::mmpp::provers::fof;
    if (!this->subproof->check()) return false;
    const auto &th = this->get_thesis();
    const auto &sub_th = this->subproof->get_thesis();
    if (!gio::eq_cmp(fof_cmp())(*std::static_pointer_cast<const FOF>(Or::create(this->disjunct, sub_th.second)), *th.second)) return false;
    if (!gio::is_equal(sub_th.first.begin(), sub_th.first.end(),
                       th.first.begin(), th.first.end(), gio::eq_cmp(gio::star_cmp(fof_cmp())))) return false;
    return true;
}

std::vector<std::shared_ptr<const NDProof> > OrIntro2Rule::get_subproofs() const {
    return {this->subproof};
}

const formula &OrIntro2Rule::get_disjunct() const {
    return this->disjunct;
}

OrIntro2Rule::OrIntro2Rule(const ndsequent &thesis, const formula &disjunct, const proof &subproof)
    : NDProof(thesis), disjunct(disjunct), subproof(subproof) {}

bool OrElimRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > OrElimRule::get_subproofs() const {
    return {this->left_proof, this->middle_proof, this->right_proof};
}

size_t OrElimRule::get_middle_idx() const {
    return safe_decode_idx(this->middle_idx, false);
}

size_t OrElimRule::get_right_idx() const {
    return safe_decode_idx(this->right_idx, false);
}

OrElimRule::OrElimRule(const ndsequent &thesis, idx_t middle_idx, idx_t right_idx, const proof &left_proof, const proof &middle_proof, const proof &right_proof)
    : NDProof(thesis), middle_idx(middle_idx), right_idx(right_idx), left_proof(left_proof), middle_proof(middle_proof), right_proof(right_proof) {}

bool NegElimRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > NegElimRule::get_subproofs() const {
    return {this->left_proof, this->right_proof};
}

NegElimRule::NegElimRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof)
    : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

bool ImpIntroRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > ImpIntroRule::get_subproofs() const {
    return {this->subproof};
}

size_t ImpIntroRule::get_ant_idx() const {
    return safe_decode_idx(this->ant_idx, false);
}

ImpIntroRule::ImpIntroRule(const ndsequent &thesis, idx_t ant_idx, const proof &subproof)
    : NDProof(thesis), ant_idx(ant_idx), subproof(subproof) {}

bool ImpElimRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > ImpElimRule::get_subproofs() const {
    return {this->left_proof, this->right_proof};
}

ImpElimRule::ImpElimRule(const ndsequent &thesis, const proof &left_proof, const proof &right_proof)
    : NDProof(thesis), left_proof(left_proof), right_proof(right_proof) {}

bool BottomElimRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > BottomElimRule::get_subproofs() const {
    return {this->subproof};
}

const formula &BottomElimRule::get_succedent() const {
    return this->form;
}

BottomElimRule::BottomElimRule(const ndsequent &thesis, const formula &form, const proof &subproof)
    : NDProof(thesis), form(form), subproof(subproof) {}

bool ForallIntroRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > ForallIntroRule::get_subproofs() const {
    return {this->subproof};
}

ForallIntroRule::ForallIntroRule(const ndsequent &thesis, const std::shared_ptr<const fof::Variable> &var, const std::shared_ptr<const fof::Variable> &eigenvar, const proof &subproof)
    : NDProof(thesis), var(var), eigenvar(eigenvar), subproof(subproof) {}

bool ForallElimRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > ForallElimRule::get_subproofs() const {
    return {this->subproof};
}

ForallElimRule::ForallElimRule(const ndsequent &thesis, const term &subst_term, const proof &subproof)
    : NDProof(thesis), subst_term(subst_term), subproof(subproof) {}

bool ExistsIntroRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > ExistsIntroRule::get_subproofs() const {
    return {this->subproof};
}

ExistsIntroRule::ExistsIntroRule(const ndsequent &thesis, const formula &form, const std::shared_ptr<const fof::Variable> &var, const term &subst_term, const proof &subproof)
    : NDProof(thesis), form(form), var(var), subst_term(subst_term), subproof(subproof) {}

bool ExistsElimRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > ExistsElimRule::get_subproofs() const {
    return {this->left_proof, this->right_proof};
}

ExistsElimRule::ExistsElimRule(const ndsequent &thesis, idx_t idx, const std::shared_ptr<const fof::Variable> &eigenvar, const proof &left_proof, const proof &right_proof)
    : NDProof(thesis), idx(idx), eigenvar(eigenvar), left_proof(left_proof), right_proof(right_proof) {}

bool EqualityIntroRule::check() const {
    using namespace gio::mmpp::provers::fof;
    const auto &th = this->get_thesis();
    if (!th.first.empty()) return false;
    if (!gio::eq_cmp(fof_cmp())(*th.second, *std::static_pointer_cast<const FOF>(Equal::create(this->t, this->t)))) return false;
    return true;
}

std::vector<std::shared_ptr<const NDProof> > EqualityIntroRule::get_subproofs() const {
    return {};
}

EqualityIntroRule::EqualityIntroRule(const ndsequent &thesis, const term &t)
    : NDProof(thesis), t(t) {}

bool EqualityElimRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > EqualityElimRule::get_subproofs() const {
    return {this->left_proof, this->right_proof};
}

EqualityElimRule::EqualityElimRule(const ndsequent &thesis, const std::shared_ptr<const fof::Variable> &var, const formula &form, const proof &left_proof, const proof &right_proof)
    : NDProof(thesis), var(var), form(form), left_proof(left_proof), right_proof(right_proof) {}

bool ExcludedMiddleRule::check() const {
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

std::vector<std::shared_ptr<const NDProof> > ExcludedMiddleRule::get_subproofs() const {
    return {this->left_proof, this->right_proof};
}

size_t ExcludedMiddleRule::get_left_idx() const {
    return safe_decode_idx(this->left_idx, false);
}

size_t ExcludedMiddleRule::get_right_idx() const {
    return safe_decode_idx(this->right_idx, false);
}

ExcludedMiddleRule::ExcludedMiddleRule(const ndsequent &thesis, idx_t left_idx, idx_t right_idx, const proof &left_proof, const proof &right_proof)
    : NDProof(thesis), left_idx(left_idx), right_idx(right_idx), left_proof(left_proof), right_proof(right_proof) {}

}
