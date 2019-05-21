
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

const RegisteredProver var_adaptor_trp = LibraryToolbox::register_prover({}, "class x");
const RegisteredProver true_trp = LibraryToolbox::register_prover({}, "wff T.");
const RegisteredProver false_trp = LibraryToolbox::register_prover({}, "wff F.");
const RegisteredProver and_trp = LibraryToolbox::register_prover({}, "wff ( ph /\\ ps )");
const RegisteredProver or_trp = LibraryToolbox::register_prover({}, "wff ( ph \\/ ps )");
const RegisteredProver implies_trp = LibraryToolbox::register_prover({}, "wff ( ph -> ps )");
const RegisteredProver exists_trp = LibraryToolbox::register_prover({}, "wff E. x ph");
const RegisteredProver forall_trp = LibraryToolbox::register_prover({}, "wff A. x ph");
const RegisteredProver equal_trp = LibraryToolbox::register_prover({}, "wff A = B");
const RegisteredProver subst_trp = LibraryToolbox::register_prover({}, "wff [. A / x ]. ph");
const RegisteredProver class_subst_trp = LibraryToolbox::register_prover({}, "class [_ A / x ]_ B");

Prover<CheckpointedProofEngine> build_var_adaptor_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &var_prover) {
    return tb.build_registered_prover(var_adaptor_trp, {{"x", var_prover}}, {});
}

Prover<CheckpointedProofEngine> build_true_prover(const LibraryToolbox &tb) {
    return tb.build_registered_prover(true_trp, {}, {});
}

Prover<CheckpointedProofEngine> build_false_prover(const LibraryToolbox &tb) {
    return tb.build_registered_prover(false_trp, {}, {});
}

Prover<CheckpointedProofEngine> build_and_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &left_prover, const Prover<CheckpointedProofEngine> &right_prover) {
    return tb.build_registered_prover(and_trp, {{"ph", left_prover}, {"ps", right_prover}}, {});
}

Prover<CheckpointedProofEngine> build_or_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &left_prover, const Prover<CheckpointedProofEngine> &right_prover) {
    return tb.build_registered_prover(or_trp, {{"ph", left_prover}, {"ps", right_prover}}, {});
}

Prover<CheckpointedProofEngine> build_implies_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &left_prover, const Prover<CheckpointedProofEngine> &right_prover) {
    return tb.build_registered_prover(implies_trp, {{"ph", left_prover}, {"ps", right_prover}}, {});
}

Prover<CheckpointedProofEngine> build_exists_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &var_prover, const Prover<CheckpointedProofEngine> &body_prover) {
    return tb.build_registered_prover(exists_trp, {{"x", var_prover}, {"ph", body_prover}}, {});
}

Prover<CheckpointedProofEngine> build_forall_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &var_prover, const Prover<CheckpointedProofEngine> &body_prover) {
    return tb.build_registered_prover(forall_trp, {{"x", var_prover}, {"ph", body_prover}}, {});
}

Prover<CheckpointedProofEngine> build_subst_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &var_prover, const Prover<CheckpointedProofEngine> &subst_prover, const Prover<CheckpointedProofEngine> &wff_prover) {
    return tb.build_registered_prover(subst_trp, {{"A", subst_prover}, {"x", var_prover}, {"ph", wff_prover}}, {});
}

Prover<CheckpointedProofEngine> build_equal_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &left_prover, const Prover<CheckpointedProofEngine> &right_prover) {
    return tb.build_registered_prover(equal_trp, {{"A", left_prover}, {"B", right_prover}}, {});
}

Prover<CheckpointedProofEngine> build_class_subst_prover(const LibraryToolbox &tb, const Prover<CheckpointedProofEngine> &var_prover, const Prover<CheckpointedProofEngine> &subst_prover, const Prover<CheckpointedProofEngine> &class_prover) {
    return tb.build_registered_prover(class_subst_trp, {{"A", subst_prover}, {"x", var_prover}, {"B", class_prover}}, {});
}

Prover<CheckpointedProofEngine> build_label_prover(const LibraryToolbox &tb, LabTok lab) {
    (void) tb;
    return [lab](CheckpointedProofEngine &engine) {
        engine.process_label(lab);
        return true;
    };
}

}
