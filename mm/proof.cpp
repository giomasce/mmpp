
#include "proof.h"

#include <iostream>
#include <algorithm>
#include <functional>
#include <cassert>
#include <unordered_map>

#include "utils/utils.h"
#include "library.h"

const size_t max_decompression_size = 1024 * 1024;

Proof::~Proof()
{
}

CompressedProof::CompressedProof(const std::vector<LabTok> &refs, const std::vector<CodeTok> &codes) :
    refs(refs), codes(codes)
{
}

std::shared_ptr<ProofOperator> CompressedProof::get_operator(const Library &lib, const Assertion &ass) const
{
    return std::make_shared< CompressedProofOperator >(lib, ass, *this);
}

const std::vector<LabTok> &CompressedProof::get_refs() const
{
    return this->refs;
}

const std::vector<CodeTok> &CompressedProof::get_codes() const
{
    return this->codes;
}

const CompressedProof CompressedProofOperator::compress(CompressionStrategy strategy)
{
    if (strategy == CS_ANY) {
        return this->proof;
    } else {
        // If we want to recompress with a specified strategy, then we uncompress and compress again
        // TODO Implement a direct algorithm
        return this->uncompress().get_operator(this->lib, this->ass)->compress(strategy);
    }
}

const UncompressedProof CompressedProofOperator::uncompress()
{
    std::vector< size_t > opening_stack;
    std::vector< LabTok > labels;
    std::vector< std::vector< LabTok > > saved;
    for (size_t i = 0; i < this->proof.get_codes().size(); i++) {
        /* The decompressiong algorithm is potentially exponential in the input data,
         * which means that even with very short compressed proof you can obtain very
         * big uncompressed proofs. This is analogous to a "ZIP bomb" and might lead to
         * security problems. Therefore we fail when decompressed data are too long.
         */
        assert_or_throw< ProofException< Sentence > >(labels.size() < max_decompression_size, "Decompressed proof is too large");
        const CodeTok &code = this->proof.get_codes().at(i);
        if (code == 0) {
            saved.emplace_back(labels.begin() + opening_stack.back(), labels.end());
        } else if (code <= this->ass.get_mand_hyps_num()) {
            LabTok label = this->ass.get_mand_hyp(code-1);
            size_t opening = labels.size();
            assert_or_throw< ProofException< Sentence > >(opening_stack.size() >= this->get_hyp_num(label), "Stack too small to pop hypotheses");
            for (size_t j = 0; j < this->get_hyp_num(label); j++) {
                opening = opening_stack.back();
                opening_stack.pop_back();
            }
            opening_stack.push_back(opening);
            labels.push_back(label);
        } else if (code <= this->ass.get_mand_hyps_num() + this->proof.get_refs().size()) {
            LabTok label = this->proof.get_refs().at(code-this->ass.get_mand_hyps_num()-1);
            size_t opening = labels.size();
            assert_or_throw< ProofException< Sentence > >(opening_stack.size() >= this->get_hyp_num(label), "Stack too small to pop hypotheses");
            for (size_t j = 0; j < this->get_hyp_num(label); j++) {
                opening = opening_stack.back();
                opening_stack.pop_back();
            }
            opening_stack.push_back(opening);
            labels.push_back(label);
        } else {
            assert_or_throw< ProofException< Sentence > >(code <= this->ass.get_mand_hyps_num() + this->proof.get_refs().size() + saved.size(), "Code too big in compressed proof");
            const std::vector< LabTok > &sent = saved.at(code-this->ass.get_mand_hyps_num()-this->proof.get_refs().size()-1);
            size_t opening = labels.size();
            opening_stack.push_back(opening);
            std::copy(sent.begin(), sent.end(), back_inserter(labels));
        }
    }
    assert_or_throw< ProofException< Sentence > >(opening_stack.size() == 1, "Proof uncompression did not end with a single element on the stack");
    assert(opening_stack.at(0) == 0);
    return UncompressedProof(labels);
}

bool CompressedProofOperator::check_syntax()
{
    for (auto &ref : this->proof.get_refs()) {
        if (!this->lib.get_assertion(ref).is_valid() && this->ass.get_opt_hyps().find(ref) == this->ass.get_opt_hyps().end()) {
            //cerr << "Syntax error for assertion " << this->lib.resolve_label(this->ass.get_thesis()) << " in reference " << this->lib.resolve_label(ref) << endl;
            //abort();
            return false;
        }
    }
    unsigned int zero_count = 0;
    for (auto &code : this->proof.get_codes()) {
        assert(code != INVALID_CODE);
        if (code == 0) {
            zero_count++;
        } else {
            if (code > this->ass.get_mand_hyps_num() + this->proof.get_refs().size() + zero_count) {
                return false;
            }
        }
    }
    return true;
}

