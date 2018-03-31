
#include <algorithm>
#include <iostream>
#include <map>
#include <regex>

#include "library.h"
#include "proof.h"
#include "utils/utils.h"
#include "reader.h"

LibraryImpl::LibraryImpl()
{
}

SymTok LibraryImpl::create_symbol(std::string s)
{
    assert(is_valid_symbol(s));
    SymTok res = this->syms.create(s);
    if (res == SymTok{}) {
        throw MMPPException("creating an already existing symbol");
    }
    return res;
}

SymTok LibraryImpl::create_or_get_symbol(std::string s)
{
    assert(is_valid_symbol(s));
    SymTok res = this->syms.get_or_create(s);
    return res;
}

LabTok LibraryImpl::create_label(std::string s)
{
    assert(is_valid_label(s));
    auto res = this->labels.create(s);
    if (res == LabTok{}) {
        throw MMPPException("creating an already existing label");
    }
    //cerr << "Resizing from " << this->assertions.size() << " to " << res+1 << endl;
    this->sentences.resize(res.val()+1);
    this->sentence_types.resize(res.val()+1);
    this->assertions.resize(res.val()+1);
    return res;
}

SymTok LibraryImpl::get_symbol(std::string s) const
{
    return this->syms.get(s);
}

LabTok LibraryImpl::get_label(std::string s) const
{
    return this->labels.get(s);
}

std::string LibraryImpl::resolve_symbol(SymTok tok) const
{
    return this->syms.resolve(tok);
}

std::string LibraryImpl::resolve_label(LabTok tok) const
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

const std::unordered_map< SymTok, std::string > &LibraryImpl::get_symbols() const
{
    return this->syms.get_cache();
}

const std::unordered_map<LabTok, std::string> &LibraryImpl::get_labels() const
{
    return this->labels.get_cache();
}

void LibraryImpl::add_sentence(LabTok label, const Sentence &content, SentenceType type) {
    //this->sentences.insert(make_pair(label, content));
    assert(label.val() < this->sentences.size());
    this->sentences[label.val()] = content;
    this->sentence_types[label.val()] = type;
}

const Sentence &LibraryImpl::get_sentence(LabTok label) const {
    return this->sentences.at(label.val());
}

const Sentence *LibraryImpl::get_sentence_ptr(LabTok label) const
{
    if (label.val() < this->sentences.size()) {
        return &this->sentences[label.val()];
    } else {
        return nullptr;
    }
}

SentenceType LibraryImpl::get_sentence_type(LabTok label) const
{
    return this->sentence_types.at(label.val());
}

void LibraryImpl::add_assertion(LabTok label, const Assertion &ass)
{
    this->assertions[label.val()] = ass;
}

const Assertion &LibraryImpl::get_assertion(LabTok label) const
{
    return this->assertions.at(label.val());
}

const std::vector<Sentence> &LibraryImpl::get_sentences() const
{
    return this->sentences;
}

const std::vector<SentenceType> &LibraryImpl::get_sentence_types() const
{
    return this->sentence_types;
}

const std::vector<Assertion> &LibraryImpl::get_assertions() const
{
    return this->assertions;
}

void LibraryImpl::set_constant(SymTok c, bool is_const)
{
    this->consts.resize(std::max(this->consts.size(), static_cast< size_t > (c.val())+1));
    this->consts[c.val()] = is_const;
}

bool LibraryImpl::is_constant(SymTok c) const
{
    if (c.val() < this->consts.size()) {
        return this->consts[c.val()];
    } else {
        return false;
    }
}

void LibraryImpl::set_final_stack_frame(const StackFrame &final_stack_frame)
{
    this->final_stack_frame = final_stack_frame;
}

void LibraryImpl::set_max_number(LabTok max_number)
{
    this->max_number = max_number;
}

Assertion::Assertion() :
    valid(false), theorem(false), modif_disc(false), usage_disc(false), _has_proof(false)
{
}

