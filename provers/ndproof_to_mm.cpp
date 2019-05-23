
#include "ndproof_to_mm.h"

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
    /*auto prover = build_true_prover(tb);
    for (const auto &ant : boost::adaptors::reverse(cont)) {
        prover = build_and_prover(tb, ctx.convert_prover(ant), prover);
    }
    return prover;*/
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
const RegisteredProver or_intro1_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )"}, "|- ( ph -> ( ps \\/ ch ) )");
const RegisteredProver or_intro2_rule_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )"}, "|- ( ph -> ( ch \\/ ps ) )");
const RegisteredProver excluded_middle_rule_rp = LibraryToolbox::register_prover({"|- ( ( ps /\\ ph ) -> ch )", "|- ( ( -. ps /\\ et ) -> ch )"}, "|- ( ( ph /\\ et ) -> ch )");

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::convert_proof(const proof &pr) const {
    if (const auto la = pr->mapped_dynamic_cast<const LogicalAxiom>()) {
        return this->tb.build_registered_prover(logical_axiom_rp, {{"ph", this->ctx.convert_prover(la->get_formula())}}, {});
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
        prover = this->merge_antecedents({left_ants, right_ants}, prover);
        return prover;
    } else {
        throw std::runtime_error(gio_make_string("invalid proof type " << boost::typeindex::type_id_runtime(*pr).pretty_name()));
    }
}

const RegisteredProver select_ant_init_rp = LibraryToolbox::register_prover({}, "|- ( ph <-> ph )");
const RegisteredProver select_ant_rp = LibraryToolbox::register_prover({"|- ( ph <-> ( ps /\\ ch ) )"}, "|- ( ( et /\\ ph ) <-> ( ps /\\ ( et /\\ ch ) ) )");
const RegisteredProver select_ant_finish_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ph -> ch )"}, "|- ( ps -> ch )");

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::select_antecedent(const ndsequent &seq, const Prover<CheckpointedProofEngine> &pr, size_t idx) const {
    if (idx == 0) {
        return pr;
    }
    return make_throwing_prover("select_antecent() not implemented");
}

const RegisteredProver merge_ants_init_rp = LibraryToolbox::register_prover({}, "|- ( ph <-> ( T. /\\ ph ) )");
const RegisteredProver merge_ants_rp = LibraryToolbox::register_prover({"|- ( ph <-> ( ps /\\ ch ) )"}, "|- ( ( et /\\ ph ) <-> ( ( et /\\ ps ) /\\ ch ) )");

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::merge_antecedents(const std::vector<std::vector<formula> > &ants, const Prover<CheckpointedProofEngine> &pr) const {
    return pr;
}

}
