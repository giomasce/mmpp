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

SymTok Library::get_symbol(string s) const
{
    return this->syms.get(s);
}

LabTok Library::get_label(string s) const
{
    return this->labels.get(s);
}

string Library::resolve_symbol(SymTok tok) const
{
    return this->syms.resolve(tok);
}

string Library::resolve_label(LabTok tok) const
{
    return this->labels.resolve(tok);
}

size_t Library::get_symbol_num() const
{
    return this->syms.size();
}

size_t Library::get_label_num() const
{
    return this->labels.size();
}

void Library::add_sentence(LabTok label, std::vector<SymTok> content) {
    //this->sentences.insert(make_pair(label, content));
    assert(label < this->sentences.size());
    this->sentences[label] = content;
}

const std::vector<SymTok> &Library::get_sentence(LabTok label) const {
    return this->sentences.at(label);
}

void Library::add_assertion(LabTok label, const Assertion &ass)
{
    this->assertions[label] = ass;
}

const Assertion &Library::get_assertion(LabTok label) const
{
    return this->assertions.at(label);
}

void Library::add_constant(SymTok c)
{
    this->consts.insert(c);
}

bool Library::is_constant(SymTok c) const
{
    return this->consts.find(c) != this->consts.end();
}

Assertion::Assertion() :
    valid(false)
{
}

Assertion::Assertion(bool theorem,
                     size_t num_floating,
                     std::set<std::pair<SymTok, SymTok> > dists,
                     std::vector<LabTok> hyps,
                     LabTok thesis) :
    valid(true), num_floating(num_floating), theorem(theorem), dists(dists), hyps(hyps), thesis(thesis), proof(NULL)
{
}

bool Assertion::is_valid() const
{
    return this->valid;
}

bool Assertion::is_theorem() const {
    return this->theorem;
}

size_t Assertion::get_num_floating() const
{
    return this->num_floating;
}

const std::set<std::pair<SymTok, SymTok> > &Assertion::get_dists() const {
    return this->dists;
}

const std::vector<LabTok> &Assertion::get_hyps() const {
    return this->hyps;
}

LabTok Assertion::get_thesis() const {
    return this->thesis;
}

void Assertion::add_proof(Proof *proof)
{
    assert(this->proof == NULL);
    assert(proof->check_syntax());
    this->proof = proof;
}

Proof *Assertion::get_proof()
{
    return this->proof;
}

ostream &operator<<(ostream &os, const SentencePrinter &sp)
{
    bool first = true;
    for (auto &tok : sp.sent) {
        if (first) {
            first = false;
        } else {
            os << string(" ");
        }
        os << sp.lib.resolve_symbol(tok);
    }
    return os;
}

SentencePrinter print_sentence(const std::vector<SymTok> &sent, const Library &lib)
{
    return SentencePrinter({ sent, lib });
}
