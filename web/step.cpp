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

std::shared_ptr<Step> Step::get_parent() const
{
    return this->parent.lock();
}

bool Step::orphan()
{
    // We assume that a global lock is held by the Workset, so here we can lock both
    // this and the parent without fear of deadlocks; we still need to lock both,
    // because answer_api1() has already released the Workset's lock
    unique_lock< mutex > lock(this->global_mutex);
    shared_ptr< Step > strong_parent = this->get_parent();
    if (strong_parent == NULL) {
        return false;
    }
    unique_lock< mutex > parent_lock(strong_parent->global_mutex);
    this->parent = weak_ptr< Step >();
    auto &pchildren = strong_parent->children;
    auto it = find_if(pchildren.begin(), pchildren.end(), [this](const shared_ptr< Step > &s)->bool { return s.get() == this; });
    assert(it != pchildren.end());
    pchildren.erase(it);
    return true;
}

bool Step::reparent(std::shared_ptr<Step> parent, size_t idx)
{
    // See comments in orphan() about locking
    unique_lock< mutex > lock(this->global_mutex);
    assert(!this->weak_this.expired());
    if (!this->parent.expired()) {
        return false;
    }
    unique_lock< mutex > parent_lock(parent->global_mutex);
    this->parent = parent;
    auto &pchildren = parent->children;
    pchildren.insert(pchildren.begin() + idx, this->weak_this.lock());
    return true;
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




