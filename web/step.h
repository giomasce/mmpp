#ifndef STEP_H
#define STEP_H

#include <vector>
#include <memory>
#include <mutex>
#include <list>
#include <unordered_map>

#include "utils/utils.h"
#include "utils/backref_registry.h"
#include "mm/toolbox.h"
#include "utils/threadmanager.h"

class Step;

#include "web/web.h"

#include "libs/json.h"

class StepOperationsListener {
public:
    virtual void before_adopting(std::shared_ptr< Step > step, size_t child_idx) {
        (void) step;
        (void) child_idx;
    }
    virtual void before_being_adopted(std::shared_ptr< Step > step, size_t child_idx) {
        (void) step;
        (void) child_idx;
    }
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
    virtual void after_orphaning(std::shared_ptr< Step > step, size_t child_idx) {
        (void) step;
        (void) child_idx;
    }
    virtual void after_being_orphaned(std::shared_ptr< Step > step, size_t child_idx) {
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
    bool get_success() const;
    const std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > &get_result() const;

private:
    std::weak_ptr< Step > parent;
    Sentence thesis;
    std::vector< Sentence > hypotheses;
    const LibraryToolbox &toolbox;

    std::atomic< bool > finished;
    bool success;
    std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > result;
};

class Step : public enable_create< Step >
{
public:
    size_t get_id();
    const std::vector<SafeWeakPtr<Step> > &get_children();
    const Sentence &get_sentence();
    std::weak_ptr<Workset> get_workset();
    void set_sentence(const Sentence &sentence);
    std::shared_ptr<Step> get_parent() const;
    bool destroy();
    bool orphan();
    bool reparent(std::shared_ptr< Step > parent, size_t idx);
    void step_coroutine_finished(StepComputation *step_comp);
    nlohmann::json answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end);
    void add_listener(const std::shared_ptr<StepOperationsListener> &listener);
    bool get_success();
    //std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > get_result();
    LabTok get_label();
    LabTok get_number();
    const std::vector< size_t > &get_permutation();
    const std::unordered_map< SymTok, std::vector< SymTok > > &get_subst_map();

protected:
    //explicit Step(BackreferenceToken< Step, Workset > &&token);
    explicit Step(size_t id, std::shared_ptr< Workset > workset);
    ~Step();

private:
    void clean_listeners();
    void before_adopting(size_t child_idx);
    void before_being_adopted(size_t child_idx);
    void after_adopting(size_t child_idx);
    void after_being_adopted(size_t child_idx);
    void before_orphaning(size_t child_idx);
    void before_being_orphaned(size_t child_idx);
    void after_orphaning(size_t child_idx);
    void after_being_orphaned(size_t child_idx);
    void after_new_sentence(const Sentence &old_sent);
    void restart_coroutine();

    bool reaches_by_parents(const Step &to);

    //BackreferenceToken< Step, Workset > token;
    size_t id;
    SafeWeakPtr< Workset > workset;
    std::vector< SafeWeakPtr< Step > > children;
    std::weak_ptr< Step > parent;
    std::recursive_mutex global_mutex;
    std::list< std::weak_ptr< StepOperationsListener > > listeners;

    Sentence sentence;
    std::shared_ptr< StepComputation > last_comp;
    std::shared_ptr< Coroutine > last_coro;

    /*LabTok label;
    std::vector< size_t > permutation;
    std::unordered_map< SymTok, std::vector< SymTok > > subst_map;*/
};

#endif // STEP_H
