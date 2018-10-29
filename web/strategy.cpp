
#include <giolib/containers.h>

#include "strategy.h"
#include "provers/wff.h"
#include "provers/wffblock.h"
#include "provers/wffsat.h"
#include "provers/uct.h"

StepStrategy::~StepStrategy() {
}

StepStrategy::StepStrategy(std::weak_ptr<StrategyManager> manager, std::shared_ptr<const StepStrategyData> data, const LibraryToolbox &toolbox)
    : manager(manager), data(data), toolbox(toolbox) {
}

void StepStrategy::maybe_report_result(std::shared_ptr< StepStrategy > strategy, std::shared_ptr<StepStrategyResult> result) {
    auto strong_manager = this->manager.lock();
    if (strong_manager) {
        strong_manager->report_result(strategy, result);
    }
}

class FailingStrategyResult : public StepStrategyResult, public gio::enable_create< FailingStrategyResult > {
protected:
    FailingStrategyResult() {}

    bool get_success() const {
        return false;
    }

    nlohmann::json get_web_json() const {
        return {};
    }

    nlohmann::json get_dump_json() const {
        return {};
    }

    bool prove(CheckpointedProofEngine &engine, const std::vector< std::shared_ptr< StepStrategyCallback > > &children) const {
        (void) engine;
        (void) children;

        return false;
    }
};

void FailingStrategy::operator()(Yielder &yield) {
    (void) yield;

    this->maybe_report_result(this->shared_from_this(), FailingStrategyResult::create());
}

struct UnificationStrategyResult : public StepStrategyResult, public gio::enable_create< UnificationStrategyResult > {
    UnificationStrategyResult(const LibraryToolbox &toolbox) : toolbox(toolbox) {}

    bool get_success() const {
        return this->success;
    }

    nlohmann::json get_web_json() const {
        nlohmann::json ret = nlohmann::json::object();
        LabTok label = std::get<0>(this->data);
        ret["type"] = "unification";
        ret["label"] = label;
        ret["permutation"] = std::get<1>(this->data);
        ret["subst_map"] = std::get<2>(this->data);
        auto ass = this->toolbox.get_assertion(label);
        if (ass.is_valid()) {
            ret["number"] = ass.get_number();
        } else {
            ret["number"] = LabTok{};
        }
        return ret;
    }

    nlohmann::json get_dump_json() const {
        nlohmann::json ret = nlohmann::json::object();
        ret["type"] = "unification";
        ret["label"] = std::get<0>(this->data);
        ret["permutation"] = std::get<1>(this->data);
        ret["subst_map"] = std::get<2>(this->data);
        return ret;
    }

    bool prove(CheckpointedProofEngine &engine, const std::vector< std::shared_ptr< StepStrategyCallback > > &children) const {
        RegisteredProverInstanceData inst_data(this->data);
        std::vector< Prover< CheckpointedProofEngine > > hyps_provers;
        for (const auto &child : children) {
            hyps_provers.push_back([child](CheckpointedProofEngine &engine2) {
                (void) engine2;
                return child->prove();
            });
        }
        // FIXME Here I assume that hyps_provers will be called on the same engine that is passed to proving_helper
        return this->toolbox.proving_helper(inst_data, std::unordered_map< SymTok, Prover< CheckpointedProofEngine > >{}, hyps_provers, engine);
    }

    bool success;
    std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > data;
    const LibraryToolbox &toolbox;
};

void UnificationStrategy::operator()(Yielder &yield) {
    auto result = UnificationStrategyResult::create(this->toolbox);
    result->success = false;

    Finally f1([this,result]() {
        this->maybe_report_result(this->shared_from_this(), result);
    });

    yield();

    auto pt_th = std::make_pair(this->data->thesis[0], this->data->pt_thesis);
    std::vector< std::pair< SymTok, ParsingTree< SymTok, LabTok > > > pt_hyps;
    for (size_t i = 0; i < this->data->pt_hypotheses.size(); i++) {
        pt_hyps.push_back(std::make_pair(this->data->hypotheses[i][0], this->data->pt_hypotheses[i]));
    }

    auto res = this->toolbox.unify_assertion(pt_hyps, pt_th, true, true, this->data->antidists);
    if (!res.empty()) {
        result->success = true;
        result->data = res[0];
    }
}

struct WffStrategyResult : public StepStrategyResult, public gio::enable_create< WffStrategyResult > {
    WffStrategyResult(const LibraryToolbox &toolbox) : toolbox(toolbox) {}

    bool get_success() const {
        return this->success;
    }

    nlohmann::json get_web_json() const {
        nlohmann::json ret;
        ret["type"] = "wff";
        return ret;
    }

    nlohmann::json get_dump_json() const {
        nlohmann::json ret;
        ret["type"] = "wff";
        return ret;
    }

