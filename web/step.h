#ifndef STEP_H
#define STEP_H

#include <vector>
#include <memory>

#include "utils/utils.h"
#include "utils/backref_registry.h"

class Step
{
public:
    static std::shared_ptr< Step > create(BackreferenceToken< Step > token) {
        return std::make_shared< enable_make< Step > >(token);
    }

    size_t get_id() const;

protected:
    Step(BackreferenceToken< Step > token);

private:
    BackreferenceToken< Step > token;
    std::vector< std::shared_ptr< Step > > children;
};

#endif // STEP_H
