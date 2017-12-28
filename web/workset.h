#ifndef WORKSET_H
#define WORKSET_H

#include <mutex>
#include <memory>

class Workset;

#include "web/web.h"
#include "library.h"
#include "web/step.h"
#include "toolbox.h"
#include "utils/threadmanager.h"

class Workset {
public:
    static std::shared_ptr< Workset > create() {
        auto pointer = std::make_shared< enable_make< Workset > >();
        pointer->step_backrefs->set_main(pointer);
        return pointer;
    }
    nlohmann::json answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end);
    void load_library(boost::filesystem::path filename, boost::filesystem::path cache_filename, std::string turnstile);
    const std::string &get_name();
    void set_name(const std::string &name);
    const LibraryToolbox &get_toolbox() const {
        return *this->toolbox;
    }

    void add_coroutine(std::weak_ptr<Coroutine> coro);
    void add_to_queue(nlohmann::json data);

protected:
    Workset();

private:
    std::unique_ptr< ExtendedLibrary > library;
    std::unique_ptr< LibraryToolbox > toolbox;
    std::unique_ptr< CoroutineThreadManager > thread_manager;
    std::mutex global_mutex;
    std::string name;
    std::shared_ptr< BackreferenceRegistry< Step, Workset > > step_backrefs;
    std::shared_ptr< Step > root_step;

    std::mutex queue_mutex;
    std::condition_variable queue_variable;
    std::list< nlohmann::json > queue;
};

#endif // WORKSET_H
