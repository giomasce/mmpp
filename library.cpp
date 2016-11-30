#include "library.h"
#include "statics.h"
#include "parser.h"
#include "unification.h"

#include <algorithm>
#include <iostream>
#include <map>
using namespace std;

Library::Library()
{
}

SymTok Library::create_symbol(string s)
{
    assert_or_throw(is_symbol(s));
    return this->syms.get_or_create(s);
}

LabTok Library::create_label(string s)
{
    assert_or_throw(is_label(s));
    auto res = this->labels.get_or_create(s);
    //cerr << "Resizing from " << this->assertions.size() << " to " << res+1 << endl;
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
    this->assertions_by_type[this->sentences.at(label).at(0)].push_back(label);
}

const Assertion &Library::get_assertion(LabTok label) const
{
    return this->assertions.at(label);
}

const std::vector<Assertion> &Library::get_assertions() const
{
    return this->assertions;
}

void Library::add_constant(SymTok c)
{
    this->consts.insert(c);
}

bool Library::is_constant(SymTok c) const
{
    return this->consts.find(c) != this->consts.end();
}

std::unordered_map<LabTok, std::vector<std::pair<std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > > > Library::unify_assertion(std::vector<std::vector<SymTok> > hypotheses, std::vector<SymTok> thesis, bool just_first) const
{
    std::unordered_map<LabTok, std::vector<std::pair< std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > > ret;

    vector< SymTok > sent;
    for (auto &hyp : hypotheses) {
        copy(hyp.begin(), hyp.end(), back_inserter(sent));
        sent.push_back(0);
    }
    copy(thesis.begin(), thesis.end(), back_inserter(sent));

    for (const Assertion &ass : this->assertions) {
        if (!ass.is_valid()) {
            continue;
        }
        if (ass.is_usage_disc()) {
            continue;
        }
        if (ass.get_mand_hyps().size() - ass.get_num_floating() != hypotheses.size()) {
            continue;
        }
        // We have to generate all the hypotheses' permutations; fortunately usually hypotheses are not many
        // TODO Is there a better algorithm?
        // The i-th specified hypothesis is matched with the perm[i]-th assertion hypothesis
        vector< size_t > perm;
        for (size_t i = 0; i < hypotheses.size(); i++) {
            perm.push_back(i);
        }
        do {
            vector< SymTok > templ;
            for (size_t i = 0; i < hypotheses.size(); i++) {
                auto &hyp = this->get_sentence(ass.get_mand_hyps().at(ass.get_num_floating()+perm[i]));
                copy(hyp.begin(), hyp.end(), back_inserter(templ));
                templ.push_back(0);
            }
            auto &th = this->get_sentence(ass.get_thesis());
            copy(th.begin(), th.end(), back_inserter(templ));
            auto unifications = unify(sent, templ, *this);
            if (!unifications.empty()) {
                ret.insert(make_pair(ass.get_thesis(), vector<std::pair< std::vector< size_t >,  unordered_map< SymTok, vector< SymTok > > > >()));
                for (auto &unification : unifications) {
                    ret.at(ass.get_thesis()).push_back(make_pair(perm, unification));
                    if (just_first) {
                        return ret;
                    }
                }
            }
        } while (next_permutation(perm.begin(), perm.end()));
    }

    return ret;
}

