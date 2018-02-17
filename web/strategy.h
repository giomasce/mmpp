#pragma once

#include <memory>

#include "libs/json.h"
#include "utils/utils.h"
#include "utils/threadmanager.h"
#include "mm/toolbox.h"
#include "mm/engine.h"

class StepStrategyCallback {
public:
    virtual bool prove(CreativeCheckpointedProofEngine< Sentence > &engine) = 0;
};

class StepStrategyResult {
public:
    virtual bool get_success() const = 0;
    virtual nlohmann::json get_web_json() const = 0;
    virtual bool prove(CreativeCheckpointedProofEngine< Sentence > &engine, const std::vector< std::shared_ptr< StepStrategyCallback > > &children) const = 0;
};

class StepStrategy;

class StrategyManager {
public:
    virtual void report_result(std::shared_ptr< StepStrategy > strategy, std::shared_ptr< StepStrategyResult > result) = 0;
};

class StepStrategy {
public:
    ~StepStrategy();
    virtual void operator()(Yield &yield) = 0;

protected:
    StepStrategy(std::weak_ptr< StrategyManager > manager, const Sentence &thesis, const std::vector< Sentence > &hypotheses, const LibraryToolbox &toolbox);
    void maybe_report_result(std::shared_ptr<StepStrategy> strategy, std::shared_ptr< StepStrategyResult > result);

    std::weak_ptr< StrategyManager > manager;
    Sentence thesis;
    std::vector< Sentence > hypotheses;
    const LibraryToolbox &toolbox;
};

class FailingStrategy : public StepStrategy, public enable_create< FailingStrategy > {
    using StepStrategy::StepStrategy;

public:
    void operator()(Yield &yield);
};

class UnificationStrategy : public StepStrategy, public enable_create< UnificationStrategy > {
    using StepStrategy::StepStrategy;

public:
    void operator()(Yield &yield);
};

template< typename... Args >
std::vector< std::shared_ptr< StepStrategy > > create_strategies(Args&&... args)
{
    return {
        //FailingStrategy::create(std::forward< Args >(args)...),
        UnificationStrategy::create(std::forward< Args >(args)...),
    };
}
