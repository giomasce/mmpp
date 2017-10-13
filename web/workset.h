#ifndef WORKSET_H
#define WORKSET_H

#include <mutex>

class Workset;

#include "web/web.h"
#include "library.h"

class Workset {
public:
    Workset();
    nlohmann::json answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end, std::string method);
private:
    std::unique_ptr< ExtendedLibrary > library;
    std::mutex global_mutex;
};

#endif // WORKSET_H
