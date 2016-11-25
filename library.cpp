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
    auto res = this->labels.get_or_create(s);
    this->sentences.resize(res+1);
    this->assertions.resize(res+1);
    return res;
}

SymTok Library::get_symbol(string s)
{
    return this->syms.get(s);
}

LabTok Library::get_label(string s)
{
    return this->labels.get(s);
}

size_t Library::get_symbol_num()
{
    return this->syms.size();
}

size_t Library::get_label_num()
{
    return this->labels.size();
}

void Library::add_sentence(LabTok label, std::vector<SymTok> content) {
    //this->sentences.insert(make_pair(label, content));
    assert(label < this->sentences.size());
    this->sentences[label] = content;
}

vector< SymTok > Library::get_sentence(LabTok label) {
    return this->sentences.at(label);
}

void Library::add_assertion(LabTok label, const Assertion &ass)
{
    this->assertions[label] = ass;
}

Assertion Library::get_assertion(LabTok label)
{
    return this->assertions.at(label);
}

Assertion::Assertion() :
    valid(false)
{
}

Assertion::Assertion(bool theorem,
                     int num_floating,
                     std::set<std::pair<SymTok, SymTok> > dists,
                     std::vector<LabTok> hyps,
                     LabTok thesis,
                     std::vector<LabTok> proof) :
    valid(true), num_floating(num_floating), theorem(theorem), dists(dists), hyps(hyps), thesis(thesis), proof(proof)
{
}

bool Assertion::is_valid()
{
    return this->valid;
}

bool Assertion::is_theorem() {
    return this->theorem;
}

int Assertion::get_num_floating()
{
    return this->num_floating;
}

std::set<std::pair<SymTok, SymTok> > Assertion::get_dists() {
    return this->dists;
}

std::vector<LabTok> Assertion::get_hyps() {
    return this->hyps;
}

LabTok Assertion::get_thesis() {
    return this->thesis;
}
