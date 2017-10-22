#include "step.h"

Step::Step(BackreferenceToken<Step> token) : token(token)
{
}

size_t Step::get_id() const
{
    return this->token.get_id();
}




