#include "workset.h"

#include "libs/json.h"

#include "mm/reader.h"
#include "mm/engine.h"
#include "mm/proof.h"
#include "platform.h"
#include "jsonize.h"

using namespace std;
using namespace chrono;
using namespace nlohmann;

Workset::Workset(std::weak_ptr<Session> session) : thread_manager(make_unique< CoroutineThreadManager >(4)) /*, step_backrefs(BackreferenceRegistry< Step, Workset >::create()) */, session(session)
{
}

std::shared_ptr<Step> Workset::create_step(bool do_no_search)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    shared_ptr< Step > new_step = Step::create(this->id_dist.get_id(), this->shared_from_this(), do_no_search);
    this->steps[new_step->get_id()] = new_step;
    return new_step;
}

template< typename TokType >
std::vector< std::string > map_to_vect(const std::unordered_map< TokType, std::string > &m) {
    std::vector< std::string > ret;
    ret.resize(m.size()+1);
    for (auto &i : m) {
        ret[i.first] = i.second;
    }
    return ret;
}

void Workset::init() {
    // We cannot create the root step before create() has returned (i.e., in the constructor)
    this->root_step = this->create_step(true);
    //pointer->step_backrefs->set_main(pointer);
}

json Workset::answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    assert_or_throw< SendError >(path_begin != path_end, 404);
    if (*path_begin == "load") {
        this->load_library(platform_get_resources_base() / "set.mm", platform_get_resources_base() / "set.mm.cache", "|-");
        json ret = { { "status", "ok" } };
        return ret;
    } else if (*path_begin == "get_context") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        json ret;
        ret["name"] = this->get_name();
        ret["root_step_id"] = this->root_step->get_id();
        if (this->library == NULL) {
            ret["status"] = "unloaded";
            return ret;
        }
        ret["status"] = "loaded";
        ret["symbols"] = map_to_vect(this->library->get_symbols());
        ret["labels"] = map_to_vect(this->library->get_labels());
        const auto &addendum = this->library->get_addendum();
        ret["addendum"] = jsonize(addendum);
        ret["max_number"] = this->library->get_max_number();
        return ret;
    } else if (*path_begin == "get_sentence") {
        path_begin++;
        assert_or_throw< SendError >(path_begin != path_end, 404);
        int tok = safe_stoi(*path_begin);
        try {
            const Sentence &sent = this->library->get_sentence(tok);
            json ret;
            ret["sentence"] = sent;
            return ret;
        } catch (out_of_range) {
            throw SendError(404);
        }
    } else if (*path_begin == "get_assertion") {
        path_begin++;
        assert_or_throw< SendError >(path_begin != path_end, 404);
        int tok = safe_stoi(*path_begin);
        try {
            const Assertion &ass = this->library->get_assertion(tok);
            assert_or_throw< SendError >(ass.is_valid(), 404);
            json ret;
            ret["assertion"] = jsonize(ass);
            return ret;
        } catch (out_of_range) {
            throw SendError(404);
        }
    } else if (*path_begin == "destroy") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        assert_or_throw< SendError >(cb.get_method() == "POST", 405);
        throw WaitForPost([self=this->shared_from_this()] (const auto &post_data) {
            (void) post_data;
            auto res = self->destroy();
            json ret = json::object();
            ret["success"] = static_cast< bool >(res);
            return ret;
        });
    } else if (*path_begin == "queue") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        assert_or_throw< SendError >(cb.get_method() == "POST", 405);
        throw WaitForPost([this] (const auto &post_data) {
            (void) post_data;
            unique_lock< mutex > queue_lock(this->queue_mutex);
            // Queue automatically returns after some timeout
            auto timeout = steady_clock::now() + 1s;
            while (this->queue.empty()) {
                auto reason = this->queue_variable.wait_until(queue_lock, timeout);
                if (reason == cv_status::timeout) {
                    json ret = json::object();
                    ret["event"] = "nothing";
                    return ret;
                }
            }
            auto ret = this->queue.front();
            this->queue.pop_front();
            return ret;
        });
    } else if (*path_begin == "get_proof_tree") {
        path_begin++;
        assert_or_throw< SendError >(path_begin != path_end, 404);
        int tok = safe_stoi(*path_begin);
        try {
            const Assertion &ass = this->library->get_assertion(tok);
            assert_or_throw< SendError >(ass.is_valid(), 404);
            const auto &executor = ass.get_proof_executor< Sentence >(*this->library, true);
            executor->execute();
            const auto &proof_tree = executor->get_proof_tree();
            json ret;
            ret["proof_tree"] = jsonize(proof_tree);
            return ret;
        } catch (out_of_range) {
            throw SendError(404);
        }
    } else if (*path_begin == "step") {
        path_begin++;
        assert_or_throw< SendError >(path_begin != path_end, 404);
        if (*path_begin == "create") {
            path_begin++;
            assert_or_throw< SendError >(path_begin == path_end, 404);
            assert_or_throw< SendError >(cb.get_method() == "POST", 405);
            throw WaitForPost([this] (const auto &post_data) {
                (void) post_data;
                unique_lock< recursive_mutex > lock(this->global_mutex);
                auto new_step = this->create_step(false);
                json ret = json::object();
                ret["id"] = new_step->get_id();
                return ret;
            });
        } else if (*path_begin == "create_from_dump") {
            path_begin++;
            assert_or_throw< SendError >(path_begin == path_end, 404);
            assert_or_throw< SendError >(cb.get_method() == "POST", 405);
            throw WaitForPost([this] (const auto &post_data) {
                unique_lock< recursive_mutex > lock(this->global_mutex);
                const auto dump_str = safe_at(post_data, "dump").value;
                json dump;
                try {
                    dump = json::parse(dump_str);
                } catch (nlohmann::detail::exception) {
                    throw SendError(422);
                }
                auto new_step = this->create_steps_from_dump(dump);
                json ret = json::object();
                ret["id"] = new_step->get_id();
                return ret;
            });
        } else {
            size_t id = safe_stoi(*path_begin);
            path_begin++;
            shared_ptr< Step > step = safe_at(this->steps, id);
            return step->answer_api1(cb, path_begin, path_end);
        }
        try {
            return jsonize(*this->root_step);
        } catch (out_of_range) {
            throw SendError(404);
        }
    }
    throw SendError(404);
}

