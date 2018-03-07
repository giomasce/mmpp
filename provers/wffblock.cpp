#include "wffblock.h"

using namespace std;

static const RegisteredProver mp_rp = LibraryToolbox::register_prover({"|- ph", "|- ( ph -> ps )"}, "|- ps");
Prover<CheckpointedProofEngine> imp_mp_prover(shared_ptr< const Imp > imp, Prover<CheckpointedProofEngine> ant_prover, Prover<CheckpointedProofEngine> imp_prover, const LibraryToolbox &tb)
{
    return tb.build_registered_prover< CheckpointedProofEngine >(mp_rp, { {"ph", imp->get_a()->get_type_prover(tb)}, {"ps", imp->get_b()->get_type_prover(tb)}}, { ant_prover, imp_prover });
}

static const RegisteredProver unitp_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps -> ( ch \\/ th ) ) )", "|- ( ph -> ( ps -> -. th ) )"}, "|- ( ph -> ( ps -> ch ) )");
static const RegisteredProver unitn_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps -> ( ch \\/ -. th ) ) )", "|- ( ph -> ( ps -> th ) )"}, "|- ( ph -> ( ps -> ch ) )");
static const RegisteredProver or_ass_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps -> ( ( ch \\/ th ) \\/ ta ) ) )"}, "|- ( ph -> ( ps -> ( ( ch \\/ ta ) \\/ th ) ) )");
static const RegisteredProver or_com_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps -> ( ch \\/ th ) ) )"}, "|- ( ph -> ( ps -> ( th \\/ ch ) ) )");
Prover<CheckpointedProofEngine> unit_res_prover(const std::vector<pwff> &orands, const std::vector<bool> &orand_sign, size_t thesis_idx,
                                              Prover<CheckpointedProofEngine> or_prover, const std::vector< Prover<CheckpointedProofEngine> > &orand_prover,
                                              pwff glob_ctx, pwff loc_ctx, const LibraryToolbox &tb)
{
    assert(orand_prover.size() + 1 == orands.size());
    assert(orand_prover.size() == orand_sign.size());
    assert(thesis_idx < orands.size());
    pwff structure = orands[0];
    for (size_t i = 1; i < orands.size(); i++) {
        structure = Or::create(structure, orands[i]);
    }
    auto ret = or_prover;
    for (size_t eliminating = orands.size() - 1; eliminating > 0; eliminating--) {
        if (eliminating <= thesis_idx) {
            if (eliminating == 1) {
                auto or_struct = dynamic_pointer_cast< const Or >(structure);
                assert(or_struct);
                auto or_left = or_struct->get_a();
                auto or_right = or_struct->get_b();
                ret = tb.build_registered_prover(or_com_rp, {{"ph", glob_ctx->get_type_prover(tb)}, {"ps", loc_ctx->get_type_prover(tb)}, {"ch", or_left->get_type_prover(tb)}, {"th", or_right->get_type_prover(tb)}}, {ret});
                structure = Or::create(or_right, or_left);
            } else {
                auto or_struct = dynamic_pointer_cast< const Or >(structure);
                assert(or_struct);
                auto or_left = or_struct->get_a();
                auto or_right = or_struct->get_b();
                auto or_left_or = dynamic_pointer_cast< const Or >(or_left);
                assert(or_left_or);
                auto or_left_or_left = or_left_or->get_a();
                auto or_left_or_right = or_left_or->get_b();
                ret = tb.build_registered_prover(or_ass_rp, {{"ph", glob_ctx->get_type_prover(tb)}, {"ps", loc_ctx->get_type_prover(tb)}, {"ch", or_left_or_left->get_type_prover(tb)}, {"th", or_left_or_right->get_type_prover(tb)}, {"ta", or_right->get_type_prover(tb)}}, {ret});
                structure = Or::create(Or::create(or_left_or_left, or_right), or_left_or_right);
            }
        }
        if (orand_sign[eliminating-1]) {
            auto or_struct = dynamic_pointer_cast< const Or >(structure);
            assert(or_struct);
            auto or_left = or_struct->get_a();
            auto or_right = or_struct->get_b();
            ret = tb.build_registered_prover(unitp_rp, {{"ph", glob_ctx->get_type_prover(tb)}, {"ps", loc_ctx->get_type_prover(tb)}, {"ch", or_left->get_type_prover(tb)}, {"th", or_right->get_type_prover(tb)}}, {ret, orand_prover[eliminating-1]});
            structure = or_left;
        } else {
            auto or_struct = dynamic_pointer_cast< const Or >(structure);
            assert(or_struct);
            auto or_left = or_struct->get_a();
            auto or_right = or_struct->get_b();
            auto neg_or_right = dynamic_pointer_cast< const Not >(or_right);
            assert(neg_or_right);
            auto or_right_arg = neg_or_right->get_a();
            ret = tb.build_registered_prover(unitn_rp, {{"ph", glob_ctx->get_type_prover(tb)}, {"ps", loc_ctx->get_type_prover(tb)}, {"ch", or_left->get_type_prover(tb)}, {"th", or_right_arg->get_type_prover(tb)}}, {ret, orand_prover[eliminating-1]});
            structure = or_left;
        }
    }
    return ret;
}

