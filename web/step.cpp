#include "step.h"

#include "web/jsonize.h"

using namespace std;

Step::Step(BackreferenceToken<Step> &&token) : token(move(token))
{
}

size_t Step::get_id() const
{
    return this->token.get_id();
}

const std::vector<std::shared_ptr<Step> > &Step::get_children() const
{
    return this->children;
}

nlohmann::json Step::answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end, std::string method)
{
    (void) cb;
    (void) method;
    unique_lock< mutex > lock(this->global_mutex);
    assert_or_throw< SendError >(path_begin != path_end, 404);
    if (*path_begin == "get") {
        path_begin++;
        assert_or_throw< SendError >(path_begin == path_end, 404);
        return jsonize(*this);
    }
    throw SendError(404);
}




