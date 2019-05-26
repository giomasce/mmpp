#pragma once

#include "mm/toolbox.h"
#include "fof.h"
#include "setmm.h"

namespace gio::mmpp::provers::fof {

class fof_to_mm_ctx {
public:
    fof_to_mm_ctx(const LibraryToolbox &tb);

    template<typename Cont>
    void alloc_vars(const Cont &c) {
        using namespace gio::mmpp::setmm;
        for (const auto &var : c) {
            auto it = this->vars.find(var);
            if (it == this->vars.end()) {
                this->vars.insert(std::make_pair(var, this->tsa.new_temp_var(setvar_sym(this->tb)).first));
            }
        }
    }

    template<typename Cont>
    void alloc_functs(const Cont &c) {
        using namespace gio::mmpp::setmm;
        for (const auto &func : c) {
            auto it = this->functs.find(func.first);
            if (it == this->functs.end()) {
                std::vector<LabTok> vars;
                for (size_t i = 0; i < func.second; i++) {
                    vars.push_back(this->tsa.new_temp_var(setvar_sym(this->tb)).first);
                }
                this->functs.insert(std::make_pair(func.first, std::make_pair(this->tsa.new_temp_var(class_sym(this->tb)).first, vars)));
            } else {
                gio::assert_or_throw<std::invalid_argument>(it->second.second.size() == func.second, "arity mismatch for functor " + func.first);
            }
        }
    }

    template<typename Cont>
    void alloc_preds(const Cont &c) {
        using namespace gio::mmpp::setmm;
        for (const auto &func : c) {
            auto it = this->preds.find(func.first);
            if (it == this->preds.end()) {
                std::vector<LabTok> vars;
                for (size_t i = 0; i < func.second; i++) {
                    vars.push_back(this->tsa.new_temp_var(setvar_sym(this->tb)).first);
                }
                this->preds.insert(std::make_pair(func.first, std::make_pair(this->tsa.new_temp_var(wff_sym(this->tb)).first, vars)));
            } else {
                gio::assert_or_throw<std::invalid_argument>(it->second.second.size() == func.second, "arity mismatch for predicate " + func.first);
            }
        }
    }

    Prover<CheckpointedProofEngine> convert_prover(const std::shared_ptr<const FOF> &fof) const;
    Prover<CheckpointedProofEngine> convert_prover(const std::shared_ptr<const FOT> &fot, bool make_class = false) const;
    ParsingTree<SymTok, LabTok> convert(const std::shared_ptr<const FOF> &fof) const;
    Prover<CheckpointedProofEngine> sethood_prover(const std::shared_ptr<const FOT> &fot) const;
    Prover<CheckpointedProofEngine> not_free_prover(const std::shared_ptr<const FOF> &fof, const std::string &var_name) const;
    Prover<CheckpointedProofEngine> replace_prover(const std::shared_ptr<const FOF> &fof, const std::string &var_name, const std::shared_ptr<const FOT> &term) const;

private:
    const LibraryToolbox &tb;
    temp_stacked_allocator tsa;
    std::map<std::string, LabTok> vars;
    std::map<std::string, std::pair<LabTok, std::vector<LabTok>>> functs;
    std::map<std::string, std::pair<LabTok, std::vector<LabTok>>> preds;
};

}
