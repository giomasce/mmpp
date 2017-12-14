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
        auto pointer = std::make_shared< enable_make< Step > >(std::move(token));
        pointer->weak_this = pointer;
        return pointer;
    }
    size_t get_id() const;
    const std::vector< std::shared_ptr< Step > > &get_children() const;
    const Sentence &get_sentence() const;
    void set_sentence(const Sentence &sentence);
    std::shared_ptr<Step> get_parent() const;
    bool orphan();
    bool reparent(std::shared_ptr< Step > parent, size_t idx);
    nlohmann::json answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end, std::string method);

protected:
    explicit Step(BackreferenceToken<Step> &&token);

private:
    BackreferenceToken< Step > token;
    std::vector< std::shared_ptr< Step > > children;
    std::weak_ptr< Step > parent;
    std::mutex global_mutex;
    std::weak_ptr< Step > weak_this;

    Sentence sentence;
};

#endif // STEP_H
