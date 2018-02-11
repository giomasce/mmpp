#include "step.h"

#include <boost/algorithm/string.hpp>

#include "web/jsonize.h"
#include "strategy.h"

using namespace std;
using namespace nlohmann;

#define LOG_STEP_OPS

/*Step::Step(BackreferenceToken<Step, Workset> &&token) : token(move(token))
{
}*/

void Step::clean_listeners()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    this->listeners.remove_if([](const auto &x) { return x.expired(); });
}

void Step::before_adopting(size_t child_idx) {
    (void) child_idx;
}

void Step::before_being_adopted(size_t child_idx) {
    (void) child_idx;
}

void Step::after_adopting(size_t child_idx) {
    (void) child_idx;
    this->restart_search();
}

void Step::after_being_adopted(size_t child_idx) {
    (void) child_idx;
}

void Step::before_orphaning(size_t child_idx) {
    (void) child_idx;
}

void Step::before_being_orphaned(size_t child_idx) {
    (void) child_idx;
}

void Step::after_orphaning(size_t child_idx) {
    (void) child_idx;
    this->restart_search();
}

void Step::after_being_orphaned(size_t child_idx) {
    (void) child_idx;
}

void Step::after_new_sentence(const Sentence &old_sent) {
    (void) old_sent;
    unique_lock< recursive_mutex > lock(this->global_mutex);
    this->restart_search();
    auto strong_parent = this->parent.lock();
    if (strong_parent) {
        auto workset = this->get_workset().lock();
        if (workset) {
            // Restart search from a coroutine to avoid deadlocks
            auto body = [strong_parent](Yield &yield) {
                (void) yield;
                strong_parent->restart_search();
            };
            auto shared_body = make_shared< decltype(body) >(body);
            workset->add_coroutine(make_auto_coroutine(shared_body));
        }
    }
}

void Step::restart_search()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    auto workset = this->get_workset().lock();
    if (!workset) {
        return;
    }
    if (this->do_not_search) {
        return;
    }

    this->active_strategies.clear();
    this->winning_strategy = NULL;

    vector< Sentence > hyps;
    for (const auto &child : this->get_children()) {
        hyps.push_back(child.lock()->get_sentence());
    }

    auto strategies = create_strategies(this->weak_from_this(), this->get_sentence(), hyps, workset->get_toolbox());
    for (const auto &strat : strategies) {
        auto coro = make_shared< Coroutine >(strat);
        workset->add_coroutine(coro);
        this->active_strategies.push_back(make_pair(strat, coro));
    }
    this->maybe_notify_update();
}

void Step::report_result(std::shared_ptr<StepStrategy> strategy, std::shared_ptr<StepStrategyResult> result)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    auto it = find_if(this->active_strategies.begin(), this->active_strategies.end(), [&strategy](const auto &x) { return x.first == strategy; });
    if (it == this->active_strategies.end()) {
        return;
    }
    if (result->get_success()) {
        this->active_strategies.clear();
        this->winning_strategy = result;
        this->maybe_notify_update();
    } else {
        this->active_strategies.erase(it);
        if (this->active_strategies.empty()) {
            this->maybe_notify_update();
        }
    }
}

bool Step::reaches_by_parents(const Step &to)
{
    vector< unique_lock< recursive_mutex > > locks;
    vector< shared_ptr< Step > > parents;
    locks.emplace_back(this->global_mutex);
    parents.push_back(this->shared_from_this());
    assert(parents.back());
    while (true) {
        auto new_parent = parents.back()->parent.lock();
        if (!new_parent) {
            return false;
        }
        if (new_parent.get() == &to) {
            return true;
        }
        locks.emplace_back(new_parent->global_mutex);
        parents.push_back(new_parent);
    }
}

size_t Step::get_id()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    //return this->token.get_id();
    return this->id;
}

const std::vector<SafeWeakPtr<Step> > &Step::get_children()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    return this->children;
}

const Sentence &Step::get_sentence()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    return this->sentence;
}

std::weak_ptr<Workset> Step::get_workset()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    //return this->token.get_main();
    return this->workset;
}

