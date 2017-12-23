#include "step.h"

#include <boost/algorithm/string.hpp>

#include "web/jsonize.h"

using namespace std;
using namespace nlohmann;

Step::Step(BackreferenceToken<Step, Workset> &&token) : token(move(token))
{
}

void Step::clean_listeners()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    this->listeners.remove_if([](const auto &x) { return x.expired(); });
}

void Step::after_adopting(size_t child_idx) {
    (void) child_idx;
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

void Step::after_new_sentence(const Sentence &old_sent) {
    (void) old_sent;
}

void Step::restart_coroutine()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    vector< Sentence > hyps;
    for (const auto &child : this->get_children()) {
        hyps.push_back(child->get_sentence());
    }
    auto workset = this->get_workset().lock();
    if (workset != NULL) {
        auto comp = make_shared< StepComputation >(this->weak_this, this->get_sentence(), hyps, workset->get_toolbox());
    }
}

size_t Step::get_id()
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    return this->token.get_id();
}

const std::vector<std::shared_ptr<Step> > &Step::get_children()
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
    return this->token.get_main();
}

void Step::set_sentence(const Sentence &sentence)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    auto strong_this = this->weak_this.lock();
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

bool Step::orphan()
{
    // We assume that a global lock is held by the Workset, so here we can lock both
    // this and the parent without fear of deadlocks; we still need to lock both,
    // because answer_api1() has already released the Workset's lock
    unique_lock< recursive_mutex > lock(this->global_mutex);
    auto strong_this = this->weak_this.lock();
    assert(strong_this != NULL);
    shared_ptr< Step > strong_parent = this->get_parent();
    if (strong_parent == NULL) {
        return false;
    }
    unique_lock< recursive_mutex > parent_lock(strong_parent->global_mutex);
    auto &pchildren = strong_parent->children;
    auto it = find_if(pchildren.begin(), pchildren.end(), [this](const shared_ptr< Step > &s)->bool { return s.get() == this; });
    assert(it != pchildren.end());

    // Call listeners
    size_t idx = it - pchildren.begin();
    strong_parent->clean_listeners();
    strong_parent->before_orphaning(idx);
    for (auto &listener : strong_parent->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->before_orphaning(strong_parent, idx);
        }
    }
    this->clean_listeners();
    this->before_being_orphaned(idx);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->before_being_orphaned(strong_this, idx);
        }
    }

    // Actual orphaning
    this->parent = weak_ptr< Step >();
    pchildren.erase(it);
    return true;
}

bool Step::reparent(std::shared_ptr<Step> parent, size_t idx)
{
    // See comments in orphan() about locking
    unique_lock< recursive_mutex > lock(this->global_mutex);
    auto strong_this = this->weak_this.lock();
    assert(strong_this != NULL);
    if (!this->parent.expired()) {
        return false;
    }
    unique_lock< recursive_mutex > parent_lock(parent->global_mutex);
    auto &pchildren = parent->children;
    if (idx > pchildren.size()) {
        return false;
    }

    // Actual reparenting
    this->parent = parent;
    pchildren.insert(pchildren.begin() + idx, strong_this);

    // Call listeners
    parent->clean_listeners();
    parent->after_adopting(idx);
    for (auto &listener : parent->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->after_adopting(parent, idx);
        }
    }
    this->clean_listeners();
    this->after_being_adopted(idx);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != NULL) {
            locked->after_being_adopted(strong_this, idx);
        }
    }

    return true;
}

void Step::step_coroutine_finished()
{
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
        throw WaitForPost([this] (const auto &post_data) {
            unique_lock< recursive_mutex > lock(this->global_mutex);
            string sent_str = safe_at(post_data, "sentence").value;
            vector< string> toks;
            boost::split(toks, sent_str, boost::is_any_of(" "));
            Sentence sent;
            for (const auto &x : toks) {
                sent.push_back(safe_stoi(x));
            }
            this->set_sentence(sent);
            json ret = json::object();
            ret["success"] = true;
            return ret;
        });
    }
    throw SendError(404);
}

void Step::add_listener(const std::shared_ptr<StepOperationsListener> &listener)
{
    this->listeners.push_back(listener);
}

StepComputation::StepComputation(std::weak_ptr< Step > parent, const Sentence &thesis, const std::vector<Sentence> &hypotheses, const LibraryToolbox &toolbox)
    : parent(parent), thesis(thesis), hypotheses(hypotheses), toolbox(toolbox), finished(false), success(false)
{
}

void StepComputation::operator()(Yield &yield)
{
    (void) yield;

    auto res = this->toolbox.unify_assertion(this->hypotheses, this->thesis);
    if (!res.empty()) {
        this->success = true;
        this->result = res[0];
    }

    // Set termination and signal parent
    this->finished = true;
    auto strong_parent = this->parent.lock();
    if (strong_parent != NULL) {
        strong_parent->step_coroutine_finished();
    }
}
