
#include "strategy.h"
#include "provers/wff.h"
#include "provers/wffblock.h"
#include "provers/wffsat.h"

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

class FailingStrategyResult : public StepStrategyResult, public enable_create< FailingStrategyResult > {
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

struct UnificationStrategyResult : public StepStrategyResult, public enable_create< UnificationStrategyResult > {
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

    Finally f1([this,result]() {
        this->maybe_report_result(this->shared_from_this(), result);
    });

    bool can_go = true;
    if (this->data->thesis.size() == 0) {
        can_go = false;
    }
    for (const auto &hyp : this->data->hypotheses) {
        if (hyp.size() == 0) {
            can_go = false;
        }
    }
    if (!can_go) {
        result->success = false;
        return;
    }

    yield();

    auto res = this->toolbox.unify_assertion(this->data->hypotheses, this->data->thesis, false);
    result->success = false;
    for (const auto &match : res) {
        // Check that the substitution map does not violate any antidist constraint
        const auto &ass = this->toolbox.get_assertion(std::get<0>(match));
        assert(ass.is_valid());
        const auto &ass_dists = ass.get_dists();
        const std::set< std::pair< SymTok, SymTok > > antidists;
        const auto &tb = this->toolbox;
        const auto &subst_map = std::get<2>(match);
        for (auto it1 = subst_map.begin(); it1 != subst_map.end(); it1++) {
            for (auto it2 = subst_map.begin(); it2 != it1; it2++) {
                const auto &var1 = it1->first;
                const auto &var2 = it2->first;
                const auto &subst1 = it1->second;
                const auto &subst2 = it2->second;
                if (ass_dists.find(std::minmax(var1, var2)) != ass_dists.end()) {
                    std::set< SymTok > vars1;
                    std::set< SymTok > vars2;
                    collect_variables(subst1, tb.get_standard_is_var_sym(), vars1);
                    collect_variables(subst2, tb.get_standard_is_var_sym(), vars2);
                    for (const auto &lab1 : vars1) {
                        for (const auto &lab2 : vars2) {
                            if (lab1 == lab2 || antidists.find(std::minmax(lab1, lab2)) != antidists.end()) {
                                goto not_valid;
                            }
                        }
                    }
                }
            }
        }
        result->success = true;
        result->data = match;
        break;
        not_valid:
        continue;
    }
}

struct WffStrategyResult : public StepStrategyResult, public enable_create< WffStrategyResult > {
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

    Finally f1([this,result]() {
        this->maybe_report_result(this->shared_from_this(), result);
    });

    result->success = false;
    if (this->data->thesis.size() == 0) {
        return;
    }
    ParsingTree< SymTok, LabTok > thesis_pt = this->toolbox.parse_sentence(this->data->thesis.begin()+1, this->data->thesis.end(), this->toolbox.get_turnstile_alias());
    if (thesis_pt.label == LabTok{}) {
        return;
    }
    auto wff = wff_from_pt(thesis_pt, this->toolbox);
    //vector< ParsingTree< SymTok, LabTok > > hyps_pt;
    for (const auto &hyp : this->data->hypotheses) {
        if (hyp.size() == 0) {
            return;
        }
        ParsingTree< SymTok, LabTok > hyp_pt = this->toolbox.parse_sentence(hyp.begin()+1, hyp.end(), this->toolbox.get_turnstile_alias());
        if (hyp_pt.label == LabTok{}) {
            return;
        }
        //hyps_pt.push_back(hyp_pt);
        wff = Imp::create(wff_from_pt(hyp_pt, this->toolbox), wff);
        yield();
    }

    yield();

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
    default:
        return {};
    }
}