void Workset::load_library(boost::filesystem::path filename, boost::filesystem::path cache_filename, std::string turnstile)
{
    FileTokenizer ft(filename);
    Reader p(ft, false, true);
    p.run();
    this->library = make_unique< LibraryImpl >(p.get_library());
    shared_ptr< ToolboxCache > cache = make_shared< FileToolboxCache >(cache_filename);
    this->toolbox = make_unique< LibraryToolbox >(*this->library, turnstile, cache);
}

const string &Workset::get_name()
{
    return this->name;
}

void Workset::set_name(const string &name)
{
    this->name = name;
}

const LibraryToolbox &Workset::get_toolbox() const {
    return *this->toolbox;
}

std::shared_ptr<Step> Workset::get_root_step() const {
    return this->root_step;
}

std::shared_ptr<Workset> Workset::destroy()
{
    auto strong_this = this->shared_from_this();
    auto strong_session = this->session.lock();
    if (strong_session) {
        strong_session->destroy_workset(strong_this);
    }
    return strong_this;
}

void Workset::add_coroutine(std::weak_ptr<Coroutine> coro)
{
    this->thread_manager->add_coroutine(coro);
}

void Workset::add_to_queue(json data)
{
    unique_lock< mutex > lock(this->queue_mutex);
    this->queue.push_back(data);
    this->queue_variable.notify_one();
}

std::shared_ptr<Step> Workset::get_step(size_t id)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    return this->steps.at(id);
}

std::shared_ptr<Step> Workset::at(size_t id)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    return this->get_step(id);
}

bool Workset::destroy_step(size_t id)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    if (this->root_step->get_id() == id) {
        return false;
    } else {
        this->steps.erase(id);
        return true;
    }
}

std::shared_ptr<Step> Workset::create_steps_from_dump(const json &dump)
{
    unique_lock< recursive_mutex > lock(this->global_mutex);
    auto step = this->create_step(false);
    step->load_dump(dump);
    size_t idx = 0;
    for (const auto &child : dump["children"]) {
        auto step_child = this->create_steps_from_dump(child);
        step_child->reparent(step, idx);
        idx++;
    }
    return step;
}

/*std::shared_ptr<BackreferenceRegistry<Step, Workset> > Workset::get_step_backrefs() const
{
    return this->step_backrefs;
}*/
