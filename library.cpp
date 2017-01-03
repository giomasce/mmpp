#include "library.h"
#include "statics.h"
#include "parser.h"
#include "unification.h"
#include "toolbox.h"

#include <algorithm>
#include <iostream>
#include <map>
using namespace std;

LibraryImpl::LibraryImpl()
{
}

SymTok LibraryImpl::create_symbol(string s)
{
    assert_or_throw(is_symbol(s));
    return this->syms.get_or_create(s);
}

LabTok LibraryImpl::create_label(string s)
{
    assert_or_throw(is_label(s));
    auto res = this->labels.get_or_create(s);
    //cerr << "Resizing from " << this->assertions.size() << " to " << res+1 << endl;
    this->sentences.resize(res+1);
    this->assertions.resize(res+1);
    return res;
}

SymTok LibraryImpl::get_symbol(string s) const
{
    return this->syms.get(s);
}

LabTok LibraryImpl::get_label(string s) const
{
    return this->labels.get(s);
}

string LibraryImpl::resolve_symbol(SymTok tok) const
{
    return this->syms.resolve(tok);
}

string LibraryImpl::resolve_label(LabTok tok) const
{
    return this->labels.resolve(tok);
}

size_t LibraryImpl::get_symbols_num() const
{
    return this->syms.size();
}

size_t LibraryImpl::get_labels_num() const
{
    return this->labels.size();
}

void LibraryImpl::add_sentence(LabTok label, std::vector<SymTok> content) {
    //this->sentences.insert(make_pair(label, content));
    assert(label < this->sentences.size());
    this->sentences[label] = content;
}

const std::vector<SymTok> &LibraryImpl::get_sentence(LabTok label) const {
    return this->sentences.at(label);
}

void LibraryImpl::add_assertion(LabTok label, const Assertion &ass)
{
    this->assertions[label] = ass;
    this->assertions_by_type[this->sentences.at(label).at(0)].push_back(label);
}

const Assertion &LibraryImpl::get_assertion(LabTok label) const
{
    return this->assertions.at(label);
}

const std::vector<Assertion> &LibraryImpl::get_assertions() const
{
    return this->assertions;
}

void LibraryImpl::add_constant(SymTok c)
{
    this->consts.insert(c);
}

bool LibraryImpl::is_constant(SymTok c) const
{
    return this->consts.find(c) != this->consts.end();
}

std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > LibraryImpl::unify_assertion(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first) const
{
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > ret;

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
        if (ass.get_ess_hyps().size() != hypotheses.size()) {
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
                auto &hyp = this->get_sentence(ass.get_ess_hyps()[perm[i]]);
                copy(hyp.begin(), hyp.end(), back_inserter(templ));
                templ.push_back(0);
            }
            auto &th = this->get_sentence(ass.get_thesis());
            copy(th.begin(), th.end(), back_inserter(templ));
            auto unifications = unify(sent, templ, *this);
            if (!unifications.empty()) {
                for (auto &unification : unifications) {
                    ret.emplace_back(ass.get_thesis(), perm, unification);
                    if (just_first) {
                        return ret;
                    }
                }
            }
        } while (next_permutation(perm.begin(), perm.end()));
    }

    return ret;
}

std::vector<LabTok> LibraryImpl::prove_type2(const std::vector<SymTok> &type_sent) const
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
        if (templ_ass.get_ess_hyps().size() != 0) {
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
                auto res = matches.insert(make_pair(var, this->prove_type2(new_type_sent)));
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

void LibraryImpl::set_types(const std::vector<LabTok> &types)
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
                     std::set<std::pair<SymTok, SymTok> > dists,
                     std::set<std::pair<SymTok, SymTok> > opt_dists,
                     std::vector<LabTok> float_hyps, std::vector<LabTok> ess_hyps, std::set<LabTok> opt_hyps,
                     LabTok thesis, string comment) :
    valid(true), theorem(theorem), mand_dists(dists), opt_dists(opt_dists),
    float_hyps(float_hyps), ess_hyps(ess_hyps), opt_hyps(opt_hyps), thesis(thesis), proof(NULL), comment(comment),
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