std::vector<LabTok> Library::prove_type(const std::vector<SymTok> &type_sent) const
{
    // Iterate over all propositions (maybe just axioms would be enough) with zero essential hypotheses, try to match and recur on all matches;
    // hopefully nearly all branches die early and there is just one real long-standing branch;
    // when the length is 2 try to match with floating hypotheses.
    // The current implementation is probably less efficient and more copy-ish than it could be.
    assert(type_sent.size() >= 2);
    if (type_sent.size() == 2) {
        for (auto &test_type : this->types) {
            if (this->get_sentence(test_type) == type_sent) {
                return { test_type };
            }
        }
    }
    auto &type_const = type_sent.at(0);
    // If a there are no assertions for a certain type (which is possible, see for example "set" in set.mm), then processing stops here
    if (this->assertions_by_type.find(type_const) == this->assertions_by_type.end()) {
        return {};
    }
    for (auto &templ : this->assertions_by_type.at(type_const)) {
        const Assertion &templ_ass = this->get_assertion(templ);
        if (templ_ass.get_num_floating() != templ_ass.get_mand_hyps().size()) {
            continue;
        }
        const auto &templ_sent = this->get_sentence(templ);
        auto unifications = unify(type_sent, templ_sent, *this);
        for (auto &unification : unifications) {
            bool failed = false;
            unordered_map< SymTok, vector< LabTok > > matches;
            for (auto &unif_pair : unification) {
                const SymTok &var = unif_pair.first;
                const vector< SymTok > &subst = unif_pair.second;
                SymTok type = this->get_sentence(this->types_by_var[var]).at(0);
                vector< SymTok > new_type_sent = { type };
                // TODO This is not very efficient
                copy(subst.begin(), subst.end(), back_inserter(new_type_sent));
                auto res = matches.insert(make_pair(var, this->prove_type(new_type_sent)));
                assert(res.second);
                if (res.first->second.empty()) {
                    failed = true;
                    break;
                }
            }
            if (!failed) {
                // We have to sort hypotheses by order af appearance; here we assume that numeric orderd of labels coincides with the order of appearance
                vector< pair< LabTok, SymTok > > hyp_labels;
                for (auto &match_pair : matches) {
                    hyp_labels.push_back(make_pair(this->types_by_var[match_pair.first], match_pair.first));
                }
                sort(hyp_labels.begin(), hyp_labels.end());
                vector< LabTok > ret;
                for (auto &hyp_pair : hyp_labels) {
                    auto &hyp_var = hyp_pair.second;
                    copy(matches.at(hyp_var).begin(), matches.at(hyp_var).end(), back_inserter(ret));
                }
                ret.push_back(templ);
                return ret;
            }
        }
    }
    return {};
}

void Library::set_types(const std::vector<LabTok> &types)
{
    assert(this->types.empty());
    this->types = types;
    for (auto &type : types) {
        const SymTok &var = this->sentences.at(type).at(1);
        this->types_by_var.resize(max(this->types_by_var.size(), (size_t) var+1));
        this->types_by_var[var] = type;
    }
}

Assertion::Assertion() :
    valid(false)
{
}

Assertion::Assertion(bool theorem,
                     size_t num_floating,
                     std::set<std::pair<SymTok, SymTok> > dists,
                     std::set<std::pair<SymTok, SymTok> > opt_dists,
                     std::vector<LabTok> hyps, std::set<LabTok> opt_hyps,
                     LabTok thesis, string comment) :
    valid(true), num_floating(num_floating), theorem(theorem), dists(dists), opt_dists(opt_dists),
    hyps(hyps), opt_hyps(opt_hyps), thesis(thesis), proof(NULL), comment(comment),
    modif_disc(false), usage_disc(false)
{
    if (this->comment.find("(Proof modification is discouraged.)") != string::npos) {
        this->modif_disc = true;
    }
    if (this->comment.find("(New usage is discouraged.)") != string::npos) {
        this->usage_disc = true;
    }
}

Assertion::~Assertion()
{
}

bool Assertion::is_valid() const
{
    return this->valid;
}

bool Assertion::is_theorem() const {
    return this->theorem;
}

bool Assertion::is_modif_disc() const
{
    return this->modif_disc;
}

bool Assertion::is_usage_disc() const
{
    return this->usage_disc;
}

size_t Assertion::get_num_floating() const
{
    return this->num_floating;
}

const std::set<std::pair<SymTok, SymTok> > &Assertion::get_mand_dists() const {
    return this->dists;
}

const std::set<std::pair<SymTok, SymTok> > &Assertion::get_opt_dists() const
{
    return this->opt_dists;
}

