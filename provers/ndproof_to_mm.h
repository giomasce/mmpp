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
    Prover<CheckpointedProofEngine> select_antecedents(const ndsequent &seq, const Prover<CheckpointedProofEngine> &pr, size_t idx1, size_t idx2) const;
    Prover<CheckpointedProofEngine> merge_antecedents(const std::vector<formula> &ant1, const std::vector<formula> &ant2, const formula &suc, const Prover<CheckpointedProofEngine> &pr) const;
    Prover<CheckpointedProofEngine> merge_antecedents(const std::vector<formula> &ant1, const std::vector<formula> &ant2, const std::vector<formula> &ant3, const formula &suc, const Prover<CheckpointedProofEngine> &pr) const;

    const LibraryToolbox &tb;
    const gio::mmpp::provers::fof::fof_to_mm_ctx &ctx;
};

}
