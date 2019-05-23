#pragma once

#include <boost/range/adaptor/reversed.hpp>

#include "mm/toolbox.h"
#include "ndproof.h"
#include "fof_to_mm.h"

namespace gio::mmpp::provers::ndproof {

class nd_proof_to_mm_ctx {
public:
    nd_proof_to_mm_ctx(const LibraryToolbox &tb, const gio::mmpp::provers::fof::fof_to_mm_ctx &ctx);

    Prover<CheckpointedProofEngine> convert_ndsequent_ant_prover(const ndsequent &seq) const;
    Prover<CheckpointedProofEngine> convert_ndsequent_suc_prover(const ndsequent &seq) const;
    Prover<CheckpointedProofEngine> convert_ndsequent_prover(const ndsequent &seq) const;
    ParsingTree<SymTok, LabTok> convert_ndsequent(const ndsequent &seq) const;
    Prover<CheckpointedProofEngine> convert_proof(const proof &pr) const;

private:
    Prover<CheckpointedProofEngine> select_antecedent(const ndsequent &seq, const Prover<CheckpointedProofEngine> &pr, size_t idx) const;
    Prover<CheckpointedProofEngine> merge_antecedents(const std::vector<std::vector<formula>> &ants, const Prover<CheckpointedProofEngine> &pr) const;

    const LibraryToolbox &tb;
    const gio::mmpp::provers::fof::fof_to_mm_ctx &ctx;
};

}
