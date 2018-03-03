#pragma once

#include <memory>

#include "wff.h"
#include "mm/toolbox.h"

Prover<CheckpointedProofEngine> imp_mp_prover(std::shared_ptr< const Imp > imp, Prover<CheckpointedProofEngine> ant_prover, Prover<CheckpointedProofEngine> imp_prover, const LibraryToolbox &tb);
Prover<CheckpointedProofEngine> or_res_prover(const std::vector< pwff > &orands, const std::vector<bool> &orand_sign, size_t thesis_idx, Prover<CheckpointedProofEngine> or_prover, const std::vector< Prover<CheckpointedProofEngine> > &orand_prover, pwff glob_ctx, pwff loc_ctx, const LibraryToolbox &tb);
