
#include "proof.h"
#include "statics.h"

#include <iostream>
#include <algorithm>
#include <functional>
#include <cassert>

// I do not want this to apply to cassert
#define NDEBUG

const size_t max_decompression_size = 1024 * 1024;

using namespace std;

Proof::~Proof()
{
}

CompressedProof::CompressedProof(const std::vector<LabTok> &refs, const std::vector<CodeTok> &codes) :
    refs(refs), codes(codes)
{
}

std::shared_ptr<ProofExecutor> CompressedProof::get_executor(const LibraryInterface &lib, const Assertion &ass) const
{
    return shared_ptr< ProofExecutor >(new CompressedProofExecutor(lib, ass, *this));
}

const CompressedProof CompressedProofExecutor::compress()
{
    return this->proof;
}

const UncompressedProof CompressedProofExecutor::uncompress()
{
    vector< size_t > opening_stack;
    vector< LabTok > labels;
    vector< vector< LabTok > > saved;
    for (size_t i = 0; i < this->proof.codes.size(); i++) {
        /* The decompressiong algorithm is potentially exponential in the input data,
         * which means that even with very short compressed proof you can obtain very
         * big uncompressed proofs. This is analogous to a "ZIP bomb" and might lead to
         * security problems. Therefore we fail when decompressed data are too long.
         */
        assert_or_throw(labels.size() < max_decompression_size, "Decompressed proof is too large");
        const CodeTok &code = this->proof.codes.at(i);
        if (code == 0) {
            saved.emplace_back(labels.begin() + opening_stack.back(), labels.end());
        } else if (code <= this->ass.get_mand_hyps().size()) {
            LabTok label = this->ass.get_mand_hyps().at(code-1);
            size_t opening = labels.size();
            assert_or_throw(opening_stack.size() >= this->get_hyp_num(label), "Stack too small to pop hypotheses");
            for (size_t j = 0; j < this->get_hyp_num(label); j++) {
                opening = opening_stack.back();
                opening_stack.pop_back();
            }
            opening_stack.push_back(opening);
            labels.push_back(label);
        } else if (code <= this->ass.get_mand_hyps().size() + this->proof.refs.size()) {
            LabTok label = this->proof.refs.at(code-this->ass.get_mand_hyps().size()-1);
            size_t opening = labels.size();
            assert_or_throw(opening_stack.size() >= this->get_hyp_num(label), "Stack too small to pop hypotheses");
            for (size_t j = 0; j < this->get_hyp_num(label); j++) {
                opening = opening_stack.back();
                opening_stack.pop_back();
            }
            opening_stack.push_back(opening);
            labels.push_back(label);
        } else {
            assert_or_throw(code <= this->ass.get_mand_hyps().size() + this->proof.refs.size() + saved.size(), "Code too big in compressed proof");
            const vector< LabTok > &sent = saved.at(code-this->ass.get_mand_hyps().size()-this->proof.refs.size()-1);
            size_t opening = labels.size();
            opening_stack.push_back(opening);
            copy(sent.begin(), sent.end(), back_inserter(labels));
        }
    }
    assert_or_throw(opening_stack.size() == 1, "Proof uncompression did not end with a single element on the stack");
    assert(opening_stack.at(0) == 0);
    return UncompressedProof(labels);
}

void CompressedProofExecutor::execute()
{
#ifndef NDEBUG
    cerr << "Executing proof of " << this->lib.resolve_label(this->ass.get_thesis()) << endl;
#endif
    vector< vector< SymTok > > saved;
    for (auto &code : this->proof.codes) {
        if (code == 0) {
            saved.push_back(this->get_stack().back());
        } else if (code <= this->ass.get_mand_hyps().size()) {
            LabTok label = this->ass.get_mand_hyps().at(code-1);
            this->process_label(label);
        } else if (code <= this->ass.get_mand_hyps().size() + this->proof.refs.size()) {
            LabTok label = this->proof.refs.at(code-this->ass.get_mand_hyps().size()-1);
            this->process_label(label);
        } else {
            assert_or_throw(code <= this->ass.get_mand_hyps().size() + this->proof.refs.size() + saved.size(), "Code too big in compressed proof");
            const vector< SymTok > &sent = saved.at(code-this->ass.get_mand_hyps().size()-this->proof.refs.size()-1);
            this->process_sentence(sent);
        }
    }
    this->final_checks();
}

