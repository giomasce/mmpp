
#include "strategy.h"

using namespace std;
using namespace nlohmann;

StepStrategy::~StepStrategy() {
}

StepStrategy::StepStrategy(std::weak_ptr<StrategyManager> manager, const Sentence &thesis, const std::vector<Sentence> &hypotheses, const LibraryToolbox &toolbox)
    : manager(manager), thesis(thesis), hypotheses(hypotheses), toolbox(toolbox) {
}

void StepStrategy::maybe_report_result(std::shared_ptr< StepStrategy > strategy, std::shared_ptr<StepStrategyResult> result) {
    auto strong_manager = this->manager.lock();
    if (strong_manager) {
        strong_manager->report_result(strategy, result);
    }
}

class FailingStrategyResult : public StepStrategyResult, public enable_create< FailingStrategyResult > {
protected:
    FailingStrategyResult() {}

    bool get_success() const {
        return false;
    }

    nlohmann::json get_web_json() const {
        return {};
    }

    virtual bool prove(ExtendedProofEngine< Sentence > &engine, const std::vector< std::shared_ptr< StepStrategyCallback > > &children) const {
        (void) engine;
        (void) children;

        return false;
    }
};

void FailingStrategy::operator()(Yield &yield) {
    (void) yield;

    this->maybe_report_result(this->shared_from_this(), FailingStrategyResult::create());
}

struct UnificationStrategyResult : public StepStrategyResult, public enable_create< UnificationStrategyResult > {
    UnificationStrategyResult(const LibraryToolbox &toolbox) : toolbox(toolbox) {}

    bool get_success() const {
        return this->success;
    }

    nlohmann::json get_web_json() const {
        json ret;
        LabTok label = get<0>(this->data);
        ret["type"] = "unification";
        ret["label"] = label;
        ret["permutation"] = get<1>(this->data);
        ret["subst_map"] = get<2>(this->data);
        auto ass = this->toolbox.get_assertion(label);
        if (ass.is_valid()) {
            ret["number"] = ass.get_number();
        } else {
            ret["number"] = LabTok{};
        }
        return ret;
    }

    virtual bool prove(ExtendedProofEngine< Sentence > &engine, const std::vector< std::shared_ptr< StepStrategyCallback > > &children) const {
        RegisteredProverInstanceData inst_data(this->data);
        vector< Prover > hyps_provers;
        for (const auto &child : children) {
            hyps_provers.push_back([child](ExtendedProofEngine< Sentence > &engine2) {
                return child->prove(engine2);
            });
        }
        return this->toolbox.proving_helper(inst_data, std::unordered_map< SymTok, Prover >{}, hyps_provers, engine);
    }

    bool success;
    std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > data;
    const LibraryToolbox &toolbox;
};

void UnificationStrategy::operator()(Yield &yield) {
    auto result = UnificationStrategyResult::create(this->toolbox);

    Finally f1([this,result]() {
        this->maybe_report_result(this->shared_from_this(), result);
    });

    bool can_go = true;
    if (this->thesis.size() == 0) {
        can_go = false;
    }
    for (const auto &hyp : this->hypotheses) {
        if (hyp.size() == 0) {
            can_go = false;
        }
    }
    if (!can_go) {
        result->success = false;
        return;
    }

    yield();

    auto res = this->toolbox.unify_assertion(this->hypotheses, this->thesis);
    if (!res.empty()) {
        result->success = true;
        result->data = res[0];
    } else {
        result->success = false;
    }
}
