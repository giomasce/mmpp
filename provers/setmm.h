#pragma once

#include <string>

#include "mm/toolbox.h"
#include "mm/ptengine.h"

namespace gio::mmpp::setmm {

static const std::string CLASS = "class";
static const std::string VAR = "setvar";
static const std::string WFF = "wff";

const RegisteredProver true_trp = LibraryToolbox::register_prover({}, "wff T.");

ParsingTree<SymTok, LabTok> build_true(const LibraryToolbox &tb) {
    auto prover = tb.build_registered_prover(true_trp, {}, {});
    return pt2_to_pt(prover_to_pt2(tb, prover));
}

}