static const RegisteredProver or_eliml_rp = LibraryToolbox::register_prover({"|- ( ph -> -. ( ps \\/ ch )  )"}, "|- ( ph -> -. ps )");
static const RegisteredProver or_elimr_rp = LibraryToolbox::register_prover({"|- ( ph -> -. ( ps \\/ ch )  )"}, "|- ( ph -> -. ch )");
static const RegisteredProver or_elimln_rp = LibraryToolbox::register_prover({"|- ( ph -> -. ( -. ps \\/ ch )  )"}, "|- ( ph -> ps )");
static const RegisteredProver or_elimrn_rp = LibraryToolbox::register_prover({"|- ( ph -> -. ( ps \\/ -. ch )  )"}, "|- ( ph -> ch )");
Prover<CheckpointedProofEngine> not_or_elim_prover(const std::vector<pwff> &orands, size_t thesis_idx, bool thesis_sign, Prover<CheckpointedProofEngine> not_or_prover, pwff glob_ctx, const LibraryToolbox &tb)
{
    assert(thesis_idx < orands.size());
    pwff structure = orands[0];
    for (size_t i = 1; i < orands.size(); i++) {
        structure = Or::create(structure, orands[i]);
    }
    auto ret = not_or_prover;
    for (size_t eliminating = orands.size() - 1; eliminating > 0; eliminating--) {
        auto or_struct = dynamic_pointer_cast< const Or >(structure);
        assert(or_struct);
        auto or_left = or_struct->get_a();
        auto or_right = or_struct->get_b();
        if (eliminating == thesis_idx) {
            if (thesis_sign) {
                ret = tb.build_registered_prover(or_elimr_rp, {{"ph", glob_ctx->get_type_prover(tb)}, {"ps", or_left->get_type_prover(tb)}, {"ch", or_right->get_type_prover(tb)}}, {ret});
            } else {
                auto or_right_not = dynamic_pointer_cast< const Not >(or_right);
                assert(or_right_not);
                auto or_right_not_neg = or_right_not->get_a();
                ret = tb.build_registered_prover(or_elimrn_rp, {{"ph", glob_ctx->get_type_prover(tb)}, {"ps", or_left->get_type_prover(tb)}, {"ch", or_right_not_neg->get_type_prover(tb)}}, {ret});
            }
            break;
        } else {
            if (eliminating == 1 && thesis_sign) {
                auto or_left_not = dynamic_pointer_cast< const Not >(or_left);
                assert(or_left_not);
                auto or_left_not_neg = or_left_not->get_a();
                ret = tb.build_registered_prover(or_elimln_rp, {{"ph", glob_ctx->get_type_prover(tb)}, {"ps", or_left_not_neg->get_type_prover(tb)}, {"ch", or_right->get_type_prover(tb)}}, {ret});
            } else {
                ret = tb.build_registered_prover(or_eliml_rp, {{"ph", glob_ctx->get_type_prover(tb)}, {"ps", or_left->get_type_prover(tb)}, {"ch", or_right->get_type_prover(tb)}}, {ret});
                structure = or_left;
            }
        }
    }
    return ret;
}

static const RegisteredProver absurd_rp = LibraryToolbox::register_prover({"|- ( ph -> ( -. ps -> ch ) )", "|- ( ph -> ( -. ps -> -. ch ) )"}, "|- ( ph -> ps )");
Prover<CheckpointedProofEngine> absurdum_prover(pwff concl, Prover<CheckpointedProofEngine> pos_prover, Prover<CheckpointedProofEngine> neg_prover, pwff glob_ctx, pwff loc_ctx, const LibraryToolbox &tb)
{
    return tb.build_registered_prover(absurd_rp, {{"ph", glob_ctx->get_type_prover(tb)}, {"ps", loc_ctx->get_type_prover(tb)}, {"ch", concl->get_type_prover(tb)}}, {pos_prover, neg_prover});
}

static const RegisteredProver imp_intr_rp = LibraryToolbox::register_prover({"|- ( ph -> ch )"}, "|- ( ph -> ( ps -> ch ) )");
Prover<CheckpointedProofEngine> imp_intr_prover(pwff concl, Prover<CheckpointedProofEngine> concl_prover, pwff glob_ctx, pwff loc_ctx, const LibraryToolbox &tb)
{
    return tb.build_registered_prover(imp_intr_rp, {{"ph", glob_ctx->get_type_prover(tb)}, {"ps", loc_ctx->get_type_prover(tb)}, {"ch", concl->get_type_prover(tb)}}, {concl_prover});
}