bool CompressedProofExecutor::check_syntax()
{
    for (auto &ref : this->proof.refs) {
        if (!this->lib.get_assertion(ref).is_valid() && this->ass.get_opt_hyps().find(ref) == this->ass.get_opt_hyps().end()) {
            //cerr << "Syntax error for assertion " << this->lib.resolve_label(this->ass.get_thesis()) << " in reference " << this->lib.resolve_label(ref) << endl;
            //abort();
            return false;
        }
    }
    unsigned int zero_count = 0;
    for (auto &code : this->proof.codes) {
        assert(code != INVALID_CODE);
        if (code == 0) {
            zero_count++;
        } else {
            if (code > this->ass.get_mand_hyps().size() + this->proof.refs.size() + zero_count) {
                return false;
            }
        }
    }
    return true;
}

UncompressedProof::UncompressedProof(const std::vector<LabTok> &labels) :
    labels(labels)
{
}

std::shared_ptr<ProofExecutor> UncompressedProof::get_executor(const LibraryInterface &lib, const Assertion &ass) const
{
    return shared_ptr< ProofExecutor >(new UncompressedProofExecutor(lib, ass, *this));
}

const CompressedProof UncompressedProofExecutor::compress()
{
    // TODO use backreferences
    CodeTok code_idx = 1;
    unordered_map< LabTok, CodeTok > label_map;
    vector < LabTok > refs;
    vector < CodeTok > codes;
    for (auto &label : this->ass.get_mand_hyps()) {
        auto res = label_map.insert(make_pair(label, code_idx++));
        assert(res.second);
    }
    for (auto &label : this->proof.labels) {
        if (label_map.find(label) == label_map.end()) {
            auto res = label_map.insert(make_pair(label, code_idx++));
            assert(res.second);
            refs.push_back(label);
        }
        codes.push_back(label_map.at(label));
    }
    return CompressedProof(refs, codes);
}

const UncompressedProof UncompressedProofExecutor::uncompress()
{
    return this->proof;
}

void UncompressedProofExecutor::execute()
{
#ifndef NDEBUG
    cerr << "Executing proof of " << this->lib.resolve_label(this->ass.get_thesis()) << endl;
#endif
    for (auto &label : this->proof.labels) {
        this->process_label(label);
    }
    this->final_checks();
}

bool UncompressedProofExecutor::check_syntax()
{
    const set< LabTok > mand_hyps_set(this->ass.get_mand_hyps().begin(), this->ass.get_mand_hyps().end());
    for (auto &label : this->proof.labels) {
        if (!this->lib.get_assertion(label).is_valid() && mand_hyps_set.find(label) == mand_hyps_set.end()) {
            return false;
        }
    }
    return true;
}

ProofExecutor::ProofExecutor(const LibraryInterface &lib, const Assertion &ass) :
    lib(lib), ass(ass), engine(lib) {
}

void ProofExecutor::process_sentence(const vector<SymTok> &sent)
{
    this->engine.process_sentence(sent);
}

void ProofExecutor::process_label(const LabTok label)
{
    const Assertion &child_ass = this->lib.get_assertion(label);
    if (!child_ass.is_valid()) {
        // In line of principle searching in a set would be faster, but since usually hypotheses are not many the vector is probably better
        assert_or_throw(find(this->ass.get_mand_hyps().begin(), this->ass.get_mand_hyps().end(), label) != this->ass.get_mand_hyps().end() ||
                find(this->ass.get_opt_hyps().begin(), this->ass.get_opt_hyps().end(), label) != this->ass.get_opt_hyps().end(),
                        "Requested label cannot be used by this theorem");
    }
    this->engine.process_label(label);
}

