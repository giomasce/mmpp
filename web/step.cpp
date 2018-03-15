#include "step.h"

#include <boost/algorithm/string.hpp>

#include "web/jsonize.h"
#include "strategy.h"
#include "mm/proof.h"

using namespace nlohmann;

//#define LOG_STEP_OPS

/*Step::Step(BackreferenceToken<Step, Workset> &&token) : token(move(token))
{
}*/

void Step::clean_listeners()
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
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
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    this->restart_search();
    auto strong_parent = this->parent.lock();
    if (strong_parent) {
        auto workset = this->get_workset().lock();
        if (workset) {
            // Restart search from a coroutine to avoid deadlocks
            auto body = [strong_parent](Yielder &yield) {
                (void) yield;
                strong_parent->restart_search();
            };
            auto shared_body = std::make_shared< decltype(body) >(body);
            workset->add_coroutine(make_auto_coroutine(shared_body));
        }
    }
}

void Step::restart_search()
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    auto workset = this->get_workset().lock();
    if (!workset) {
        return;
    }
    if (this->do_not_search) {
        return;
    }

    this->active_strategies.clear();
    this->winning_strategy = nullptr;

    std::vector< Sentence > hyps;
    for (const auto &child : this->get_children()) {
        hyps.push_back(child.lock()->get_sentence());
    }

    auto strategies = create_strategies(this->weak_from_this(), this->get_sentence(), hyps, workset->get_toolbox());
    for (const auto &strat : strategies) {
        auto coro = std::make_shared< Coroutine >(strat);
        workset->add_coroutine(coro);
        this->active_strategies.push_back(std::make_pair(strat, coro));
    }
    this->maybe_notify_update();
}

void Step::report_result(std::shared_ptr<StepStrategy> strategy, std::shared_ptr<StepStrategyResult> result)
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
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
    std::vector< std::unique_lock< std::recursive_mutex > > locks;
    std::vector< std::shared_ptr< Step > > parents;
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
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    //return this->token.get_id();
    return this->id;
}

const std::vector<SafeWeakPtr<Step> > &Step::get_children()
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    return this->children;
}

const Sentence &Step::get_sentence()
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    return this->sentence;
}

const ParsingTree<SymTok, LabTok> &Step::get_parsing_tree()
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    return this->parsing_tree;
}

std::weak_ptr<Workset> Step::get_workset()
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    //return this->token.get_main();
    return this->workset;
}

void Step::set_sentence(const Sentence &sentence)
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    auto strong_this = this->shared_from_this();
    assert(strong_this != nullptr);
    Sentence old_sentence = this->sentence;
    this->sentence = sentence;
    if (!sentence.empty()) {
        auto &tb = this->get_workset().lock()->get_toolbox();
        auto type_it = tb.get_parsing_addendum().get_syntax().find(sentence[0]);
        if (type_it != tb.get_parsing_addendum().get_syntax().end()) {
            this->parsing_tree = tb.parse_sentence(sentence.begin()+1, sentence.end(), type_it->second);
        } else {
            this->parsing_tree = tb.parse_sentence(sentence.begin()+1, sentence.end(), sentence[0]);
        }
    } else {
        this->parsing_tree = {};
    }
    this->clean_listeners();
    this->after_new_sentence(old_sentence);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != nullptr) {
            locked->after_new_sentence(strong_this, old_sentence);
        }
    }
}

std::shared_ptr<Step> Step::get_parent() const
{
    return this->parent.lock();
}

std::shared_ptr< Step > Step::destroy()
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    // Take a strong reference to this and return it, so that the object is not deallocated while this method is still running
    auto strong_this = this->shared_from_this();
    auto strong_parent = this->get_parent();
    if (strong_parent) {
        return nullptr;
    }
    if (!this->children.empty()) {
        return nullptr;
    }
    auto workset = this->get_workset().lock();
    workset->destroy_step(this->get_id());
    return strong_this;
}

