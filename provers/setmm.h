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

Prover<CheckpointedProofEngine> build_var_adaptor_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &var_prover);
Prover<CheckpointedProofEngine> build_true_prover(const LibraryToolbox &tb);
Prover<CheckpointedProofEngine> build_false_prover(const LibraryToolbox &tb);
Prover<CheckpointedProofEngine> build_not_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &arg_prover);
Prover<CheckpointedProofEngine> build_and_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &left_prover, const Prover<CheckpointedProofEngine> &right_prover);
Prover<CheckpointedProofEngine> build_or_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &left_prover, const Prover<CheckpointedProofEngine> &right_prover);
Prover<CheckpointedProofEngine> build_implies_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &left_prover, const Prover<CheckpointedProofEngine> &right_prover);
Prover<CheckpointedProofEngine> build_exists_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &var_prover, const Prover<CheckpointedProofEngine> &body_prover);
Prover<CheckpointedProofEngine> build_forall_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &var_prover, const Prover<CheckpointedProofEngine> &body_prover);
Prover<CheckpointedProofEngine> build_equal_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &left_prover, const Prover<CheckpointedProofEngine> &right_prover);
Prover<CheckpointedProofEngine> build_subst_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &var_prover, const Prover<CheckpointedProofEngine> &subst_prover, const Prover<CheckpointedProofEngine> &wff_prover);
Prover<CheckpointedProofEngine> build_class_subst_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &var_prover, const Prover<CheckpointedProofEngine> &subst_prover, const Prover<CheckpointedProofEngine> &class_prover);
Prover<CheckpointedProofEngine> build_label_prover(const LibraryToolbox &tb, LabTok lab);

}
