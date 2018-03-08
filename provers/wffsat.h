#pragma once

#include "wff.h"

std::pair< bool, Prover< CheckpointedProofEngine > > get_sat_prover(pwff wff, const LibraryToolbox &tb);