bool Step::orphan()
{
    // We assume that a global lock is held by the Workset, so here we can lock both
    // this and the parent without fear of deadlocks; we still need to lock both,
    // because answer_api1() has already released the Workset's lock
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    auto strong_this = this->shared_from_this();
    auto strong_parent = this->get_parent();
    if (strong_parent == nullptr) {
        return false;
    }
    std::unique_lock< std::recursive_mutex > parent_lock(strong_parent->global_mutex);
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
        if (locked != nullptr) {
            locked->before_orphaning(strong_parent, idx);
        }
    }
    this->before_being_orphaned(idx);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != nullptr) {
            locked->before_being_orphaned(strong_this, idx);
        }
    }

    // Actual orphaning
#ifdef LOG_STEP_OPS
    std::cerr << "Orphaning step " << this->get_id() << " from step " << strong_parent->get_id() << std::endl;
#endif
    this->parent = std::weak_ptr< Step >();
    pchildren.erase(it);

    // Call after listeners
    strong_parent->after_orphaning(idx);
    for (auto &listener : strong_parent->listeners) {
        auto locked = listener.lock();
        if (locked != nullptr) {
            locked->after_orphaning(strong_parent, idx);
        }
    }
    this->after_being_orphaned(idx);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != nullptr) {
            locked->after_being_orphaned(strong_this, idx);
        }
    }

    return true;
}

bool Step::reparent(std::shared_ptr<Step> parent, size_t idx)
{
    // See comments in orphan() about locking
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    auto strong_this = this->shared_from_this();
    assert(strong_this != nullptr);
    if (!this->parent.expired()) {
        return false;
    }
    std::unique_lock< std::recursive_mutex > parent_lock(parent->global_mutex);
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
        if (locked != nullptr) {
            locked->before_adopting(parent, idx);
        }
    }
    this->before_being_adopted(idx);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != nullptr) {
            locked->before_being_adopted(strong_this, idx);
        }
    }

    // Actual reparenting
    this->parent = parent;
    pchildren.insert(pchildren.begin() + idx, strong_this);
#ifdef LOG_STEP_OPS
    std::cerr << "Reparenting step " << this->get_id() << " under step " << parent->get_id() << std::endl;
#endif

    // Call after listeners
    parent->after_adopting(idx);
    for (auto &listener : parent->listeners) {
        auto locked = listener.lock();
        if (locked != nullptr) {
            locked->after_adopting(parent, idx);
        }
    }
    this->after_being_adopted(idx);
    for (auto &listener : this->listeners) {
        auto locked = listener.lock();
        if (locked != nullptr) {
            locked->after_being_adopted(strong_this, idx);
        }
    }

    return true;
}