Assertion::Assertion(bool theorem, bool _has_proof,
                     const std::set<std::pair<SymTok, SymTok> > &dists,
                     const std::set<std::pair<SymTok, SymTok> > &opt_dists,
                     const std::vector<LabTok> &float_hyps, const std::vector<LabTok> &ess_hyps, const std::set<LabTok> &opt_hyps,
                     LabTok thesis, LabTok number, const std::string &comment) :
    valid(true), theorem(theorem), mand_dists(dists), opt_dists(opt_dists),
    float_hyps(float_hyps), ess_hyps(ess_hyps), opt_hyps(opt_hyps), thesis(thesis), number(number), proof(nullptr),
    comment(comment), modif_disc(false), usage_disc(false), _has_proof(_has_proof)
{
    if (this->comment.find("(Proof modification is discouraged.)") != std::string::npos) {
        this->modif_disc = true;
    }
    if (this->comment.find("(New usage is discouraged.)") != std::string::npos) {
        this->usage_disc = true;
    }
}

Assertion::Assertion(const std::vector<LabTok> &float_hyps, const std::vector<LabTok> &ess_hyps) : Assertion({}, {}, {}, {}, float_hyps, ess_hyps, {}, {}, {}) {}

Assertion::~Assertion()
{
}

const std::set<std::pair<SymTok, SymTok> > Assertion::get_dists() const
{
    std::set<std::pair<SymTok, SymTok> > ret;
    set_union(this->get_mand_dists().begin(), this->get_mand_dists().end(),
              this->get_opt_dists().begin(), this->get_opt_dists().end(),
              inserter(ret, ret.begin()));
    return ret;
}

LabTok Assertion::get_mand_hyp(size_t i) const
{
    if (i < this->get_float_hyps().size()) {
        return this->get_float_hyps()[i];
    } else {
        return this->get_ess_hyps()[i-this->get_float_hyps().size()];
    }
}

std::shared_ptr<ProofOperator> Assertion::get_proof_operator(const Library &lib) const
{
    return this->proof->get_operator(lib, *this);
}

const StackFrame &LibraryImpl::get_final_stack_frame() const
{
    return this->final_stack_frame;
}

const LibraryAddendumImpl &LibraryImpl::get_addendum() const
{
    return this->addendum;
}

const ParsingAddendumImpl &LibraryImpl::get_parsing_addendum() const
{
    return this->parsing_addendum;
}

class AssertionGenerator {
public:
    AssertionGenerator(const std::vector< Assertion > &ref) :
        it(ref.begin()), ref(ref) {
    }
    const Assertion *operator()() {
        while (this->it != this->ref.end()) {
            auto it2 = this->it;
            this->it++;
            if (it2->is_valid()) {
                return &*it2;
            }
        }
        return nullptr;
    }

private:
    std::vector< Assertion >::const_iterator it;
    const std::vector< Assertion > &ref;
};

std::function<const Assertion *()> LibraryImpl::list_assertions() const {
    return AssertionGenerator(this->assertions);
}

Generator<std::reference_wrapper<const Assertion> > LibraryImpl::gen_assertions() const
{
    return Generator< std::reference_wrapper< const Assertion > >([this](auto &sink) {
        for (const auto &ass : this->assertions) {
            if (ass.is_valid()) {
                sink(std::cref(ass));
            }
        }
    });
}

LabTok LibraryImpl::get_max_number() const
{
    return this->max_number;
}

bool LibraryImpl::is_immutable() const
{
    return true;
}

void LibraryImpl::set_addendum(const LibraryAddendumImpl &add)
{
    this->addendum = add;
}

void LibraryImpl::set_parsing_addendum(const ParsingAddendumImpl &add)
{
    this->parsing_addendum = add;
}

bool Library::is_immutable() const
{
    return false;
}

Library::~Library()
{
}

std::string fix_htmlcss_for_qt(std::string s)
{
    std::string tmp(s);
    tmp = tmp + "\n\n";
    // Qt does not recognize HTML comments in the stylesheet
    tmp = regex_replace(tmp, std::regex("<!--"), "");
    tmp = regex_replace(tmp, std::regex("-->"), "");
    // Qt uses the system font (not the one provided by the CSS), so it must use the real font name
    tmp = regex_replace(tmp, std::regex("XITSMath-Regular"), "XITS Math");
    // Qt does not recognize the LINK tags and stops rendering altogether
    tmp = regex_replace(tmp, std::regex("<LINK [^>]*>"), "");
    return tmp;
}
