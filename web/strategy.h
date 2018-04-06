#pragma once

#include <memory>

#include "libs/json.h"
#include "utils/utils.h"
#include "utils/threadmanager.h"
#include "mm/toolbox.h"
#include "mm/engine.h"

struct StepStrategyData {
    Sentence thesis;
    std::vector< Sentence > hypotheses;
    ParsingTree< SymTok, LabTok > pt_thesis;
    std::vector< ParsingTree< SymTok, LabTok > > pt_hypotheses;
    std::set< std::pair< SymTok, SymTok > > antidists;
    std::set< std::pair< LabTok, LabTok > > lab_antidists;
};

class StepStrategyCallback {
public:
    virtual bool prove() = 0;
};

class StepStrategyResult {
public:
    virtual bool get_success() const = 0;
    virtual nlohmann::json get_web_json() const = 0;
    virtual nlohmann::json get_dump_json() const = 0;
    virtual bool prove(CheckpointedProofEngine &engine, const std::vector< std::shared_ptr< StepStrategyCallback > > &children) const = 0;
};

class StepStrategy;

class StrategyManager {
public:
    virtual void report_result(std::shared_ptr< StepStrategy > strategy, std::shared_ptr< StepStrategyResult > result) = 0;
};

class StepStrategy {
public:
    virtual ~StepStrategy();
    virtual void operator()(Yielder &yield) = 0;

protected:
    StepStrategy(std::weak_ptr< StrategyManager > manager, std::shared_ptr< const StepStrategyData > data, const LibraryToolbox &toolbox);
    void maybe_report_result(std::shared_ptr<StepStrategy> strategy, std::shared_ptr< StepStrategyResult > result);

    std::weak_ptr< StrategyManager > manager;
    std::shared_ptr< const StepStrategyData > data;
    const LibraryToolbox &toolbox;
};

class FailingStrategy : public StepStrategy, public enable_create< FailingStrategy > {
    using StepStrategy::StepStrategy;

public:
    void operator()(Yielder &yield);
};

class UnificationStrategy : public StepStrategy, public enable_create< UnificationStrategy > {
    using StepStrategy::StepStrategy;

public:
    void operator()(Yielder &yield);
};

class WffStrategy : public StepStrategy, public enable_create< WffStrategy > {
public:
    enum SubStrategy {
        SUBSTRATEGY_WFF,
        SUBSTRATEGY_WFFSAT,
    };
    WffStrategy(std::weak_ptr< StrategyManager > manager, std::shared_ptr< const StepStrategyData > data, const LibraryToolbox &toolbox, SubStrategy substrategy);
    void operator()(Yielder &yield);
private:
    SubStrategy substrategy;
};

class UctStrategy : public StepStrategy, public enable_create< UctStrategy > {
    using StepStrategy::StepStrategy;

public:
    void operator()(Yielder &yield);
};

/*template< typename... Args >
std::vector< std::shared_ptr< StepStrategy > > create_strategies(Args&&... args)
{
    return {
        FailingStrategy::create(std::forward< Args >(args)...),
        UnificationStrategy::create(std::forward< Args >(args)...),
        WffStrategy::create(std::forward< Args >(args)...),
    };
}*/

std::vector< std::shared_ptr< StepStrategy > > create_strategies(unsigned priority, std::weak_ptr< StrategyManager > manager, std::shared_ptr< const StepStrategyData > data, const LibraryToolbox &toolbox);
