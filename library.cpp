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
    return this->syms.get_or_create(s);
}

Tok Library::create_label(string s)
{
    assert(is_label(s));
    return this->labels.get_or_create(s);
}

Tok Library::get_symbol(string s)
{
    return this->syms.get(s);
}

Tok Library::get_label(string s)
{
    return this->labels.get(s);
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

Tok StringCache::get_or_create(string s) {
    Tok tok = this->get(s);
    if (tok == 0) {
        tok = this->create(s);
    }
    return tok;
}

string StringCache::resolve(Tok id)
{
    return this->inv.at(id);
}

Assertion::Assertion(bool theorem, std::vector<std::pair<Tok, Tok> > types, std::vector<std::pair<Tok, Tok> > dists, std::vector<std::vector<Tok> > hyps, std::vector<Tok> thesis, std::vector<Tok> proof) :
    theorem(theorem), types(types), dists(dists), hyps(hyps), thesis(thesis), proof(proof)
{
}
