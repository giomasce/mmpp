#pragma once

#include "mm/toolbox.h"
#include "fof.h"
#include "setmm.h"

namespace gio::mmpp::provers::fof {

class fof_to_mm_ctx {
public:
    fof_to_mm_ctx(const LibraryToolbox &tb);

    void alloc_var(const std::string &name, LabTok label);
    void alloc_functor(const std::string &name, const std::vector<LabTok> &vars, Prover<CheckpointedProofEngine> expr_prover, Prover<CheckpointedProofEngine> sethood_prover, std::function<Prover<CheckpointedProofEngine>(LabTok)> not_free_prover);
    void alloc_predicate(const std::string &name, const std::vector<LabTok> &vars, Prover<CheckpointedProofEngine> expr_prover, std::function<Prover<CheckpointedProofEngine>(LabTok)> not_free_prover);

    Prover<CheckpointedProofEngine> convert_prover(const std::shared_ptr<const FOT> &fot, bool make_class = false) const;
    Prover<CheckpointedProofEngine> convert_prover(const std::shared_ptr<const FOF> &fof) const;
    ParsingTree<SymTok, LabTok> convert(const std::shared_ptr<const FOF> &fof) const;
    Prover<CheckpointedProofEngine> sethood_prover(const std::shared_ptr<const FOT> &fot) const;
    Prover<CheckpointedProofEngine> not_free_prover(const std::shared_ptr<const FOT> &fot, const std::string &var_name) const;
    Prover<CheckpointedProofEngine> not_free_prover(const std::shared_ptr<const FOF> &fof, const std::string &var_name) const;
    Prover<CheckpointedProofEngine> replace_prover(const std::shared_ptr<const FOT> &fot, const std::string &var_name, const std::shared_ptr<const FOT> &term) const;
    Prover<CheckpointedProofEngine> replace_prover(const std::shared_ptr<const FOF> &fof, const std::string &var_name, const std::shared_ptr<const FOT> &term) const;

private:
    Prover<CheckpointedProofEngine> convert_predicate_prover(const std::shared_ptr<const Predicate> &pred, size_t start_idx = 0) const;
    Prover<CheckpointedProofEngine> not_free_predicate_prover(const std::shared_ptr<const Predicate> &pred, const std::string &var_name, size_t start_idx = 0) const;
    Prover<CheckpointedProofEngine> convert_functor_prover(const std::shared_ptr<const Functor> &func, size_t start_idx = 0) const;
    Prover<CheckpointedProofEngine> not_free_functor_prover(const std::shared_ptr<const Functor> &func, const std::string &var_name, size_t start_idx = 0) const;
    //Prover<CheckpointedProofEngine> internal_not_free_prover(const std::shared_ptr<const FOT> &fot, const std::string &var_name, size_t start_idx = 0) const;
    Prover<CheckpointedProofEngine> replace_predicate_prover(const std::shared_ptr<const Predicate> &pred, const std::string &var_name, const std::shared_ptr<const FOT> &term, size_t start_idx = 0) const;
    Prover<CheckpointedProofEngine> internal_not_free_prover(const std::shared_ptr<const FOT> &fot, LabTok var_lab) const;
    Prover<CheckpointedProofEngine> internal_not_free_functor_prover(const std::shared_ptr<const Functor> &funct, LabTok var_lab, size_t start_idx = 0) const;

    const LibraryToolbox &tb;
    temp_stacked_allocator tsa;
    std::map<std::string, LabTok> vars;
    std::map<std::string, std::tuple<std::vector<LabTok>, Prover<CheckpointedProofEngine>, Prover<CheckpointedProofEngine>, std::function<Prover<CheckpointedProofEngine>(LabTok)>>> functs;
    std::map<std::string, std::tuple<std::vector<LabTok>, Prover<CheckpointedProofEngine>, std::function<Prover<CheckpointedProofEngine>(LabTok)>>> preds;
};

}
