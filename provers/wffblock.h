#pragma once

#include <memory>

#include "wff.h"
#include "mm/toolbox.h"

Prover<CheckpointedProofEngine> imp_mp_prover(std::shared_ptr< const Imp > imp, Prover<CheckpointedProofEngine> ant_prover, Prover<CheckpointedProofEngine> imp_prover, const LibraryToolbox &tb);
Prover<CheckpointedProofEngine> unit_res_prover(const std::vector< pwff > &orands, const std::vector<bool> &orand_sign, size_t thesis_idx, Prover<CheckpointedProofEngine> or_prover, const std::vector< Prover<CheckpointedProofEngine> > &orand_prover, pwff glob_ctx, pwff loc_ctx, const LibraryToolbox &tb);
Prover<CheckpointedProofEngine> not_or_elim_prover(const std::vector< pwff > &orands, size_t thesis_idx, bool thesis_sign, Prover<CheckpointedProofEngine> not_or_prover, pwff loc_ctx, const LibraryToolbox &tb);
Prover<CheckpointedProofEngine> absurdum_prover(pwff concl, Prover<CheckpointedProofEngine> pos_prover, Prover<CheckpointedProofEngine> neg_prover, pwff glob_ctx, pwff loc_ctx, const LibraryToolbox &tb);
Prover<CheckpointedProofEngine> imp_intr_prover(pwff concl, Prover<CheckpointedProofEngine> concl_prover, pwff glob_ctx, pwff loc_ctx, const LibraryToolbox &tb);