bool CompressedProofOperator::is_trivial() const
{
    bool first_ess_found = false;
    for (const auto &code : this->proof.get_codes()) {
        if (code == 0) {
            return false;
        }
        if (code <= this->ass.get_float_hyps().size()) {
            continue;
        }
        if (first_ess_found) {
            return false;
        }
        first_ess_found = true;
    }
    return true;
}

UncompressedProof::UncompressedProof(const std::vector<LabTok> &labels) :
    labels(labels)
{
}

std::shared_ptr<ProofOperator> UncompressedProof::get_operator(const Library &lib, const Assertion &ass) const
{
    return std::make_shared< UncompressedProofOperator >(lib, ass, *this);
}

const std::vector<LabTok> &UncompressedProof::get_labels() const
{
    return this->labels;
}

static void compress_unwind_proof_tree_phase1(const ProofTree< Sentence > &tree,
                                              std::unordered_map< LabTok, CodeTok > &label_map,
                                              std::vector< LabTok > &refs,
                                              std::set< std::vector< SymTok > > &sents,
                                              std::set< std::vector< SymTok > > &dupl_sents,
                                              CodeTok &code_idx) {
    // If the sentence is duplicate and it has children, prune the subtree and record the sentence as duplicate
    // There is no point in deduplicating subtrees that are already trivial
    if (!tree.children.empty() && sents.find(tree.sentence) != sents.end()) {
        dupl_sents.insert(tree.sentence);
        return;
    }
    // Recur
    for (const auto &child : tree.children) {
        compress_unwind_proof_tree_phase1(child, label_map, refs, sents, dupl_sents, code_idx);
    }
    // If the label is new, record it
    if (label_map.find(tree.label) == label_map.end()) {
        auto res = label_map.insert(std::make_pair(tree.label, code_idx++));
        assert(res.second);
        refs.push_back(tree.label);
    }
    // Record the sentence as seen; this must be done when closing, otherwise there are problem with identical nested sentences
    sents.insert(tree.sentence);
}

// In order to avoid inconsistencies with label numbering, it is important that phase 2 performs the same prunings as phase 1
static void compress_unwind_proof_tree_phase2(const ProofTree< Sentence > &tree,
                                              const std::unordered_map< LabTok, CodeTok > &label_map,
                                              const std::vector< LabTok > &refs,
                                              std::set< std::vector< SymTok > > &sents,
                                              const std::set< std::vector< SymTok > > &dupl_sents,
                                              std::map< std::vector< SymTok >, CodeTok > &dupl_sents_map,
                                              std::vector< CodeTok > &codes, CodeTok &code_idx) {
    // If the sentence is duplicate and it has children, prune the subtree and recall saved sentence
    if (!tree.children.empty() && sents.find(tree.sentence) != sents.end()) {
        codes.push_back(dupl_sents_map.at(tree.sentence));
        return;
    }
    // Recur
    for (const auto &child : tree.children) {
        compress_unwind_proof_tree_phase2(child, label_map, refs, sents, dupl_sents, dupl_sents_map, codes, code_idx);
    }
    // Push this label
    codes.push_back(label_map.at(tree.label));
    // If the sentence is known to be duplicate and has not been saved yet, save it
    if (dupl_sents.find(tree.sentence) != dupl_sents.end() && dupl_sents_map.find(tree.sentence) == dupl_sents_map.end()) {
        auto res = dupl_sents_map.insert(std::make_pair(tree.sentence, code_idx++));
        assert(res.second);
        codes.push_back(0);
    }
    // Record the sentence as seen; this must be done when closing, for same reason as above
    sents.insert(tree.sentence);
}

