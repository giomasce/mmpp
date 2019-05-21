
#include "fof_to_mm.h"

#include <boost/range/adaptor/reversed.hpp>
#include <boost/type_index.hpp>

#include "setmm.h"

namespace gio::mmpp::provers::fof {

fof_to_mm_ctx::fof_to_mm_ctx(const LibraryToolbox &tb) : tb(tb), tsa(tb) {}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::convert_prover(const std::shared_ptr<const gio::mmpp::provers::fof::FOF> &fof) const {
    using namespace gio::mmpp::setmm;
    if (const auto fof_true = fof->mapped_dynamic_cast<const True>()) {
        return build_true_prover(this->tb);
    } else if (const auto fof_false = fof->mapped_dynamic_cast<const False>()) {
        return build_false_prover(this->tb);
    } else if (const auto fof_and = fof->mapped_dynamic_cast<const And>()) {
        return build_and_prover(this->tb, this->convert_prover(fof_and->get_left()), this->convert_prover(fof_and->get_right()));
    } else if (const auto fof_or = fof->mapped_dynamic_cast<const Or>()) {
        return build_or_prover(this->tb, this->convert_prover(fof_or->get_left()), this->convert_prover(fof_or->get_right()));
    } else if (const auto fof_implies = fof->mapped_dynamic_cast<const Implies>()) {
        return build_implies_prover(this->tb, this->convert_prover(fof_implies->get_left()), this->convert_prover(fof_implies->get_right()));
    } else if (const auto fof_exists = fof->mapped_dynamic_cast<const Exists>()) {
        return build_exists_prover(this->tb, this->convert_prover(fof_exists->get_var()), this->convert_prover(fof_exists->get_arg()));
    } else if (const auto fof_forall = fof->mapped_dynamic_cast<const Forall>()) {
        return build_forall_prover(this->tb, this->convert_prover(fof_forall->get_var()), this->convert_prover(fof_forall->get_arg()));
    } else if (const auto fof_equal = fof->mapped_dynamic_cast<const Equal>()) {
        return build_equal_prover(this->tb, this->convert_prover(fof_equal->get_left(), true), this->convert_prover(fof_equal->get_right(), true));
    } else if (const auto fof_pred = fof->mapped_dynamic_cast<const Predicate>()) {
        const auto &pred = this->preds.at(fof_pred->get_name());
        auto prover = build_label_prover(this->tb, pred.first);
        size_t i = 0;
        for (const auto &arg : boost::adaptors::reverse(fof_pred->get_args())) {
            prover = build_subst_prover(this->tb, build_label_prover(this->tb, pred.second.at(i)), this->convert_prover(arg, true), prover);
            i++;
        }
        return prover;
    } else {
        gio_should_not_arrive_here_ctx(boost::typeindex::type_id_runtime(*fof).pretty_name());
    }
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::convert_prover(const std::shared_ptr<const FOT> &fot, bool make_class) const {
    using namespace gio::mmpp::setmm;
    if (const auto fot_funct = fot->mapped_dynamic_cast<const Functor>()) {
        const auto &funct = this->functs.at(fot_funct->get_name());
        auto prover = build_label_prover(this->tb, funct.first);
        size_t i = 0;
        for (const auto &arg : boost::adaptors::reverse(fot_funct->get_args())) {
            prover = build_class_subst_prover(this->tb, build_label_prover(this->tb, funct.second.at(i)), this->convert_prover(arg, true), prover);
            i++;
        }
        return prover;
    } else if (const auto fot_var = fot->mapped_dynamic_cast<const Variable>()) {
        auto prover = build_label_prover(this->tb, this->vars.at(fot_var->get_name()));
        if (make_class) {
            prover = build_var_adaptor_prover(this->tb, prover);
        }
        return prover;
    } else {
        gio_should_not_arrive_here_ctx(boost::typeindex::type_id_runtime(*fot).pretty_name());
    }
}

ParsingTree<SymTok, LabTok> fof_to_mm_ctx::convert(const std::shared_ptr<const FOF> &fof) const {
    return pt2_to_pt(prover_to_pt2(this->tb, this->convert_prover(fof)));
}

}
