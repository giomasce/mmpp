#pragma once

#include <string>

#include "mm/toolbox.h"
#include "mm/ptengine.h"

namespace gio::mmpp::setmm {

static const std::string CLASS = "class";
static const std::string VAR = "setvar";
static const std::string WFF = "wff";

const RegisteredProver true_trp = LibraryToolbox::register_prover({}, "wff T.");
const RegisteredProver false_trp = LibraryToolbox::register_prover({}, "wff F.");

Prover<CheckpointedProofEngine> build_true_prover(const LibraryToolbox &tb) {
    return tb.build_registered_prover(true_trp, {}, {});
}

Prover<CheckpointedProofEngine> build_false_prover(const LibraryToolbox &tb) {
    return tb.build_registered_prover(false_trp, {}, {});
}

}