size_t ProofExecutor::get_hyp_num(const LabTok label) const {
    const Assertion &child_ass = this->lib.get_assertion(label);
    if (child_ass.is_valid()) {
        return child_ass.get_mand_hyps().size();
    } else {
        return 0;
    }
}

void ProofExecutor::final_checks() const
{
    assert_or_throw(this->get_stack().size() == 1, "Proof execution did not end with a single element on the stack");
    assert_or_throw(this->get_stack().at(0) == this->lib.get_sentence(this->ass.get_thesis()), "Proof does not prove the thesis");
    assert_or_throw(includes(this->ass.get_dists().begin(), this->ass.get_dists().end(),
                             this->engine.get_dists().begin(), this->engine.get_dists().end()),
                    "Distinct variables constraints are too wide");
}

const std::vector<std::vector<SymTok> > &ProofExecutor::get_stack() const
{
    return this->engine.get_stack();
}

ProofExecutor::~ProofExecutor()
{
}

CompressedProofExecutor::CompressedProofExecutor(const LibraryInterface &lib, const Assertion &ass, const CompressedProof &proof) :
    ProofExecutor(lib, ass), proof(proof)
{
}

UncompressedProofExecutor::UncompressedProofExecutor(const LibraryInterface &lib, const Assertion &ass, const UncompressedProof &proof) :
    ProofExecutor(lib, ass), proof(proof)
{
}

ProofEngine::ProofEngine(const LibraryInterface &lib) :
    lib(lib)
{
}

const std::vector<std::vector<SymTok> > &ProofEngine::get_stack() const
{
    return this->stack;
}

const std::set<std::pair<SymTok, SymTok> > &ProofEngine::get_dists() const
{
    return this->dists;
}