const std::set<std::pair<SymTok, SymTok> > Assertion::get_dists() const
{
    std::set<std::pair<SymTok, SymTok> > ret;
    set_union(this->get_mand_dists().begin(), this->get_mand_dists().end(),
              this->get_opt_dists().begin(), this->get_opt_dists().end(),
              inserter(ret, ret.begin()));
    return ret;
}

const std::vector<LabTok> &Assertion::get_mand_hyps() const {
    return this->hyps;
}

const std::set<LabTok> &Assertion::get_opt_hyps() const
{
    return this->opt_hyps;
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

std::shared_ptr<ProofExecutor> Assertion::get_proof_executor(const Library &lib) const
{
    return this->proof->get_executor(lib, *this);
}

void Assertion::set_proof(shared_ptr< Proof > proof)
{
    assert(this->theorem);
    this->proof = proof;
}

shared_ptr< Proof > Assertion::get_proof() const
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
    return lib.print_sentence(sent);
}

vector< SymTok > parse_sentence(const std::string &in, const LibraryInterface &lib) {
    return lib.parse_sentence(in);
}

std::vector<SymTok> Library::parse_sentence(const string &in) const
{
    auto toks = tokenize(in);
    vector< SymTok > res;
    for (auto &tok : toks) {
        auto tok_num = this->get_symbol(tok);
        assert_or_throw(tok_num != 0);
        res.push_back(tok_num);
    }
    return res;
}

SentencePrinter Library::print_sentence(const std::vector<SymTok> &sent) const
{
    return SentencePrinter({ sent, *this });
}

ProofPrinter Library::print_proof(const std::vector<LabTok> &proof) const
{
    return ProofPrinter({ proof, *this });
}

LibraryCache::LibraryCache(const Library &lib) :
    lib(lib)
{
}

std::unordered_map<LabTok, std::vector<std::pair<std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > > > LibraryCache::unify_assertion(std::vector<std::vector<SymTok> > hypotheses, std::vector<SymTok> thesis, bool just_first)
{
    const auto key = make_tuple(hypotheses, thesis, just_first);
    if (this->cache.find(key) == this->cache.end()) {
        this->cache[key] = this->lib.unify_assertion(hypotheses, thesis, just_first);
    }
    return this->cache.at(key);
}

SymTok LibraryCache::get_symbol(string s) const
{
    return this->lib.get_symbol(s);
}

LabTok LibraryCache::get_label(string s) const
{
    return this->lib.get_label(s);
}

string LibraryCache::resolve_symbol(SymTok tok) const
{
    return this->lib.resolve_symbol(tok);
}

string LibraryCache::resolve_label(LabTok tok) const
{
    return this->lib.resolve_label(tok);
}

size_t LibraryCache::get_symbol_num() const
{
    return this->lib.get_symbol_num();
}

size_t LibraryCache::get_label_num() const
{
    return this->lib.get_label_num();
}

std::vector<LabTok> LibraryCache::prove_type(const std::vector<SymTok> &type_sent) const
{
    return this->lib.prove_type(type_sent);
}

bool LibraryCache::is_constant(SymTok c) const
{
    return this->lib.is_constant(c);
}

std::vector<SymTok> LibraryCache::parse_sentence(const string &in) const
{
    return this->lib.parse_sentence(in);
}

SentencePrinter LibraryCache::print_sentence(const std::vector<SymTok> &sent) const
{
    return this->lib.print_sentence(sent);
}

ProofPrinter LibraryCache::print_proof(const std::vector<LabTok> &proof) const
{
    return this->lib.print_proof(proof);
}

const Assertion &LibraryCache::get_assertion(LabTok label) const
{
    return this->lib.get_assertion(label);
}

const std::vector<SymTok> &LibraryCache::get_sentence(LabTok label) const
{
    return this->lib.get_sentence(label);
}

LibraryInterface::~LibraryInterface()
{
}

ostream &operator<<(ostream &os, const ProofPrinter &sp)
{
    bool first = true;
    for (auto &label : sp.proof) {
        if (first) {
            first = false;
        } else {
            os << string(" ");
        }
        os << sp.lib.resolve_label(label);
    }
    return os;
}
