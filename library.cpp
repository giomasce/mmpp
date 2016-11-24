#include "library.h"
#include "statics.h"

using namespace std;

Library::Library()
{

}

SymTok Library::create_symbol(string s)
{
    assert(is_symbol(s));
    return this->syms.get_or_create(s);
}

LabTok Library::create_label(string s)
{
    assert(is_label(s));
    return this->labels.get_or_create(s);
}

SymTok Library::get_symbol(string s)
{
    return this->syms.get(s);
}

LabTok Library::get_label(string s)
{
    return this->labels.get(s);
}

Assertion::Assertion(bool theorem,
                     std::vector<std::pair<SymTok, SymTok> > types,
                     std::vector<std::pair<SymTok, SymTok> > dists,
                     std::vector<std::vector<SymTok> > hyps,
                     std::vector<LabTok> thesis,
                     std::vector<LabTok> proof) :
    theorem(theorem), types(types), dists(dists), hyps(hyps), thesis(thesis), proof(proof)
{
}
