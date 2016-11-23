#include "library.h"
#include "statics.h"

#include <cassert>

using namespace std;

Library::Library()
{

}

Tok Library::create_symbol(string s)
{
    assert(is_symbol(s));
    return this->syms.create(s);
}

Tok Library::create_label(string s)
{
    assert(is_label(s));
    return this->labels.create(s);
}

Tok StringCache::get(string s) {
    auto it = this->dir.find(s);
    if (it == this->dir.end()) {
        return 0;
    } else {
        return it->second;
    }
}

Tok StringCache::create(string s)
{
    auto it = this->dir.find(s);
    if (it == this->dir.end()) {
        bool res;
        tie(it, res) = this->dir.insert(make_pair(s, this->next_id));
        assert(res);
        tie(ignore, res) = this->inv.insert(make_pair(this->next_id, s));
        assert(res);
        this->next_id++;
        return it->second;
    } else {
        return 0;
    }
}

string StringCache::resolve(Tok id)
{
    return this->inv.at(id);
}
