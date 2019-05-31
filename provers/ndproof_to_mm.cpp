
#include "ndproof_to_mm.h"

#include <boost/range/join.hpp>

namespace gio::mmpp::provers::ndproof {

nd_proof_to_mm_ctx::nd_proof_to_mm_ctx(const LibraryToolbox &tb, const fof::fof_to_mm_ctx &ctx) : tb(tb), ctx(ctx) {}

template<typename It>
Prover<CheckpointedProofEngine> build_conjuction_prover(const LibraryToolbox &tb, const gio::mmpp::provers::fof::fof_to_mm_ctx &ctx, const It &begin, const It &end) {
    using namespace gio::mmpp::setmm;
    if (begin == end) {
        return build_true_prover(tb);
    } else {
        auto begin2 = begin;
        begin2++;
        return build_and_prover(tb, ctx.convert_prover(*begin), build_conjuction_prover(tb, ctx, begin2, end));
    }
}

template<typename Cont>
Prover<CheckpointedProofEngine> build_conjuction_prover(const LibraryToolbox &tb, const gio::mmpp::provers::fof::fof_to_mm_ctx &ctx, const Cont &cont) {
    return build_conjuction_prover(tb, ctx, cont.begin(), cont.end());
}

const RegisteredProver not_free_true_rp = LibraryToolbox::register_prover({}, "|- F/ x T.");
const RegisteredProver not_free_and_rp = LibraryToolbox::register_prover({"|- F/ x ph", "|- F/ x ps"}, "|- F/ x ( ph /\\ ps )");

template<typename It>
Prover<CheckpointedProofEngine> build_conjunction_not_free_prover(const LibraryToolbox &tb, const fof::fof_to_mm_ctx &ctx, const It &begin, const It &end, const std::string &var_name) {
    using namespace gio::mmpp::provers::fof;
    if (begin == end) {
        return tb.build_registered_prover(not_free_true_rp, {{"x", ctx.convert_prover(Variable::create(var_name), false)}}, {});
    } else {
        auto begin2 = begin;
        begin2++;
        return tb.build_registered_prover(not_free_and_rp, {{"x", ctx.convert_prover(Variable::create(var_name), false)},
                                                            {"ph", ctx.convert_prover(*begin)},
                                                            {"ps", build_conjuction_prover(tb, ctx, begin2, end)}},
                                          {ctx.not_free_prover(*begin, var_name), build_conjunction_not_free_prover(tb, ctx, begin2, end, var_name)});
    }
}

template<typename Cont>
Prover<CheckpointedProofEngine> build_conjunction_not_free_prover(const LibraryToolbox &tb, const fof::fof_to_mm_ctx &ctx, const Cont &cont, const std::string &var_name) {
    return build_conjunction_not_free_prover(tb, ctx, cont.begin(), cont.end(), var_name);
}

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::convert_ndsequent_ant_prover(const ndsequent &seq) const {
    return build_conjuction_prover(this->tb, this->ctx, seq.first);
}

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::convert_ndsequent_suc_prover(const ndsequent &seq) const {
    using namespace gio::mmpp::setmm;
    return this->ctx.convert_prover(seq.second);
}

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::convert_ndsequent_prover(const ndsequent &seq) const {
    using namespace gio::mmpp::setmm;
    return build_implies_prover(this->tb, this->convert_ndsequent_ant_prover(seq), this->convert_ndsequent_suc_prover(seq));
}

ParsingTree<SymTok, LabTok> nd_proof_to_mm_ctx::convert_ndsequent(const ndsequent &seq) const {
    return pt2_to_pt(prover_to_pt2(this->tb, this->convert_ndsequent_prover(seq)));
}

const RegisteredProver logical_axiom_rp = LibraryToolbox::register_prover({}, "|- ( ( ph /\\ T. ) -> ph )");
const RegisteredProver weakening_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )"}, "|- ( ( ch /\\ ph ) -> ps )");
const RegisteredProver contraction_rule_rp = LibraryToolbox::register_prover({"|- ( ( ph /\\ ( ph /\\ ps ) ) -> ch )"}, "|- ( ( ph /\\ ps ) -> ch )");
const RegisteredProver and_intro_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> ch )", "|- ( ps -> et )"}, "|- ( ( ph /\\ ps ) -> ( ch /\\ et ) )");
const RegisteredProver and_elim1_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps /\\ ch ) )"}, "|- ( ph -> ps )");
const RegisteredProver and_elim2_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ch /\\ ps ) )"}, "|- ( ph -> ps )");
const RegisteredProver or_intro1_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )"}, "|- ( ph -> ( ps \\/ ch ) )");
const RegisteredProver or_intro2_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )"}, "|- ( ph -> ( ch \\/ ps ) )");
const RegisteredProver or_elim_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps \\/ ch ) )", "|- ( ( ps /\\ et ) -> th )", "|- ( ( ch /\\ rh ) -> th )"}, "|- ( ( ph /\\ ( et /\\ rh ) ) -> th )");
const RegisteredProver neg_elim_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> -. ch )", "|- ( ps -> ch )"}, "|- ( ( ph /\\ ps ) -> F. )");
const RegisteredProver imp_intro_rule_rp = LibraryToolbox::register_prover({"|- ( ( ph /\\ ps ) -> ch )"}, "|- ( ps -> ( ph -> ch ) )");
const RegisteredProver imp_elim_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps -> ch ) )", "|- ( et -> ps )"}, "|- ( ( ph /\\ et ) -> ch )");
const RegisteredProver bottom_elim_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> F. )"}, "|- ( ph -> ps )");
const RegisteredProver forall_intro_rp = LibraryToolbox::register_prover({"|- F/ y ph", "|- F/ y ch", "|- ( [. y / x ]. ch <-> ps )", "|- ( ph -> ps )"}, "|- ( ph -> A. x ch )");
const RegisteredProver forall_elim_rp = LibraryToolbox::register_prover({"|- A e. _V", "|- ( ph -> A. x ch )", "|- ( [. A / x ]. ch <-> ps )"}, "|- ( ph -> ps )");
const RegisteredProver exists_intro_rp = LibraryToolbox::register_prover({"|- ( [. A / x ]. ch <-> ps )", "|- ( ph -> ps )"}, "|- ( ph -> E. x ch )");
const RegisteredProver exists_elim_rp = LibraryToolbox::register_prover({"|- ( ph -> E. x th )", "|- ( [. y / x ]. th <-> et )", "|- ( ( et /\\ ps ) -> ch )", "|- F/ y ch", "|- F/ y th", "|- F/ y ps"}, "|- ( ( ph /\\ ps ) -> ch )");
const RegisteredProver equality_intro_rule_rp = LibraryToolbox::register_prover({}, "|- A = A");
const RegisteredProver equality_elim_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> A = B )", "|- ( ps -> th )", "|- ( [. A / x ]. ch <-> th )", "|- ( [. B / x ]. ch <-> et )"}, "|- ( ( ph /\\ ps ) -> et )");
const RegisteredProver equality_elim_rule2_rp = LibraryToolbox::register_prover({"|- ( ph -> A = B )", "|- ( ps -> et )", "|- ( [. A / x ]. ch <-> th )", "|- ( [. B / x ]. ch <-> et )"}, "|- ( ( ph /\\ ps ) -> th )");
const RegisteredProver excluded_middle_rule_rp = LibraryToolbox::register_prover({"|- ( ( ps /\\ ph ) -> ch )", "|- ( ( -. ps /\\ et ) -> ch )"}, "|- ( ( ph /\\ et ) -> ch )");

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::convert_proof(const proof &pr) const {
    if (const auto la = pr->mapped_dynamic_cast<const LogicalAxiom>()) {
        return this->tb.build_registered_prover(logical_axiom_rp, {{"ph", this->ctx.convert_prover(la->get_formula())}}, {});
    } else if (const auto wr = pr->mapped_dynamic_cast<const WeakeningRule>()) {
        const auto subproof = wr->get_subproofs().at(0);
        auto sub_prover = this->convert_proof(subproof);
        return this->tb.build_registered_prover(weakening_rule_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, subproof->get_thesis().first)},
                                                                    {"ps", this->ctx.convert_prover(wr->get_thesis().second)},
                                                                    {"ch", this->ctx.convert_prover(wr->get_new_conjunct())}}, {sub_prover});
    } else if (const auto cr = pr->mapped_dynamic_cast<const ContractionRule>()) {
        const auto subproof = cr->get_subproofs().at(0);
        auto sub_prover = this->convert_proof(subproof);
        sub_prover = this->select_antecedents(subproof->get_thesis(), sub_prover, cr->get_contr_idx1(), cr->get_contr_idx2());
        std::vector<formula> ants(gio::skipping_iterator(subproof->get_thesis().first.begin(), subproof->get_thesis().first.end(), {cr->get_contr_idx1(), cr->get_contr_idx2()}),
                                  gio::skipping_iterator(subproof->get_thesis().first.end(), subproof->get_thesis().first.end(), {}));
        return this->tb.build_registered_prover(contraction_rule_rp, {{"ph", this->ctx.convert_prover(subproof->get_thesis().first.at(cr->get_contr_idx1()))},
                                                                      {"ps", build_conjuction_prover(this->tb, this->ctx, ants)},
                                                                      {"ch", this->ctx.convert_prover(cr->get_thesis().second)}}, {sub_prover});
    } else if (const auto air = pr->mapped_dynamic_cast<const AndIntroRule>()) {
        auto left_proof = air->get_subproofs().at(0);
        auto left_prover = this->convert_proof(left_proof);
        auto right_proof = air->get_subproofs().at(1);
        auto right_prover = this->convert_proof(right_proof);
        auto prover = this->tb.build_registered_prover(and_intro_rule_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, left_proof->get_thesis().first)},
                                                                           {"ps", build_conjuction_prover(this->tb, this->ctx, right_proof->get_thesis().first)},
                                                                           {"ch", this->ctx.convert_prover(left_proof->get_thesis().second)},
                                                                           {"et", this->ctx.convert_prover(right_proof->get_thesis().second)}}, {left_prover, right_prover});
        prover = this->merge_antecedents(left_proof->get_thesis().first, right_proof->get_thesis().first, air->get_thesis().second, prover);
        return prover;
    } else if (const auto ae1r = pr->mapped_dynamic_cast<const AndElim1Rule>()) {
        const auto subproof = ae1r->get_subproofs().at(0);
        auto sub_prover = this->convert_proof(subproof);
        return tb.build_registered_prover(and_elim1_rule_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, ae1r->get_thesis().first)},
                                                              {"ps", this->ctx.convert_prover(ae1r->get_thesis().second)},
                                                              {"ch", this->ctx.convert_prover(ae1r->get_conjunct())}}, {sub_prover});
    } else if (const auto ae2r = pr->mapped_dynamic_cast<const AndElim2Rule>()) {
        const auto subproof = ae2r->get_subproofs().at(0);
        auto sub_prover = this->convert_proof(subproof);
        return tb.build_registered_prover(and_elim2_rule_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, ae2r->get_thesis().first)},
                                                              {"ps", this->ctx.convert_prover(ae2r->get_thesis().second)},
                                                              {"ch", this->ctx.convert_prover(ae2r->get_conjunct())}}, {sub_prover});
    } else if (const auto oi1r = pr->mapped_dynamic_cast<const OrIntro1Rule>()) {
        const auto subproof = oi1r->get_subproofs().at(0);
        auto sub_prover = this->convert_proof(subproof);
        return this->tb.build_registered_prover(or_intro1_rule_rp, {{"ph", this->convert_ndsequent_ant_prover(subproof->get_thesis())},
                                                                    {"ps", this->convert_ndsequent_suc_prover(subproof->get_thesis())},
                                                                    {"ch", this->ctx.convert_prover(oi1r->get_disjunct())}}, {sub_prover});
    } else if (const auto oi2r = pr->mapped_dynamic_cast<const OrIntro2Rule>()) {
        const auto subproof = oi2r->get_subproofs().at(0);
        auto sub_prover = this->convert_proof(subproof);
        return this->tb.build_registered_prover(or_intro2_rule_rp, {{"ph", this->convert_ndsequent_ant_prover(subproof->get_thesis())},
                                                                    {"ps", this->convert_ndsequent_suc_prover(subproof->get_thesis())},
                                                                    {"ch", this->ctx.convert_prover(oi2r->get_disjunct())}}, {sub_prover});
    } else if (const auto oer = pr->mapped_dynamic_cast<const OrElimRule>()) {
        auto left_proof = oer->get_subproofs().at(0);
        auto left_prover = this->convert_proof(left_proof);
        auto middle_proof = oer->get_subproofs().at(1);
        auto middle_prover = this->convert_proof(middle_proof);
        middle_prover = this->select_antecedent(middle_proof->get_thesis(), middle_prover, oer->get_middle_idx());
        auto right_proof = oer->get_subproofs().at(2);
        auto right_prover = this->convert_proof(right_proof);
        right_prover = this->select_antecedent(right_proof->get_thesis(), right_prover, oer->get_right_idx());
        std::vector<formula> middle_ants(gio::skipping_iterator(middle_proof->get_thesis().first.begin(), middle_proof->get_thesis().first.end(), {oer->get_middle_idx()}),
                                         gio::skipping_iterator(middle_proof->get_thesis().first.end(), middle_proof->get_thesis().first.end(), {}));
        std::vector<formula> right_ants(gio::skipping_iterator(right_proof->get_thesis().first.begin(), right_proof->get_thesis().first.end(), {oer->get_right_idx()}),
                                        gio::skipping_iterator(right_proof->get_thesis().first.end(), right_proof->get_thesis().first.end(), {}));
        auto prover = this->tb.build_registered_prover(or_elim_rule_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, left_proof->get_thesis().first)},
                                                                         {"et", build_conjuction_prover(this->tb, this->ctx, middle_ants)},
                                                                         {"rh", build_conjuction_prover(this->tb, this->ctx, right_ants)},
                                                                         {"ps", this->ctx.convert_prover(middle_proof->get_thesis().first.at(oer->get_middle_idx()))},
                                                                         {"ch", this->ctx.convert_prover(right_proof->get_thesis().first.at(oer->get_right_idx()))},
                                                                         {"th", this->ctx.convert_prover(oer->get_thesis().second)}}, {left_prover, middle_prover, right_prover});
        prover = this->merge_antecedents(left_proof->get_thesis().first, middle_ants, right_ants, oer->get_thesis().second, prover);
        return prover;
    } else if (const auto ner = pr->mapped_dynamic_cast<const NegElimRule>()) {
        auto left_proof = ner->get_subproofs().at(0);
        auto left_prover = this->convert_proof(left_proof);
        auto right_proof = ner->get_subproofs().at(1);
        auto right_prover = this->convert_proof(right_proof);
        auto prover = this->tb.build_registered_prover(neg_elim_rule_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, left_proof->get_thesis().first)},
                                                                          {"ps", build_conjuction_prover(this->tb, this->ctx, right_proof->get_thesis().first)},
                                                                          {"ch", this->ctx.convert_prover(right_proof->get_thesis().second)}}, {left_prover, right_prover});
        prover = this->merge_antecedents(left_proof->get_thesis().first, right_proof->get_thesis().first, gio::mmpp::provers::fof::False::create(), prover);
        return prover;
    } else if (const auto iir = pr->mapped_dynamic_cast<const ImpIntroRule>()) {
        const auto subproof = iir->get_subproofs().at(0);
        auto sub_prover = this->convert_proof(subproof);
        sub_prover = this->select_antecedent(subproof->get_thesis(), sub_prover, iir->get_ant_idx());
        std::vector<formula> ants(gio::skipping_iterator(subproof->get_thesis().first.begin(), subproof->get_thesis().first.end(), {iir->get_ant_idx()}),
                                  gio::skipping_iterator(subproof->get_thesis().first.end(), subproof->get_thesis().first.end(), {}));
        return this->tb.build_registered_prover(imp_intro_rule_rp, {{"ph", this->ctx.convert_prover(subproof->get_thesis().first.at(iir->get_ant_idx()))},
                                                                    {"ps", build_conjuction_prover(this->tb, this->ctx, ants)},
                                                                    {"ch", this->ctx.convert_prover(subproof->get_thesis().second)}}, {sub_prover});
    } else if (const auto ier = pr->mapped_dynamic_cast<const ImpElimRule>()) {
        auto left_proof = ier->get_subproofs().at(0);
        auto left_prover = this->convert_proof(left_proof);
        auto right_proof = ier->get_subproofs().at(1);
        auto right_prover = this->convert_proof(right_proof);
        auto prover = this->tb.build_registered_prover(imp_elim_rule_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, left_proof->get_thesis().first)},
                                                                          {"et", build_conjuction_prover(this->tb, this->ctx, right_proof->get_thesis().first)},
                                                                          {"ps", this->ctx.convert_prover(right_proof->get_thesis().second)},
                                                                          {"ch", this->ctx.convert_prover(ier->get_thesis().second)}}, {left_prover, right_prover});
        prover = this->merge_antecedents(left_proof->get_thesis().first, right_proof->get_thesis().first, ier->get_thesis().second, prover);
        return prover;
    } else if (const auto ber = pr->mapped_dynamic_cast<const BottomElimRule>()) {
        const auto subproof = ber->get_subproofs().at(0);
        auto sub_prover = this->convert_proof(subproof);
        return tb.build_registered_prover(bottom_elim_rule_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, ber->get_thesis().first)},
                                                                {"ps", this->ctx.convert_prover(ber->get_thesis().second)}}, {sub_prover});
    } else if (const auto fir = pr->mapped_dynamic_cast<const ForallIntroRule>()) {
        auto subproof = fir->get_subproofs().at(0);
        auto sub_prover = this->convert_proof(subproof);
        auto prover = this->tb.build_registered_prover(forall_intro_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, fir->get_thesis().first)},
                                                                         {"ch", this->ctx.convert_prover(fir->get_predicate())},
                                                                         {"ps", this->ctx.convert_prover(fir->get_predicate()->replace(fir->get_var()->get_name(), fir->get_eigenvar()))}},
                                                       {build_conjunction_not_free_prover(this->tb, this->ctx, fir->get_thesis().first, fir->get_eigenvar()->get_name()),
                                                        this->ctx.not_free_prover(fir->get_predicate(), fir->get_eigenvar()->get_name()),
                                                        this->ctx.replace_prover(fir->get_predicate(), fir->get_var()->get_name(), fir->get_eigenvar()),
                                                        sub_prover});
        return prover;
    } else if (const auto fer = pr->mapped_dynamic_cast<const ForallElimRule>()) {
        auto subproof = fer->get_subproofs().at(0);
        auto sub_prover = this->convert_proof(subproof);
        auto prover = this->tb.build_registered_prover(forall_elim_rp, {{"A", this->ctx.convert_prover(fer->get_subst_term(), true)},
                                                                        {"x", this->ctx.convert_prover(fer->get_var(), false)},
                                                                        {"ph", build_conjuction_prover(this->tb, this->ctx, fer->get_thesis().first)},
                                                                        {"ch", this->ctx.convert_prover(fer->get_predicate())},
                                                                        {"ps", this->ctx.convert_prover(fer->get_predicate()->replace(fer->get_var()->get_name(), fer->get_subst_term()))}},
                                                       {this->ctx.sethood_prover(fer->get_subst_term()), sub_prover, this->ctx.replace_prover(fer->get_predicate(), fer->get_var()->get_name(), fer->get_subst_term())});
        return prover;
    } else if (const auto eir = pr->mapped_dynamic_cast<const ExistsIntroRule>()) {
        auto subproof = eir->get_subproofs().at(0);
        auto sub_prover = this->convert_proof(subproof);
        auto prover = this->tb.build_registered_prover(exists_intro_rp, {{"A", this->ctx.convert_prover(eir->get_subst_term(), true)},
                                                                         {"x", this->ctx.convert_prover(eir->get_var(), false)},
                                                                         {"ph", build_conjuction_prover(this->tb, this->ctx, eir->get_thesis().first)},
                                                                         {"ch", this->ctx.convert_prover(eir->get_predicate())},
                                                                         {"ps", this->ctx.convert_prover(eir->get_predicate()->replace(eir->get_var()->get_name(), eir->get_subst_term()))}},
                                {this->ctx.replace_prover(eir->get_predicate(), eir->get_var()->get_name(), eir->get_subst_term()), sub_prover});
        return prover;
    } else if (const auto eer = pr->mapped_dynamic_cast<const ExistsElimRule>()) {
        auto left_proof = eer->get_subproofs().at(0);
        auto left_prover = this->convert_proof(left_proof);
        auto right_proof = eer->get_subproofs().at(1);
        auto right_prover = this->convert_proof(right_proof);
        right_prover = this->select_antecedent(right_proof->get_thesis(), right_prover, eer->get_right_idx());
        std::vector<formula> right_ants(gio::skipping_iterator(right_proof->get_thesis().first.begin(), right_proof->get_thesis().first.end(), {eer->get_right_idx()}),
                                        gio::skipping_iterator(right_proof->get_thesis().first.end(), right_proof->get_thesis().first.end(), {}));
        auto prover = this->tb.build_registered_prover(exists_elim_rp, {{"x", this->ctx.convert_prover(eer->get_var(), false)},
                                                                        {"y", this->ctx.convert_prover(eer->get_eigenvar(), false)},
                                                                        {"ph", build_conjuction_prover(this->tb, this->ctx, left_proof->get_thesis().first)},
                                                                        {"ps", build_conjuction_prover(this->tb, this->ctx, right_ants)},
                                                                        {"th", this->ctx.convert_prover(eer->get_predicate())},
                                                                        {"et", this->ctx.convert_prover(eer->get_predicate()->replace(eer->get_var()->get_name(), eer->get_eigenvar()))},
                                                                        {"ch", this->ctx.convert_prover(eer->get_thesis().second)}},
                                                       {left_prover, this->ctx.replace_prover(eer->get_predicate(), eer->get_var()->get_name(), eer->get_eigenvar()), right_prover,
                                                        this->ctx.not_free_prover(eer->get_thesis().second, eer->get_eigenvar()->get_name()),
                                                        this->ctx.not_free_prover(eer->get_predicate(), eer->get_eigenvar()->get_name()),
                                                        build_conjunction_not_free_prover(this->tb, this->ctx, right_ants, eer->get_eigenvar()->get_name())});
        prover = this->merge_antecedents(left_proof->get_thesis().first, right_ants, eer->get_thesis().second, prover);
        return prover;
    } else if (const auto eir = pr->mapped_dynamic_cast<const EqualityIntroRule>()) {
        auto prover = this->tb.build_registered_prover(equality_intro_rule_rp, {{"A", this->ctx.convert_prover(eir->get_term())}}, {});
        return prover;
    } else if (const auto eer = pr->mapped_dynamic_cast<const EqualityElimRule>()) {
        auto left_proof = eer->get_subproofs().at(0);
        auto left_prover = this->convert_proof(left_proof);
        auto right_proof = eer->get_subproofs().at(1);
        auto right_prover = this->convert_proof(right_proof);
        auto reg_prover = eer->is_reversed() ? equality_elim_rule2_rp : equality_elim_rule_rp;
        auto prover = this->tb.build_registered_prover(reg_prover, {{"ph", build_conjuction_prover(this->tb, this->ctx, left_proof->get_thesis().first)},
                                                                    {"ps", build_conjuction_prover(this->tb, this->ctx, right_proof->get_thesis().first)},
                                                                    {"x", this->ctx.convert_prover(eer->get_var(), false)},
                                                                    {"A", this->ctx.convert_prover(eer->get_left_term(), true)},
                                                                    {"B", this->ctx.convert_prover(eer->get_right_term(), true)},
                                                                    {"ch", this->ctx.convert_prover(eer->get_subst_formula())},
                                                                    {"th", this->ctx.convert_prover(eer->get_subst_formula()->replace(eer->get_var()->get_name(), eer->get_left_term()))},
                                                                    {"et", this->ctx.convert_prover(eer->get_subst_formula()->replace(eer->get_var()->get_name(), eer->get_right_term()))},},
                                                       {left_prover, right_prover, this->ctx.replace_prover(eer->get_subst_formula(), eer->get_var()->get_name(), eer->get_left_term()),
                                                        this->ctx.replace_prover(eer->get_subst_formula(), eer->get_var()->get_name(), eer->get_right_term())});
        prover = this->merge_antecedents(left_proof->get_thesis().first, right_proof->get_thesis().first, eer->get_thesis().second, prover);
        return prover;
    } else if (const auto emr = pr->mapped_dynamic_cast<const ExcludedMiddleRule>()) {
        auto left_proof = emr->get_subproofs().at(0);
        auto left_prover = this->convert_proof(left_proof);
        left_prover = this->select_antecedent(left_proof->get_thesis(), left_prover, emr->get_left_idx());
        auto right_proof = emr->get_subproofs().at(1);
        auto right_prover = this->convert_proof(right_proof);
        right_prover = this->select_antecedent(right_proof->get_thesis(), right_prover, emr->get_right_idx());
        std::vector<formula> left_ants(gio::skipping_iterator(left_proof->get_thesis().first.begin(), left_proof->get_thesis().first.end(), {emr->get_left_idx()}),
                                       gio::skipping_iterator(left_proof->get_thesis().first.end(), left_proof->get_thesis().first.end(), {}));
        std::vector<formula> right_ants(gio::skipping_iterator(right_proof->get_thesis().first.begin(), right_proof->get_thesis().first.end(), {emr->get_right_idx()}),
                                        gio::skipping_iterator(right_proof->get_thesis().first.end(), right_proof->get_thesis().first.end(), {}));
        auto prover = this->tb.build_registered_prover(excluded_middle_rule_rp, {{"ps", this->ctx.convert_prover(left_proof->get_thesis().first.at(emr->get_left_idx()))},
                                                                                 {"ph", build_conjuction_prover(this->tb, this->ctx, left_ants)},
                                                                                 {"et", build_conjuction_prover(this->tb, this->ctx, right_ants)},
                                                                                 {"ch", this->ctx.convert_prover(emr->get_thesis().second)}}, {left_prover, right_prover});
        prover = this->merge_antecedents(left_ants, right_ants, emr->get_thesis().second, prover);
        return prover;
    } else {
        throw std::runtime_error(gio_make_string("invalid proof type " << boost::typeindex::type_id_runtime(*pr).pretty_name()));
    }
}