const CompressedProof UncompressedProofOperator::compress(CompressionStrategy strategy)
{
    CodeTok code_idx = 1;
    std::unordered_map< LabTok, CodeTok > label_map;
    std::vector < LabTok > refs;
    std::vector < CodeTok > codes;
    for (auto &label : this->ass.get_float_hyps()) {
        auto res = label_map.insert(std::make_pair(label, code_idx++));
        assert(res.second);
    }
    for (auto &label : this->ass.get_ess_hyps()) {
        auto res = label_map.insert(std::make_pair(label, code_idx++));
        assert(res.second);
    }
    if (strategy == CS_NO_BACKREFS) {
        for (auto &label : this->proof.get_labels()) {
            if (label_map.find(label) == label_map.end()) {
                auto res = label_map.insert(std::make_pair(label, code_idx++));
                assert(res.second);
                refs.push_back(label);
            }
            codes.push_back(label_map.at(label));
        }
    } else if (strategy == CS_BACKREFS_ON_IDENTICAL_SENTENCE || strategy == CS_ANY) {
        this->set_relax_checks(true);
        this->execute();
        this->set_relax_checks(false);
        const auto &tree = this->engine.get_proof_tree();
        std::set< std::vector< SymTok > > sents;
        std::set< std::vector< SymTok > > dupl_sents;
        std::map< std::vector< SymTok >, CodeTok > dupl_sents_map;
        compress_unwind_proof_tree_phase1(tree, label_map, refs, sents, dupl_sents, code_idx);
        sents.clear();
        compress_unwind_proof_tree_phase2(tree, label_map, refs, sents, dupl_sents, dupl_sents_map, codes, code_idx);
    } else if (strategy == CS_BACKREFS_ON_IDENTICAL_TREE) {
        throw MMPPException("Strategy not implemented yet");
    } else {
        throw MMPPException("Strategy does not exist");
    }
    return CompressedProof(refs, codes);
}

const UncompressedProof UncompressedProofOperator::uncompress()
{
    return this->proof;
}

bool UncompressedProofOperator::check_syntax()
{
    std::set< LabTok > mand_hyps_set(this->ass.get_float_hyps().begin(), this->ass.get_float_hyps().end());
    mand_hyps_set.insert(this->ass.get_ess_hyps().begin(), this->ass.get_ess_hyps().end());
    for (auto &label : this->proof.get_labels()) {
        if (!this->lib.get_assertion(label).is_valid() && mand_hyps_set.find(label) == mand_hyps_set.end()) {
            return false;
        }
    }
    return true;
}

bool UncompressedProofOperator::is_trivial() const
{
    bool first_ess_found = false;
    for (const auto &label : this->proof.get_labels()) {
        auto it = find(this->ass.get_float_hyps().begin(), this->ass.get_float_hyps().end(), label);
        if (it != this->ass.get_float_hyps().end()) {
            continue;
        }
        if (first_ess_found) {
            return false;
        }
        first_ess_found = true;
    }
    return true;
}

size_t ProofOperator::get_hyp_num(const LabTok label) const {
    const Assertion &child_ass = this->lib.get_assertion(label);
    if (child_ass.is_valid()) {
        return child_ass.get_mand_hyps_num();
    } else {
        return 0;
    }
}

CodeTok CompressedDecoder::push_char(char c)
{
    if (is_mm_whitespace(c)) {
        return -1;
    }
    assert_or_throw< MMPPParsingError >('A' <= c && c <= 'Z', "Invalid character in compressed proof");
    if (c == 'Z') {
        assert_or_throw< MMPPParsingError >(this->current == 0, "Invalid character Z in compressed proof");
        return 0;
    } else if ('A' <= c && c <= 'T') {
        uint32_t res = this->current * 20 + (c - 'A' + 1);
        this->current = 0;
        assert(res != INVALID_CODE);
        return res;
    } else {
        this->current = this->current * 5 + (c - 'U' + 1);
        return INVALID_CODE;
    }
}

std::string CompressedEncoder::push_code(CodeTok x)
{
    assert_or_throw< MMPPParsingError >(x != INVALID_CODE);
    if (x == 0) {
        return "Z";
    }
    std::vector< char > buf;
    int div = (x-1) / 20;
    int rem = (x-1) % 20 + 1;
    buf.push_back('A' + rem - 1);
    x = div;
    while (x > 0) {
        div = (x-1) / 5;
        rem = (x-1) % 5 + 1;
        buf.push_back('U' + rem - 1);
        x = div;
    }
    return std::string(buf.rbegin(), buf.rend());
}

ProofOperator::~ProofOperator()
{
}

ProofOperator::ProofOperator(const Library &lib, const Assertion &ass, bool gen_proof_tree) : ProofExecutor< Sentence >(lib, ass, gen_proof_tree)
{
}

CompressedProofOperator::CompressedProofOperator(const Library &lib, const Assertion &ass, const CompressedProof &proof) : ProofExecutor< Sentence >(lib, ass, true), ProofOperator(lib, ass, true), CompressedProofExecutor< Sentence >(lib, ass, proof, true)
{
}

UncompressedProofOperator::UncompressedProofOperator(const Library &lib, const Assertion &ass, const UncompressedProof &proof) : ProofExecutor< Sentence >(lib, ass, true), ProofOperator(lib, ass, true), UncompressedProofExecutor< Sentence >(lib, ass, proof, true)
{
}
