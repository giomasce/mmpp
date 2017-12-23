#ifndef STEP_H
#define STEP_H

#include <vector>
#include <memory>
#include <mutex>
#include <list>
#include <unordered_map>

#include "utils/utils.h"
#include "utils/backref_registry.h"
#include "toolbox.h"
#include "utils/threadmanager.h"

class Step;

#include "web/web.h"

#include "libs/json.h"

class StepOperationsListener {
public:
    virtual void after_adopting(std::shared_ptr< Step > step, size_t child_idx) {
        (void) step;
        (void) child_idx;
    }
    virtual void after_being_adopted(std::shared_ptr< Step > step, size_t child_idx) {
        (void) step;
        (void) child_idx;
    }
    virtual void before_orphaning(std::shared_ptr< Step > step, size_t child_idx) {
        (void) step;
        (void) child_idx;
    }
    virtual void before_being_orphaned(std::shared_ptr< Step > step, size_t child_idx) {
        (void) step;
        (void) child_idx;
    }
    virtual void after_new_sentence(std::shared_ptr< Step > step, const Sentence &old_sent) {
        (void) step;
        (void) old_sent;
    }
};

class StepComputation {
public:
    StepComputation(std::weak_ptr< Step > parent, const Sentence &thesis, const std::vector<Sentence> &hypotheses, const LibraryToolbox &toolbox);
    void operator()(Yield &yield);

private:
    std::weak_ptr< Step > parent;
    Sentence thesis;
    std::vector< Sentence > hypotheses;
    const LibraryToolbox &toolbox;

    std::atomic< bool > finished;
    bool success;
    std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > result;
};

class Step
{
public:
    static std::shared_ptr< Step > create(BackreferenceToken< Step, Workset > &&token) {
        auto pointer = std::make_shared< enable_make< Step > >(std::move(token));
        pointer->weak_this = pointer;
        return pointer;
    }
    size_t get_id();
    const std::vector< std::shared_ptr< Step > > &get_children();
    const Sentence &get_sentence();
    std::weak_ptr<Workset> get_workset();
    void set_sentence(const Sentence &sentence);
    std::shared_ptr<Step> get_parent() const;
    bool orphan();
    bool reparent(std::shared_ptr< Step > parent, size_t idx);
    void step_coroutine_finished();
    nlohmann::json answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end);
    void add_listener(const std::shared_ptr<StepOperationsListener> &listener);

protected:
    explicit Step(BackreferenceToken< Step, Workset > &&token);

private:
    void clean_listeners();
    void after_adopting(size_t child_idx);
    void after_being_adopted(size_t child_idx);
    void before_orphaning(size_t child_idx);
    void before_being_orphaned(size_t child_idx);
    void after_new_sentence(const Sentence &old_sent);
    void restart_coroutine();

    BackreferenceToken< Step, Workset > token;
    std::vector< std::shared_ptr< Step > > children;
    std::weak_ptr< Step > parent;
    std::recursive_mutex global_mutex;
    std::weak_ptr< Step > weak_this;
    std::list< std::weak_ptr< StepOperationsListener > > listeners;

    Sentence sentence;
    std::shared_ptr< StepComputation > last_comp;
    std::shared_ptr< Coroutine > last_coro;
};

#endif // STEP_H