const std::set<std::pair<SymTok, SymTok> > &Assertion::get_mand_dists() const {
    return this->mand_dists;
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

size_t Assertion::get_mand_hyps_num() const
{
    return this->get_float_hyps().size() + this->get_ess_hyps().size();
}

LabTok Assertion::get_mand_hyp(size_t i) const
{
    if (i < this->get_float_hyps().size()) {
        return this->get_float_hyps()[i];
    } else {
        return this->get_ess_hyps()[i-this->get_float_hyps().size()];
    }
}

const std::vector<LabTok> &Assertion::get_float_hyps() const
{
    return this->float_hyps;
}

const std::set<LabTok> &Assertion::get_opt_hyps() const
{
    return this->opt_hyps;
}

const std::vector<LabTok> &Assertion::get_ess_hyps() const
{
    return this->ess_hyps;
}

LabTok Assertion::get_thesis() const {
    return this->thesis;
}

std::shared_ptr<ProofExecutor> Assertion::get_proof_executor(const LibraryImpl &lib) const
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
    if (sp.style == SentencePrinter::STYLE_ALTHTML) {
        os << sp.lib.get_htmlstrings().first;
        os << "<SPAN " << sp.lib.get_htmlstrings().second << ">";
    }
    for (auto &tok : sp.sent) {
        if (first) {
            first = false;
        } else {
            os << string(" ");
        }
        if (sp.style == SentencePrinter::STYLE_PLAIN) {
            os << sp.lib.resolve_symbol(tok);
        } else if (sp.style == SentencePrinter::STYLE_HTML) {
            os << sp.lib.get_htmldefs()[tok];
        } else if (sp.style == SentencePrinter::STYLE_ALTHTML) {
            os << sp.lib.get_althtmldefs()[tok];
        } else if (sp.style == SentencePrinter::STYLE_LATEX) {
            os << sp.lib.get_latexdefs()[tok];
        }
    }
    if (sp.style == SentencePrinter::STYLE_ALTHTML) {
        os << "</SPAN>";
    }
    return os;
}

std::vector<SymTok> LibraryImpl::parse_sentence(const string &in) const
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

SentencePrinter LibraryImpl::print_sentence(const std::vector<SymTok> &sent, SentencePrinter::Style style) const
{
    return SentencePrinter({ sent, *this, style });
}

ProofPrinter LibraryImpl::print_proof(const std::vector<LabTok> &proof) const
{
    return ProofPrinter({ proof, *this });
}

const std::vector<LabTok> &LibraryImpl::get_types() const
{
    return this->types;
}

const std::vector<LabTok> &LibraryImpl::get_types_by_var() const
{
    return this->types_by_var;
}

const std::unordered_map<SymTok, std::vector<LabTok> > &LibraryImpl::get_assertions_by_type() const
{
    return this->assertions_by_type;
}

void LibraryImpl::set_t_comment(std::vector<string> htmldefs, std::vector<string> althtmldefs, std::vector<string> latexdefs, string htmlcss, string htmlfont)
{
    this->htmldefs = htmldefs;
    this->althtmldefs = althtmldefs;
    this->latexdefs = latexdefs;
    this->htmlfont = htmlfont;
    this->htmlcss = htmlcss;
}

const std::vector<string> LibraryImpl::get_htmldefs() const
{
    return this->htmldefs;
}

const std::vector<string> LibraryImpl::get_althtmldefs() const
{
    return this->althtmldefs;
}

const std::vector<string> LibraryImpl::get_latexdefs() const
{
    return this->latexdefs;
}

const std::pair<string, string> LibraryImpl::get_htmlstrings() const
{
    return make_pair(this->htmlcss, this->htmlfont);
}

Library::~Library()
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