void Step::set_sentence(const Sentence &sentence)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    auto strong_this = this->shared_from_this();
    assert(strong_this != NULL);
    Sentence old_sentence = this->sentence;
    this->sentence = sentence;
    this->clean_listeners();
    this->after_new_sentence(old_sentence);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->after_new_sentence(strong_this, old_sentence);
        }
    }
}

std::shared_ptr<Step> Step::get_parent() const
{
    return this->parent.lock();
}

bool Step::destroy()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    // Take a strong reference to this, so that the object is not deallocated while this method is still running
    auto strong_this = this->shared_from_this();
    auto strong_parent = this->get_parent();
    if (strong_parent) {
        return false;
    }
    if (!this->children.empty()) {
        return false;
    }
    auto workset = this->get_workset().lock();
    return workset->destroy_step(this->get_id());
}

bool Step::orphan()
{
    // We assume that a global lock is held by the Workset, so here we can lock both
    // this and the parent without fear of deadlocks; we still need to lock both,
    // because answer_api1() has already released the Workset's lock
    unique_lock< recursive_mutex > lock(this->global_mutex);
    auto strong_this = this->shared_from_this();
    auto strong_parent = this->get_parent();
    if (strong_parent == NULL) {
        return false;
    }
    unique_lock< recursive_mutex > parent_lock(strong_parent->global_mutex);
    auto &pchildren = strong_parent->children;
    auto it = find_if(pchildren.begin(), pchildren.end(), [this](const SafeWeakPtr< Step > &s)->bool { return s.lock().get() == this; });
    assert(it != pchildren.end());
    size_t idx = it - pchildren.begin();

    // Clean listeners
    strong_parent->clean_listeners();
    this->clean_listeners();

    // Call before listeners
    strong_parent->before_orphaning(idx);
    for (auto &listener : strong_parent->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->before_orphaning(strong_parent, idx);
        }
    }
    this->before_being_orphaned(idx);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->before_being_orphaned(strong_this, idx);
        }
    }

    // Actual orphaning
#ifdef LOG_STEP_OPS
    cerr << "Orphaning step " << this->get_id() << " from step " << strong_parent->get_id() << endl;
#endif
    this->parent = std::weak_ptr< Step >();
    pchildren.erase(it);

    // Call after listeners
    strong_parent->after_orphaning(idx);
    for (auto &listener : strong_parent->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->after_orphaning(strong_parent, idx);
        }
    }
    this->after_being_orphaned(idx);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->after_being_orphaned(strong_this, idx);
        }
    }

    return true;
}

bool Step::reparent(std::shared_ptr<Step> parent, size_t idx)
{
    // See comments in orphan() about locking
    unique_lock< recursive_mutex > lock(this->global_mutex);
    auto strong_this = this->shared_from_this();
    assert(strong_this != NULL);
    if (!this->parent.expired()) {
        return false;
    }
    unique_lock< recursive_mutex > parent_lock(parent->global_mutex);
    auto &pchildren = parent->children;
    if (idx > pchildren.size()) {
        return false;
    }
    if (parent->reaches_by_parents(*this)) {
        return false;
    }

    // Clean listeners
    parent->clean_listeners();
    this->clean_listeners();

    // Call before listeners
    parent->before_adopting(idx);
    for (auto &listener : parent->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->before_adopting(parent, idx);
        }
    }
    this->before_being_adopted(idx);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->before_being_adopted(strong_this, idx);
        }
    }

    // Actual reparenting
    this->parent = parent;
    pchildren.insert(pchildren.begin() + idx, strong_this);
#ifdef LOG_STEP_OPS
    cerr << "Reparenting step " << this->get_id() << " under step " << parent->get_id() << endl;
#endif

    // Call after listeners
    parent->after_adopting(idx);
    for (auto &listener : parent->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->after_adopting(parent, idx);
        }
    }
    this->after_being_adopted(idx);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->after_being_adopted(strong_this, idx);
        }
    }

    return true;
}

