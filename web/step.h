#ifndef STEP_H
#define STEP_H

#include <vector>
#include <memory>
#include <mutex>

#include "utils/utils.h"
#include "utils/backref_registry.h"

class Step;

#include "web/web.h"

#include "libs/json.h"

class Step
{
public:
    static std::shared_ptr< Step > create(BackreferenceToken< Step > &&token) {
        return std::make_shared< enable_make< Step > >(std::move(token));
    }
    size_t get_id() const;
    const std::vector< std::shared_ptr< Step > > &get_children() const;
    nlohmann::json answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end, std::string method);

protected:
    explicit Step(BackreferenceToken<Step> &&token);

private:
    BackreferenceToken< Step > token;
    std::vector< std::shared_ptr< Step > > children;
    std::mutex global_mutex;
};

#endif // STEP_H
