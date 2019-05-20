
#include "fof_to_mm.h"

#include <boost/type_index.hpp>

#include "setmm.h"

namespace gio::mmpp::provers::fof {

fof_to_mm_ctx::fof_to_mm_ctx(const LibraryToolbox &tb) : tb(tb) {}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::convert_prover(const std::shared_ptr<const gio::mmpp::provers::fof::FOF> &fof) {
    using namespace gio::mmpp::setmm;
    if (const auto fof_true = fof->mapped_dynamic_cast<const True>()) {
        return build_true_prover(this->tb);
    } else if (const auto fof_false = fof->mapped_dynamic_cast<const False>()) {
        return build_false_prover(this->tb);
    } else {
        gio_should_not_arrive_here_ctx(boost::typeindex::type_id_runtime(*fof).pretty_name());
    }
}

ParsingTree<SymTok, LabTok> fof_to_mm_ctx::convert(const std::shared_ptr<const FOF> &fof) {
    return pt2_to_pt(prover_to_pt2(this->tb, this->convert_prover(fof)));
}

}