const RegisteredProver equality_elim_rule_rp2 = LibraryToolbox::register_prover({"|- ( ph -> A = B )", "|- ( ps -> th )", "|- ( [. A / x ]. ch <-> th )", "|- ( [. B / x ]. ch <-> et )"}, "|- ( ( ph /\\ ps ) -> et )");
const RegisteredProver equality_elim_rule2_rp2 = LibraryToolbox::register_prover({"|- ( ph -> A = B )", "|- ( ps -> et )", "|- ( [. A / x ]. ch <-> th )", "|- ( [. B / x ]. ch <-> et )"}, "|- ( ( ph /\\ ps ) -> th )");

const RegisteredProver select_ant_init_rp = LibraryToolbox::register_prover({}, "|- ( ph <-> ph )");
const RegisteredProver select_ant_rp = LibraryToolbox::register_prover({"|- ( ph <-> ( ps /\\ ch ) )"}, "|- ( ( et /\\ ph ) <-> ( ps /\\ ( et /\\ ch ) ) )");
const RegisteredProver select_ant_change_rp = LibraryToolbox::register_prover({"|- ( ph <-> ( ps /\\ et ) )", "|- ( ph -> ch )"}, "|- ( ( ps /\\ et ) -> ch )");

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::select_antecedent(const ndsequent &seq, const Prover<CheckpointedProofEngine> &pr, size_t idx) const {
    if (idx == 0) {
        return pr;
    }
    auto prover = this->tb.build_registered_prover(select_ant_init_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, seq.first.begin()+idx, seq.first.end())}}, {});
    for (size_t i = 0; i < idx; i++) {
        prover = this->tb.build_registered_prover(select_ant_rp, {{"ch", build_conjuction_prover(this->tb, this->ctx, boost::range::join(boost::iterator_range<decltype(seq.first.begin())>(seq.first.begin()+idx-i, seq.first.begin()+idx),
                                                                                                                                         boost::iterator_range<decltype(seq.first.begin())>(seq.first.begin()+idx+1, seq.first.end())))},
                                                                  {"ps", this->ctx.convert_prover(seq.first.at(idx))},
                                                                  {"ph", build_conjuction_prover(this->tb, this->ctx, seq.first.begin()+idx-i, seq.first.end())},
                                                                  {"et", this->ctx.convert_prover(seq.first.at(idx-i-1))}}, {prover});
    }
    return this->tb.build_registered_prover(select_ant_change_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, seq.first)},
                                                                   {"ch", this->ctx.convert_prover(seq.second)},
                                                                   {"ps", this->ctx.convert_prover(seq.first.at(idx))},
                                                                   {"et", build_conjuction_prover(this->tb, this->ctx, gio::skipping_iterator(seq.first.begin(), seq.first.end(), {idx}), gio::skipping_iterator(seq.first.end(), seq.first.end(), {}))}}, {prover, pr});
}

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::select_antecedents(const ndsequent &seq, const Prover<CheckpointedProofEngine> &pr, size_t idx1, size_t idx2) const {
    gio::assert_or_throw<std::invalid_argument>(idx1 != idx2, "cannot use two identical indices");
    auto prover = this->select_antecedent(seq, pr, idx2);
    ndsequent seq2;
    seq2.first.push_back(seq.first.at(idx2));
    seq2.first.insert(seq2.first.end(), gio::skipping_iterator(seq.first.begin(), seq.first.end(), {idx2}), gio::skipping_iterator(seq.first.end(), seq.first.end(), {}));
    seq2.second = seq.second;
    prover = this->select_antecedent(seq2, prover, idx2 < idx1 ? idx1 : idx1+1);
    return prover;
}

