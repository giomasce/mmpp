#pragma once

#include <string>

#include "mm/toolbox.h"
#include "mm/ptengine.h"

namespace gio::mmpp::setmm {

extern const std::string CLASS;
extern const std::string SETVAR;
extern const std::string WFF;

SymTok class_sym(const LibraryToolbox &tb);
SymTok setvar_sym(const LibraryToolbox &tb);
SymTok wff_sym(const LibraryToolbox &tb);

extern const RegisteredProver true_trp;
extern const RegisteredProver false_trp;

Prover<CheckpointedProofEngine> build_true_prover(const LibraryToolbox &tb);
Prover<CheckpointedProofEngine> build_false_prover(const LibraryToolbox &tb);

}
