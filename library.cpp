#include "library.h"
#include "statics.h"
#include "parser.h"
#include "unification.h"

#include <algorithm>

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

std::unordered_map< LabTok, vector< unordered_map< SymTok, vector< SymTok > > > > Library::unify_assertion(std::vector<std::vector<SymTok> > hypotheses, std::vector<SymTok> thesis)
{
    std::unordered_map< LabTok, vector< unordered_map< SymTok, vector< SymTok > > > > ret;

    vector< SymTok > sent;
    for (auto &hyp : hypotheses) {
        copy(hyp.begin(), hyp.end(), back_inserter(sent));
        sent.push_back(0);
    }
    copy(thesis.begin(), thesis.end(), back_inserter(sent));

    for (Assertion &ass : this->assertions) {
        if (ass.get_hyps().size() - ass.get_num_floating() != hypotheses.size()) {
            continue;
        }
        // We have to generate all the hypotheses' permutations; fortunately usually hypotheses are not many
        // TODO Is there a better algorithm?
        vector< int > perm;
        for (size_t i = 0; i < hypotheses.size(); i++) {
            perm.push_back(i);
        }
        do {
            vector< SymTok > templ;
            for (size_t i = 0; i < hypotheses.size(); i++) {
                auto &hyp = this->get_sentence(ass.get_hyps().at(ass.get_num_floating()+perm[i]));
                copy(hyp.begin(), hyp.end(), back_inserter(templ));
                templ.push_back(0);
            }
            auto &th = this->get_sentence(ass.get_thesis());
            copy(th.begin(), th.end(), back_inserter(templ));
            auto unifications = unify(sent, templ, *this);
            if (!unifications.empty()) {
                auto res = ret.insert(make_pair(ass.get_thesis(), unifications));
                assert(res.second);
            }
        } while (next_permutation(perm.begin(), perm.end()));
    }

    return ret;
}

void Library::prove_type_internal(std::vector< SymTok >::const_iterator begin,
                                  std::vector< SymTok >::const_iterator end,
                                  std::vector< LabTok > &ret) const {
    // Iterate over all axioms with zero essential hypotheses, try to match and recur on all matches;
    // hopefully nearly all branches die early and there is just one real long-standing branch;
    // when the length is 2 try to match on floating hypotheses
}

std::vector<LabTok> Library::prove_type(const std::vector<SymTok> &type) const
{
    vector< LabTok > ret;
    this->prove_type_internal(type.begin(), type.end(), ret);
    return ret;
}

void Library::set_types(const std::vector<LabTok> &types)
{
    assert(this->types.empty());
    this->types = types;
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

std::vector<LabTok> Assertion::get_ess_hyps() const
{
    vector< LabTok > ret;
    copy(this->hyps.begin() + this->num_floating, this->hyps.end(), back_inserter(ret));
    return ret;
}

LabTok Assertion::get_thesis() const {
    return this->thesis;
}

void Assertion::add_proof(Proof *proof)
{
    assert(this->theorem);
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

vector< SymTok > parse_sentence(const std::string &in, const Library &lib) {
    auto toks = tokenize(in);
    vector< SymTok > res;
    for (auto &tok : toks) {
        auto tok_num = lib.get_symbol(tok);
        assert(tok_num != 0);
        res.push_back(tok_num);
    }
    return res;
}
