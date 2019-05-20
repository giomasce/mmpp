
#include "setmm.h"

namespace gio::mmpp::setmm {

const std::string CLASS = "class";
const std::string SETVAR = "setvar";
const std::string WFF = "wff";

SymTok class_sym(const LibraryToolbox &tb) {
    return tb.get_symbol(CLASS);
}

SymTok setvar_sym(const LibraryToolbox &tb) {
    return tb.get_symbol(SETVAR);
}

SymTok wff_sym(const LibraryToolbox &tb) {
    return tb.get_symbol(WFF);
}

const RegisteredProver true_trp = LibraryToolbox::register_prover({}, "wff T.");
const RegisteredProver false_trp = LibraryToolbox::register_prover({}, "wff F.");

Prover<CheckpointedProofEngine> build_true_prover(const LibraryToolbox &tb) {
    return tb.build_registered_prover(true_trp, {}, {});
}

Prover<CheckpointedProofEngine> build_false_prover(const LibraryToolbox &tb) {
    return tb.build_registered_prover(false_trp, {}, {});
}

}