nlohmann::json Step::answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    assert_or_throw< SendError >(path_begin != path_end, 404);
    if (*path_begin == "get") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        return jsonize(*this);
    } else if (*path_begin == "set_sentence") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        assert_or_throw< SendError >(cb.get_method() == "POST", 405);
        throw WaitForPost([self=this->shared_from_this()] (const auto &post_data) {
            unique_lock< recursive_mutex > lock(self->global_mutex);
            string sent_str = safe_at(post_data, "sentence").value;
            vector< string> toks;
            boost::split(toks, sent_str, boost::is_any_of(" "));
            Sentence sent;
            for (const auto &x : toks) {
                sent.push_back(safe_stoi(x));
            }
            self->set_sentence(sent);
            json ret = json::object();
            ret["success"] = true;
            return ret;
        });
    } else if (*path_begin == "reparent") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        assert_or_throw< SendError >(cb.get_method() == "POST", 405);
        throw WaitForPost([self=this->shared_from_this()] (const auto &post_data) {
            unique_lock< recursive_mutex > lock(self->global_mutex);
            size_t parent_id = safe_stoi(safe_at(post_data, "parent").value);
            size_t idx = safe_stoi(safe_at(post_data, "index").value);
            shared_ptr< Step > parent_step;
            auto strong_workset = self->get_workset().lock();
            assert_or_throw< SendError >(static_cast< bool >(strong_workset), 404);
            parent_step = safe_at(*strong_workset, parent_id);
            bool res = self->reparent(parent_step, idx);
            json ret = json::object();
            ret["success"] = res;
            return ret;
        });
    } else if (*path_begin == "orphan") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        assert_or_throw< SendError >(cb.get_method() == "POST", 405);
        throw WaitForPost([self=this->shared_from_this()] (const auto &post_data) {
            (void) post_data;
            unique_lock< recursive_mutex > lock(self->global_mutex);
            bool res = self->orphan();
            json ret = json::object();
            ret["success"] = res;
            return ret;
        });
    } else if (*path_begin == "destroy") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        assert_or_throw< SendError >(cb.get_method() == "POST", 405);
        throw WaitForPost([self=this->shared_from_this()] (const auto &post_data) {
            (void) post_data;
            unique_lock< recursive_mutex > lock(self->global_mutex);
            bool res = self->destroy();
            json ret = json::object();
            ret["success"] = res;
            return ret;
        });
    } else if (*path_begin == "prove") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        assert_or_throw< SendError >(cb.get_method() == "POST", 405);
        throw WaitForPost([self=this->shared_from_this()] (const auto &post_data) {
            (void) post_data;
            unique_lock< recursive_mutex > lock(self->global_mutex);
            auto strong_workset = self->get_workset().lock();
            assert_or_throw< SendError >(static_cast< bool >(strong_workset), 404);
            const auto &toolbox = strong_workset->get_toolbox();
            ExtendedProofEngine< Sentence > engine(toolbox, true);
            bool res = self->prove(engine);
            json ret = json::object();
            ret["success"] = res;
            ret["uncompressed_proof"] = toolbox.print_proof(engine.get_proof_labels()).to_string();
            return ret;
        });
    }
    throw SendError(404);
}

void Step::add_listener(const std::shared_ptr<StepOperationsListener> &listener)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    this->listeners.push_back(listener);
}

void Step::maybe_notify_update()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    // Push an event to queue if the workset exists
    auto strong_workset = this->get_workset().lock();
    if (!strong_workset) {
        return;
    }
    json ret = json::object();
    ret["event"] = "step_updated";
    ret["step_id"] = this->get_id();
    strong_workset->add_to_queue(ret);
}

bool Step::is_searching()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    return !this->active_strategies.empty();
}

std::shared_ptr<const StepStrategyResult> Step::get_result()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    return this->winning_strategy;
}

bool Step::prove(ExtendedProofEngine<Sentence> &engine)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    if (this->winning_strategy) {
        vector< shared_ptr< StepStrategyCallback > > children_steps;
        for (const auto &x : this->children) {
            children_steps.push_back(x.lock());
        }
        return this->winning_strategy->prove(engine, children_steps);
    } else {
        engine.process_new_hypothesis(this->sentence);
        return true;
    }
}

Step::Step(size_t id, std::shared_ptr<Workset> workset, bool do_not_search) : id(id), workset(workset), do_not_search(do_not_search)
{
#ifdef LOG_STEP_OPS
    cerr << "Creating step with id " << id << endl;
#endif
}

void Step::init()
{
    this->restart_search();
}

Step::~Step()
{
#ifdef LOG_STEP_OPS
    cerr << "Destroying step with id " << id << endl;
#endif
}
