#pragma once

#include <mutex>
#include <memory>
#include <unordered_map>

class Workset;

#include "web/web.h"
#include "mm/library.h"
#include "web/step.h"
#include "mm/toolbox.h"
#include "utils/threadmanager.h"

class Workset : public enable_create< Workset > {
public:
    // Emulate the behaviour of a map
    typedef size_t key_type;
    typedef std::shared_ptr< Step > mapped_type;

    nlohmann::json answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end);
    void load_library(boost::filesystem::path filename, boost::filesystem::path cache_filename, std::string turnstile);
    const std::string &get_name();
    void set_name(const std::string &name);
    const LibraryToolbox &get_toolbox() const;
    std::shared_ptr< Step > get_root_step() const;
    std::shared_ptr< Workset > destroy();

    void add_coroutine(std::weak_ptr<Coroutine> coro);
    void add_timed_coroutine(std::weak_ptr<Coroutine> coro, std::chrono::system_clock::duration wait_time);
    void add_to_queue(nlohmann::json data);
    //std::shared_ptr< BackreferenceRegistry< Step, Workset > > get_step_backrefs() const;
    std::shared_ptr< Step > get_step(size_t id);
    std::shared_ptr< Step > at(size_t id);
    bool destroy_step(size_t id);

    std::shared_ptr< Step > create_steps_from_dump(const nlohmann::json &dump);

protected:
    Workset(std::weak_ptr< Session > session);
    void init();

private:
    std::shared_ptr< Step > create_step(bool do_no_search);

    std::unique_ptr< ExtendedLibrary > library;
    std::unique_ptr< LibraryToolbox > toolbox;
    std::unique_ptr< CoroutineThreadManager > thread_manager;
    std::recursive_mutex global_mutex;
    std::string name;
    //std::shared_ptr< BackreferenceRegistry< Step, Workset > > step_backrefs;
    IdDistributor id_dist;
    std::unordered_map< size_t, std::shared_ptr< Step > > steps;
    std::shared_ptr< Step > root_step;
    std::weak_ptr< Session > session;

    std::mutex queue_mutex;
    std::condition_variable queue_variable;
    std::list< nlohmann::json > queue;
};
