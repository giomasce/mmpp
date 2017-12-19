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
    Workset();
    nlohmann::json answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end, std::string method);
    void load_library(boost::filesystem::path filename, boost::filesystem::path cache_filename, std::string turnstile);
    const std::string &get_name();
    void set_name(const std::string &name);
    const LibraryToolbox &get_toolbox() const {
        return *this->toolbox;
    }

private:
    std::unique_ptr< ExtendedLibrary > library;
    std::unique_ptr< LibraryToolbox > toolbox;
    std::unique_ptr< CoroutineThreadManager > thread_manager;
    std::mutex global_mutex;
    std::string name;
    std::shared_ptr< BackreferenceRegistry< Step > > step_backrefs;
    std::shared_ptr< Step > root_step;
};

#endif // WORKSET_H