const RegisteredProver merge_ants_init_rp = LibraryToolbox::register_prover({}, "|- ( ph <-> ( T. /\\ ph ) )");
const RegisteredProver merge_ants_reinit_rp = LibraryToolbox::register_prover({"|- ( ph <-> ( ps /\\ ch ) )"}, "|- ( ph <-> ( T. /\\ ( ps /\\ ch ) ) )");
const RegisteredProver merge_ants_rp = LibraryToolbox::register_prover({"|- ( ph <-> ( ps /\\ ch ) )"}, "|- ( ( et /\\ ph ) <-> ( ( et /\\ ps ) /\\ ch ) )");
const RegisteredProver merge_ants2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ( ps /\\ ( ch /\\ th ) ) )"}, "|- ( ( et /\\ ph ) <-> ( ( et /\\ ps ) /\\ ( ch /\\ th ) ) )");
const RegisteredProver merge_ants_change_rp = LibraryToolbox::register_prover({"|- ( ps <-> ( ph /\\ th ) )", "|- ( ( ph /\\ th ) -> ch )"}, "|- ( ps -> ch )");
const RegisteredProver merge_ants2_change_rp = LibraryToolbox::register_prover({"|- ( ps <-> ( ph /\\ ( th /\\ et ) ) )", "|- ( ( ph /\\ ( th /\\ et ) ) -> ch )"}, "|- ( ps -> ch )");

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::merge_antecedents(const std::vector<formula> &ant1, const std::vector<formula> &ant2, const formula &suc, const Prover<CheckpointedProofEngine> &pr) const {
    auto right_prover = build_conjuction_prover(this->tb, this->ctx, ant2);
    auto prover = this->tb.build_registered_prover(merge_ants_init_rp, {{"ph", right_prover}}, {});
    for (size_t i = 0; i < ant1.size(); i++) {
        prover = tb.build_registered_prover(merge_ants_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, boost::range::join(boost::iterator_range<decltype(ant1.begin())>(ant1.begin()+ant1.size()-i, ant1.end()), ant2))},
                                                            {"ps", build_conjuction_prover(this->tb, this->ctx, ant1.begin()+ant1.size()-i, ant1.end())},
                                                            {"ch", right_prover},
                                                            {"et", this->ctx.convert_prover(ant1.at(ant1.size()-i-1))}}, {prover});
    }
    return tb.build_registered_prover(merge_ants_change_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, ant1)},
                                                             {"th", build_conjuction_prover(this->tb, this->ctx, ant2)},
                                                             {"ps", build_conjuction_prover(this->tb, this->ctx, boost::range::join(ant1, ant2))},
                                                             {"ch", this->ctx.convert_prover(suc)}}, {prover, pr});
}

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::merge_antecedents(const std::vector<formula> &ant1, const std::vector<formula> &ant2, const std::vector<formula> &ant3, const formula &suc, const Prover<CheckpointedProofEngine> &pr) const {
    auto middle_prover = build_conjuction_prover(this->tb, this->ctx, ant2);
    auto right_prover = build_conjuction_prover(this->tb, this->ctx, ant3);
    auto prover = this->tb.build_registered_prover(merge_ants_init_rp, {{"ph", right_prover}}, {});
    for (size_t i = 0; i < ant2.size(); i++) {
        prover = tb.build_registered_prover(merge_ants_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, boost::range::join(boost::iterator_range<decltype(ant2.begin())>(ant2.begin()+ant2.size()-i, ant2.end()), ant3))},
                                                            {"ps", build_conjuction_prover(this->tb, this->ctx, ant2.begin()+ant2.size()-i, ant2.end())},
                                                            {"ch", right_prover},
                                                            {"et", this->ctx.convert_prover(ant2.at(ant2.size()-i-1))}}, {prover});
    }
    prover = tb.build_registered_prover(merge_ants_reinit_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, boost::range::join(ant2, ant3))},
                                                               {"ps", middle_prover},
                                                               {"ch", right_prover}}, {prover});
    for (size_t i = 0; i < ant1.size(); i++) {
        prover = tb.build_registered_prover(merge_ants2_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, boost::range::join(boost::iterator_range<decltype(ant1.begin())>(ant1.begin()+ant1.size()-i, ant1.end()), boost::range::join(ant2, ant3)))},
                                                             {"ps", build_conjuction_prover(this->tb, this->ctx, ant1.begin()+ant1.size()-i, ant1.end())},
                                                             {"ch", middle_prover},
                                                             {"th", right_prover},
                                                             {"et", this->ctx.convert_prover(ant1.at(ant1.size()-i-1))}}, {prover});
    }
    return tb.build_registered_prover(merge_ants2_change_rp, {{"ph", build_conjuction_prover(this->tb, this->ctx, ant1)},
                                                              {"th", build_conjuction_prover(this->tb, this->ctx, ant2)},
                                                              {"et", build_conjuction_prover(this->tb, this->ctx, ant3)},
                                                              {"ps", build_conjuction_prover(this->tb, this->ctx, boost::range::join(ant1, boost::range::join(ant2, ant3)))},
                                                              {"ch", this->ctx.convert_prover(suc)}}, {prover, pr});
}

}
