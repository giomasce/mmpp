
#include "proof.h"

#include <iostream>
#include <algorithm>
#include <functional>
#include <cassert>

// I do not want this to apply to cassert
#define NDEBUG

using namespace std;

Proof::~Proof()
{
}

Proof::Proof(const Library &lib) :
    lib(lib), ass(&ass)
{
}

CompressedProof::CompressedProof(const Library &lib, const Assertion &ass, const std::vector<LabTok> &refs, const std::vector<CodeTok> &codes) :
    Proof(lib, ass), refs(refs), codes(codes)
{
}

const CompressedProof CompressedProof::compress() const
{
    return *this;
}

const UncompressedProof CompressedProof::uncompress() const
{

}

void CompressedProof::execute() const
{
#ifndef NDEBUG
    cerr << "Executing proof of " << this->lib.resolve_label(this->ass.get_thesis()) << endl;
#endif
    ProofExecutor pe(this->lib, *this->ass);
    vector< vector< SymTok > > saved;
    for (auto &code : this->codes) {
        if (code == 0) {
            saved.push_back(pe.get_stack().back());
        } else if (code <= this->ass->get_mand_hyps().size()) {
            LabTok label = this->ass->get_mand_hyps().at(code-1);
            pe.process_label(label);
        } else if (code <= this->ass->get_mand_hyps().size() + this->refs.size()) {
            LabTok label = this->refs.at(code-this->ass->get_mand_hyps().size()-1);
            pe.process_label(label);
        } else {
            assert(code <= this->ass->get_mand_hyps().size() + this->refs.size() + saved.size());
            const vector< SymTok > &sent = saved.at(code-this->ass->get_mand_hyps().size()-this->refs.size()-1);
            pe.process_sentence(sent);
        }
    }
    assert(pe.get_stack().size() == 1);
    assert(pe.get_stack().at(0) == this->lib.get_sentence(this->ass->get_thesis()));
}

bool CompressedProof::check_syntax() const
{
    for (auto &ref : this->refs) {
        if (!this->lib.get_assertion(ref).is_valid() && this->ass->get_opt_hyps().find(ref) == this->ass->get_opt_hyps().end()) {
            //cerr << "Syntax error for assertion " << this->lib.resolve_label(this->ass.get_thesis()) << " in reference " << this->lib.resolve_label(ref) << endl;
            //abort();
            return false;
        }
    }
    unsigned int zero_count = 0;
    for (auto &code : this->codes) {
        assert(code != INVALID_CODE);
        if (code == 0) {
            zero_count++;
        } else {
            if (code > this->ass->get_mand_hyps().size() + this->refs.size() + zero_count) {
                return false;
            }
        }
    }
    return true;
}

CompressedProof::~CompressedProof()
{
}

UncompressedProof::UncompressedProof(const Library &lib, const Assertion &ass, const std::vector<LabTok> &labels) :
    Proof(lib, ass), labels(labels)
{
}

const CompressedProof UncompressedProof::compress() const
{
    // TODO use backreferences
    CodeTok code_idx = 1;
    unordered_map< LabTok, CodeTok > label_map;
    vector < LabTok > refs;
    vector < CodeTok > codes;
    for (auto &label : this->ass->get_mand_hyps()) {
        auto res = label_map.insert(make_pair(label, code_idx++));
        assert(res.second);
    }
    for (auto &label : this->labels) {
        if (label_map.find(label) == label_map.end()) {
            auto res = label_map.insert(make_pair(label, code_idx++));
            assert(res.second);
            refs.push_back(label);
        }
        codes.push_back(label_map.at(label));
    }
    return CompressedProof(this->lib, *this->ass, refs, codes);
}

const UncompressedProof UncompressedProof::uncompress() const
{
    return *this;
}

void UncompressedProof::execute() const
{
#ifndef NDEBUG
    cerr << "Executing proof of " << this->lib.resolve_label(this->ass.get_thesis()) << endl;
#endif
    ProofExecutor pe(this->lib, *this->ass);
    for (auto &label : this->labels) {
        pe.process_label(label);
    }
    assert(pe.get_stack().size() == 1);
    assert(pe.get_stack().at(0) == this->lib.get_sentence(this->ass->get_thesis()));
}

