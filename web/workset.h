#ifndef WORKSET_H
#define WORKSET_H

#include <mutex>
#include <memory>

class Workset;

#include "web/web.h"
#include "library.h"
#include "web/step.h"

class Workset {
public:
    Workset();
    nlohmann::json answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end, std::string method);
    void load_library(boost::filesystem::path filename);
    const std::string &get_name();
    void set_name(const std::string &name);

private:
    std::unique_ptr< ExtendedLibrary > library;
    std::mutex global_mutex;
    std::string name;
    BackreferenceRegistry< Step > step_backrefs;
    std::shared_ptr< Step > root_step;
};

#endif // WORKSET_H
