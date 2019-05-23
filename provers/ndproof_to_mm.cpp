
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
const RegisteredProver imp_intro_rule_rp = LibraryToolbox::register_prover({"|- ( ( ph /\\ ps ) -> ch )"}, "|- ( ps -> ( ph -> ch ) )");
const RegisteredProver imp_elim_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps -> ch ) )", "|- ( et -> ps )"}, "|- ( ( ph /\\ et ) -> ch )");
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
    } else if (const auto emr = pr->mapped_dynamic_cast<const ExcludedMiddleRule>()) {
        auto left_proof = emr->get_subproofs().at(0);
        auto left_prover = this->convert_proof(left_proof);
        left_prover = this->select_antecedent(left_proof->get_thesis(), left_prover, emr->get_left_idx());
        auto right_proof = emr->get_subproofs().at(1);
        auto right_prover = this->convert_proof(right_proof);
        right_prover = this->select_antecedent(right_proof->get_thesis(), right_prover, emr->get_left_idx());
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
const RegisteredProver merge_ants_rp = LibraryToolbox::register_prover({"|- ( ph <-> ( ps /\\ ch ) )"}, "|- ( ( et /\\ ph ) <-> ( ( et /\\ ps ) /\\ ch ) )");
const RegisteredProver merge_ants_change_rp = LibraryToolbox::register_prover({"|- ( ps <-> ( ph /\\ th ) )", "|- ( ( ph /\\ th ) -> ch )"}, "|- ( ps -> ch )");

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

}