    bool prove(CheckpointedProofEngine &engine, const std::vector< std::shared_ptr< StepStrategyCallback > > &children) const {
        std::vector< Prover< CheckpointedProofEngine > > hyps_provers;
        for (const auto &child : children) {
            hyps_provers.push_back([child](CheckpointedProofEngine &engine2) {
                (void) engine2;
                return child->prove();
            });
        }
        auto prover2 = this->prover;
        auto wff2 = this->wff;
        for (int i = (int) children.size()-1; i >= 0; i--) {
            auto wff_imp = std::dynamic_pointer_cast< const Imp >(wff2);
            assert(wff_imp);
            prover2 = imp_mp_prover(wff_imp, hyps_provers[i], prover2, this->toolbox);
            wff2 = wff_imp->get_b();
        }
        return prover2(engine);
    }

    bool success;
    Prover<CheckpointedProofEngine> prover;
    pwff wff;
    const LibraryToolbox &toolbox;
};

WffStrategy::WffStrategy(std::weak_ptr<StrategyManager> manager, std::shared_ptr<const StepStrategyData> data, const LibraryToolbox &toolbox, WffStrategy::SubStrategy substrategy) :
    StepStrategy(manager, data, toolbox), substrategy(substrategy)
{
}

void WffStrategy::operator()(Yielder &yield)
{
    auto result = WffStrategyResult::create(this->toolbox);
    result->success = false;

    Finally f1([this,result]() {
        this->maybe_report_result(this->shared_from_this(), result);
    });

    auto wff = wff_from_pt<PropTag>(this->data->pt_thesis, this->toolbox);
    for (const auto &pt_hyp : this->data->pt_hypotheses) {
        wff = Imp::create(wff_from_pt<PropTag>(pt_hyp, this->toolbox), wff);
        yield();
    }

    result->wff = wff;
    switch (this->substrategy) {
    case SUBSTRATEGY_WFF:
        tie(result->success, result->prover) = wff->get_adv_truth_prover(this->toolbox);
        break;
    case SUBSTRATEGY_WFFSAT:
        tie(result->success, result->prover) = get_sat_prover(wff, this->toolbox);
        break;
    default:
        assert(!"Should not arrive here");
    }
}

struct UctStrategyResult : public StepStrategyResult, public gio::enable_create< UctStrategyResult > {
    UctStrategyResult() {}

    bool get_success() const {
        return this->success;
    }

    nlohmann::json get_web_json() const {
        nlohmann::json ret;
        ret["type"] = "uct";
        ret["visits_num"] = this->visits_num;
        return ret;
    }

    nlohmann::json get_dump_json() const {
        nlohmann::json ret;
        ret["type"] = "uct";
        return ret;
    }

    bool prove(CheckpointedProofEngine &engine, const std::vector< std::shared_ptr< StepStrategyCallback > > &children) const {
        this->prover->set_children_callbacks(gio::vector_map(children.begin(), children.end(),
                                                             [](auto x) -> std::function< void() > { return [x]() { x->prove(); }; }));
        this->prover->replay_proof(engine);
        return true;
    }

    bool success;
    std::shared_ptr< UCTProver > prover;
    unsigned visits_num;
};

void UctStrategy::operator()(Yielder &yield)
{
    auto result = UctStrategyResult::create();
    result->success = false;

    Finally f1([this,result]() {
        this->maybe_report_result(this->shared_from_this(), result);
    });

    auto thesis = pt_to_pt2(this->data->pt_thesis);
    auto hyps = gio::vector_map(this->data->pt_hypotheses.begin(), this->data->pt_hypotheses.end(),
                                [](const auto &x) { return pt_to_pt2(x); });
    result->prover = UCTProver::create(this->toolbox, thesis, hyps, this->data->lab_antidists);
    result->visits_num = 0;

    yield();

    for (unsigned i = 0; i < 10000; i++) {
        auto res = result->prover->visit();
        result->visits_num++;
        if (res == PROVED) {
            result->success = true;
            return;
        } else if (res == DEAD) {
            return;
        }
        yield();
    }
}

std::vector<std::shared_ptr<StepStrategy> > create_strategies(unsigned priority, std::weak_ptr<StrategyManager> manager, std::shared_ptr<const StepStrategyData> data, const LibraryToolbox &toolbox)
{
    switch (priority) {
    case 0:
        return {
            //FailingStrategy::create(manager, data, toolbox),
            UnificationStrategy::create(manager, data, toolbox),
        };
    case 1:
        return {
            //WffStrategy::create(manager, data, toolbox, WffStrategy::SUBSTRATEGY_WFF),
            WffStrategy::create(manager, data, toolbox, WffStrategy::SUBSTRATEGY_WFFSAT),
        };
    case 2:
        return {
            UctStrategy::create(manager, data, toolbox),
        };
    default:
        return {};
    }
};
