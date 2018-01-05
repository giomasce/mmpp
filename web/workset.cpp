#include "workset.h"

#include "libs/json.h"

#include "reader.h"
#include "platform.h"
#include "jsonize.h"

using namespace std;
using namespace chrono;
using namespace nlohmann;

Workset::Workset() : thread_manager(make_unique< CoroutineThreadManager >(4)), step_backrefs(BackreferenceRegistry< Step, Workset >::create()), root_step(this->step_backrefs->make_instance())
{
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

json Workset::answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end)
{
    unique_lock< mutex > lock(this->global_mutex);
    assert_or_throw< SendError >(path_begin != path_end, 404);
    if (*path_begin == "load") {
        this->load_library(platform_get_resources_base() / "library.mm", platform_get_resources_base() / "library.mm.cache", "|-");
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
            const auto &executor = ass.get_proof_executor(*this->library, true);
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
                unique_lock< mutex > lock(this->global_mutex);
                size_t parent_id = safe_stoi(safe_at(post_data, "parent").value);
                size_t idx = safe_stoi(safe_at(post_data, "index").value);
                shared_ptr< Step > parent_step;
                try {
                    parent_step = this->step_backrefs->at(parent_id);
                } catch (out_of_range) {
                    throw SendError(404);
                }
                shared_ptr< Step > new_step = this->step_backrefs->make_instance();
                bool res = new_step->reparent(parent_step, idx);
                json ret = json::object();
                ret["success"] = res;
                if (res) {
                    ret["id"] = new_step->get_id();
                }
                return ret;
            });
        } else if (*path_begin == "list") {
            /*path_begin++;
            assert_or_throw< SendError >(path_begin == path_end, 404);
            json ret = { { "worksets", this->json_list_worksets() } };
            return ret;*/
        } else {
            size_t id = safe_stoi(*path_begin);
            path_begin++;
            shared_ptr< Step > step;
            try {
                step = this->step_backrefs->at(id);
            } catch (out_of_range) {
                throw SendError(404);
            }
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
    this->toolbox = make_unique< LibraryToolbox >(*this->library, turnstile, true, cache);
}

const string &Workset::get_name()
{
    return this->name;
}

void Workset::set_name(const string &name)
{
    this->name = name;
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