bool UncompressedProof::check_syntax() const
{
    const set< LabTok > mand_hyps_set(this->ass->get_mand_hyps().begin(), this->ass->get_mand_hyps().end());
    for (auto &label : this->labels) {
        if (!this->lib.get_assertion(label).is_valid() && mand_hyps_set.find(label) == mand_hyps_set.end()) {
            return false;
        }
    }
    return true;
}

UncompressedProof::~UncompressedProof()
{
}

ProofExecutor::ProofExecutor(const Library &lib, const Assertion &ass) :
    lib(lib), ass(ass) {
}

void ProofExecutor::process_assertion(const Assertion &child_ass)
{
    // FIXME check distinct variables
    assert(this->stack.size() >= child_ass.get_mand_hyps().size());
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
        assert(hyp_sent.at(0) == stack_hyp_sent.at(0));
        auto res = subst_map.insert(make_pair(hyp_sent.at(1), vector< SymTok >(stack_hyp_sent.begin()+1, stack_hyp_sent.end())));
        assert(res.second);
#ifndef NDEBUG
        cerr << "    Hypothesis:     " << print_sentence(hyp_sent, this->lib) << endl << "      matched with: " << print_sentence(stack_hyp_sent, this->lib) << endl;
#endif
    }

    // Then parse the other hypotheses and check them
    for (; i < child_ass.get_mand_hyps().size(); i++) {
        SymTok hyp = child_ass.get_mand_hyps().at(i);
        const vector< SymTok > &hyp_sent = this->lib.get_sentence(hyp);
        assert(this->lib.is_constant(hyp_sent.at(0)));
        const vector< SymTok > &stack_hyp_sent = this->stack.at(stack_base + i);
        auto stack_it = stack_hyp_sent.begin();
        for (auto it = hyp_sent.begin(); it != hyp_sent.end(); it++) {
            const SymTok &tok = *it;
            if (this->lib.is_constant(tok)) {
                assert(tok == *stack_it);
                stack_it++;
            } else {
                const vector< SymTok > &subst = subst_map.at(tok);
                assert(distance(stack_it, stack_hyp_sent.end()) >= 0);
                assert(subst.size() <= (size_t) distance(stack_it, stack_hyp_sent.end()));
                assert(equal(subst.begin(), subst.end(), stack_it));
                stack_it += subst.size();
            }
        }
        assert(stack_it == stack_hyp_sent.end());
#ifndef NDEBUG
        cerr << "    Hypothesis:     " << print_sentence(hyp_sent, this->lib) << endl << "      matched with: " << print_sentence(stack_hyp_sent, this->lib) << endl;
#endif
    }

    // Build the thesis
    SymTok thesis = child_ass.get_thesis();
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
}

void ProofExecutor::process_sentence(const vector<SymTok> &sent)
{
#ifndef NDEBUG
    cerr << "    Pushing on stack: " << print_sentence(sent, this->lib) << endl;
#endif
    this->stack.push_back(sent);
}

void ProofExecutor::process_label(const LabTok label)
{
    const Assertion &child_ass = this->lib.get_assertion(label);
#ifndef NDEBUG
    cerr << "  Considering label " << this->lib.resolve_label(label);
#endif
    if (child_ass.is_valid()) {
#ifndef NDEBUG
        cerr << ", which is a previous assertion" << endl;
#endif
        this->process_assertion(child_ass);
    } else {
#ifndef NDEBUG
        cerr << ", which is an hypothesis" << endl;
#endif
        // In line of principle searching in a set would be faster, but since usually hypotheses are not many the vector is probably better
        assert(find(this->ass.get_mand_hyps().begin(), this->ass.get_mand_hyps().end(), label) != this->ass.get_mand_hyps().end() ||
                find(this->ass.get_opt_hyps().begin(), this->ass.get_opt_hyps().end(), label) != this->ass.get_opt_hyps().end());
        const vector< SymTok > &sent = this->lib.get_sentence(label);
        this->process_sentence(sent);
    }
}

const std::vector<std::vector<SymTok> > &ProofExecutor::get_stack()
{
    return this->stack;
}