void ProofEngine::process_assertion(const Assertion &child_ass, LabTok label)
{
    assert_or_throw(this->stack.size() >= child_ass.get_mand_hyps().size(), "Stack too small to pop hypotheses");
    assert(child_ass.get_num_floating() <= child_ass.get_mand_hyps().size());
    const size_t stack_base = this->stack.size() - child_ass.get_mand_hyps().size();

    // Use the first num_floating hypotheses to build the substitution map
    unordered_map< SymTok, vector< SymTok > > subst_map;
    size_t i;
    for (i = 0; i < child_ass.get_num_floating(); i++) {
        SymTok hyp = child_ass.get_mand_hyps().at(i);
        const vector< SymTok > &hyp_sent = this->lib.get_sentence(hyp);
        // Some extra checks
        assert(hyp_sent.size() == 2);
        assert(this->lib.is_constant(hyp_sent.at(0)));
        assert(!this->lib.is_constant(hyp_sent.at(1)));
        const vector< SymTok > &stack_hyp_sent = this->stack.at(stack_base + i);
        assert_or_throw(hyp_sent.at(0) == stack_hyp_sent.at(0), "Floating hypothesis does not match stack");
        auto res = subst_map.insert(make_pair(hyp_sent.at(1), vector< SymTok >(stack_hyp_sent.begin()+1, stack_hyp_sent.end())));
        assert(res.second);
#ifndef NDEBUG
        cerr << "    Hypothesis:     " << print_sentence(hyp_sent, this->lib) << endl << "      matched with: " << print_sentence(stack_hyp_sent, this->lib) << endl;
#endif
    }

    // Keep track of the distinct variables constraints in the substitution map
    const auto child_dists = child_ass.get_dists();
    for (auto it1 = subst_map.begin(); it1 != subst_map.end(); it1++) {
        for (auto it2 = it1; it2 != subst_map.end(); it2++) {
            if (it1 == it2) {
                continue;
            }
            auto &var1 = it1->first;
            auto &var2 = it2->first;
            auto &subst1 = it1->second;
            auto &subst2 = it2->second;
            if (child_dists.find(minmax(var1, var2)) != child_dists.end()) {
                for (auto &tok1 : subst1) {
                    if (this->lib.is_constant(tok1)) {
                        continue;
                    }
                    for (auto &tok2 : subst2) {
                        if (this->lib.is_constant(tok2)) {
                            continue;
                        }
                        assert_or_throw(tok1 != tok2, "Distinct variable constraint violated");
                        this->dists.insert(minmax(tok1, tok2));
                    }
                }
            }
        }
    }

    // Then parse the other hypotheses and check them
    for (; i < child_ass.get_mand_hyps().size(); i++) {
        LabTok hyp = child_ass.get_mand_hyps().at(i);
        const vector< SymTok > &hyp_sent = this->lib.get_sentence(hyp);
        assert(this->lib.is_constant(hyp_sent.at(0)));
        const vector< SymTok > &stack_hyp_sent = this->stack.at(stack_base + i);
        auto stack_it = stack_hyp_sent.begin();
        for (auto it = hyp_sent.begin(); it != hyp_sent.end(); it++) {
            const SymTok &tok = *it;
            if (this->lib.is_constant(tok)) {
                assert_or_throw(tok == *stack_it, "Essential hypothesis does not match stack beacuse of wrong constant");
                stack_it++;
            } else {
                const vector< SymTok > &subst = subst_map.at(tok);
                assert(distance(stack_it, stack_hyp_sent.end()) >= 0);
                assert_or_throw(subst.size() <= (size_t) distance(stack_it, stack_hyp_sent.end()), "Essential hypothesis does not match stack because stack is shorter");
                assert_or_throw(equal(subst.begin(), subst.end(), stack_it), "Essential hypothesis does not match stack because of wrong variable substitution");
                stack_it += subst.size();
            }
        }
        assert_or_throw(stack_it == stack_hyp_sent.end(), "Essential hypothesis does not match stack because stack is longer");
#ifndef NDEBUG
        cerr << "    Hypothesis:     " << print_sentence(hyp_sent, this->lib) << endl << "      matched with: " << print_sentence(stack_hyp_sent, this->lib) << endl;
#endif
    }

    // Build the thesis
    LabTok thesis = child_ass.get_thesis();
    const vector< SymTok > &thesis_sent = this->lib.get_sentence(thesis);
    vector< SymTok > stack_thesis_sent;
    for (auto it = thesis_sent.begin(); it != thesis_sent.end(); it++) {
        const SymTok &tok = *it;
        if (this->lib.is_constant(tok)) {
            stack_thesis_sent.push_back(tok);
        } else {
            const vector< SymTok > &subst = subst_map.at(tok);
            copy(subst.begin(), subst.end(), back_inserter(stack_thesis_sent));
        }
    }
#ifndef NDEBUG
    cerr << "    Thesis:         " << print_sentence(thesis_sent, this->lib) << endl << "      becomes:      " << print_sentence(stack_thesis_sent, this->lib) << endl;
#endif

    // Finally do some popping and pushing
#ifndef NDEBUG
    cerr << "    Popping from stack " << stack.size() - stack_base << " elements" << endl;
#endif
    this->stack.resize(stack_base);
#ifndef NDEBUG
    cerr << "    Pushing on stack: " << print_sentence(stack_thesis_sent, this->lib) << endl;
#endif
    this->stack.push_back(stack_thesis_sent);
    this->proof.push_back(label);
}

void ProofEngine::process_sentence(const std::vector<SymTok> &sent, LabTok label)
{
    this->stack.push_back(sent);
    this->proof.push_back(label);
}

void ProofEngine::process_label(const LabTok label)
{
    const Assertion &child_ass = this->lib.get_assertion(label);
#ifndef NDEBUG
    cerr << "  Considering label " << this->lib.resolve_label(label);
#endif
    if (child_ass.is_valid()) {
#ifndef NDEBUG
        cerr << ", which is a previous assertion" << endl;
#endif
        this->process_assertion(child_ass, label);
    } else {
#ifndef NDEBUG
        cerr << ", which is an hypothesis" << endl;
#endif
        const vector< SymTok > &sent = this->lib.get_sentence(label);
        this->process_sentence(sent, label);
    }
}

const std::vector<LabTok> &ProofEngine::get_proof()
{
    return this->proof;
}
