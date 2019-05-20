#pragma once

#include "mm/toolbox.h"
#include "fof.h"

namespace gio::mmpp::provers::fof {

class fof_to_mm_ctx {
public:
    fof_to_mm_ctx(const LibraryToolbox &tb);

    Prover<CheckpointedProofEngine> convert_prover(const std::shared_ptr<const FOF> &fof);
    ParsingTree<SymTok, LabTok> convert(const std::shared_ptr<const FOF> &fof);

private:
    const LibraryToolbox &tb;
};

}