nlohmann::json Step::answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end)
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    assert_or_throw< SendError >(path_begin != path_end, 404);
    if (*path_begin == "get") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        return jsonize(*this);
    } else if (*path_begin == "dump") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        return this->dump();
    } else if (*path_begin == "set_sentence") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        assert_or_throw< SendError >(cb.get_method() == "POST", 405);
        throw WaitForPost([self=this->shared_from_this()] (const auto &post_data) {
            std::unique_lock< std::recursive_mutex > lock(self->global_mutex);
            std::string sent_str = safe_at(post_data, "sentence").value;
            std::vector< std::string> toks;
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
            std::unique_lock< std::recursive_mutex > lock(self->global_mutex);
            size_t parent_id = safe_stoi(safe_at(post_data, "parent").value);
            size_t idx = safe_stoi(safe_at(post_data, "index").value);
            std::shared_ptr< Step > parent_step;
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
            std::unique_lock< std::recursive_mutex > lock(self->global_mutex);
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
            std::unique_lock< std::recursive_mutex > lock(self->global_mutex);
            auto res = self->destroy();
            json ret = json::object();
            ret["success"] = static_cast< bool >(res);
            return ret;
        });
    } else if (*path_begin == "prove") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        assert_or_throw< SendError >(cb.get_method() == "POST", 405);
        throw WaitForPost([self=this->shared_from_this()] (const auto &post_data) {
            (void) post_data;
            std::unique_lock< std::recursive_mutex > lock(self->global_mutex);
            auto strong_workset = self->get_workset().lock();
            assert_or_throw< SendError >(static_cast< bool >(strong_workset), 404);
            const LibraryToolbox &toolbox = strong_workset->get_toolbox();
            CreativeProofEngineImpl< Sentence > engine(toolbox, true);
            bool res = self->prove(engine);
            json ret = json::object();
            ret["success"] = res;
            if (!res) {
                return ret;
            }

            std::ostringstream buf;
            const auto &thesis = engine.get_stack().back();
            const auto &hyps = engine.get_new_hypotheses();
            std::vector< LabTok > float_hyps;
            std::vector< LabTok > ess_hyps;
            std::set< SymTok > vars;
            collect_variables(thesis, toolbox.get_standard_is_var_sym(), vars);
            for (const auto &hyp : hyps) {
                buf << toolbox.resolve_label(hyp.first) << " $e " << toolbox.print_sentence(hyp.second) << " $." << std::endl;
                collect_variables(hyp.second, toolbox.get_standard_is_var_sym(), vars);
                ess_hyps.push_back(hyp.first);
            }
            for (const auto var : vars) {
                float_hyps.push_back(toolbox.get_var_sym_to_lab(var));
            }
            // Sorting floating hypotheses by their label shoud give the expected order in the Assertion
            std::sort(float_hyps.begin(), float_hyps.end());
            buf << "thesis $p " << toolbox.print_sentence(thesis) << " $=" << std::endl;
            UncompressedProof uncomp_proof(engine.get_proof_labels());
            Assertion dummy_ass(float_hyps, ess_hyps);
            auto uncomp_op = uncomp_proof.get_operator(toolbox, dummy_ass);
            for (const auto &hyp : hyps) {
                uncomp_op->set_new_hypothesis(hyp.first, hyp.second);
            }
            auto comp_proof = uncomp_op->compress(ProofOperator::CS_BACKREFS_ON_IDENTICAL_SENTENCE);
            buf << toolbox.print_proof(comp_proof) << std::endl;
            buf << "$." << std::endl;
            ret["proof"] = buf.str();

            return ret;
        });
    }
    throw SendError(404);
}

json Step::dump()
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    json ret = json::object();
    ret["sentence"] = this->sentence;
    ret["children"] = json::array();
    for (const auto &child : this->get_children()) {
        ret["children"].push_back(child.lock()->dump());
    }
    if (this->winning_strategy) {
        ret["strategy"] = this->winning_strategy->get_dump_json();
    }
    return ret;
}

void Step::load_dump(const json &dump)
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    Sentence new_sent;
    for (SymTok x : dump["sentence"]) {
        new_sent.push_back(x);
    }
    this->set_sentence(new_sent);
}

void Step::add_listener(const std::shared_ptr<StepOperationsListener> &listener)
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    this->listeners.push_back(listener);
}

void Step::maybe_notify_update()
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
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
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    return !this->active_strategies.empty();
}

std::shared_ptr<const StepStrategyResult> Step::get_result()
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    return this->winning_strategy;
}

struct StepStrategyCallbackImpl final : public StepStrategyCallback {
    StepStrategyCallbackImpl(std::shared_ptr< Step > step, CreativeCheckpointedProofEngine< Sentence > &engine) : step(step), engine(engine) {}

    bool prove() {
        return this->step->prove(engine);
    }

    std::shared_ptr< Step > step;
    CreativeCheckpointedProofEngine< Sentence > &engine;
};

bool Step::prove(CreativeCheckpointedProofEngine<Sentence> &engine)
{
    std::unique_lock< std::recursive_mutex > lock(this->global_mutex);
    if (this->winning_strategy) {
        std::vector< std::shared_ptr< StepStrategyCallback > > children_steps;
        for (const auto &x : this->children) {
            children_steps.push_back(std::make_shared< StepStrategyCallbackImpl >(x.lock(), engine));
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
    std::cerr << "Creating step with id " << id << std::endl;
#endif
}

void Step::init()
{
    this->restart_search();
}

Step::~Step()
{
#ifdef LOG_STEP_OPS
    std::cerr << "Destroying step with id " << id << std::endl;
#endif
}
