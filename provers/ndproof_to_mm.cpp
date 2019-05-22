
#include "ndproof_to_mm.h"

namespace gio::mmpp::provers::ndproof {

nd_proof_to_mm_ctx::nd_proof_to_mm_ctx(const LibraryToolbox &tb, const fof::fof_to_mm_ctx &ctx) : tb(tb), ctx(ctx) {}

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::convert_ndsequent_prover(const ndsequent &seq) const {
    using namespace gio::mmpp::setmm;
    auto prover = build_true_prover(this->tb);
    for (const auto &ant : boost::adaptors::reverse(seq.first)) {
        prover = build_and_prover(this->tb, this->ctx.convert_prover(ant), prover);
    }
    prover = build_implies_prover(this->tb, prover, this->ctx.convert_prover(seq.second));
    return prover;
}

ParsingTree<SymTok, LabTok> nd_proof_to_mm_ctx::convert_ndsequent(const ndsequent &seq) const {
    return pt2_to_pt(prover_to_pt2(this->tb, this->convert_ndsequent_prover(seq)));
}

Prover<CheckpointedProofEngine> nd_proof_to_mm_ctx::convert_proof(const proof &pr) const {

}

}
