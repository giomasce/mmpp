#pragma once

#include <vector>
#include <set>

#include "utils/vectormap.h"
#include "funds.h"
#include "mmtypes.h"

//typedef std::unordered_map< SymTok, std::vector< SymTok > > SubstMapType;
typedef VectorMap< SymTok, Sentence > SentenceSubstMap;

struct ProofError {
    LabTok label;
    Sentence on_stack;
    Sentence to_subst;
    SentenceSubstMap subst_map;
};

class ProofException {
public:
    ProofException(std::string reason, ProofError error = {});
    const std::string &get_reason() const;
    const ProofError &get_error() const;

private:
    const std::string reason;
    const ProofError error;
};

struct ProofTree {
    Sentence sentence;
    LabTok label;
    std::vector< ProofTree > children;
    std::set< std::pair< SymTok, SymTok > > dists;
    bool essential;

    // FIXME: Duplicated data
    LabTok number;
};

template< typename SentType >
struct ProofSentenceTraits;

template<>
struct ProofSentenceTraits< Sentence > {
    typedef Sentence SentType;
    typedef SentenceSubstMap SubstMapType;
    typedef SymTok VarType;
};

template< typename SentType_ >
class ProofEngineBase {
public:
    typedef typename ProofSentenceTraits< SentType_ >::SentType SentType;
    typedef typename ProofSentenceTraits< SentType_ >::SubstMapType SubstMapType;
    typedef typename ProofSentenceTraits< SentType_ >::VarType VarType;

    ProofEngineBase(const Library &lib, bool gen_proof_tree=false);
    void set_gen_proof_tree(bool gen_proof_tree);
    const std::vector< SentType > &get_stack() const;
    const std::set< std::pair< VarType, VarType > > &get_dists() const;
    const std::vector< LabTok > &get_proof_labels() const;
    const ProofTree &get_proof_tree() const;
    void set_debug_output(const std::string &debug_output);

    size_t save_step() {
        this->saved_steps.push_back(this->stack.back());
        return this->saved_steps.size() - 1;
    }

    void process_saved_step(size_t step_num) {
        this->process_sentence(this->saved_steps.at(step_num));
    }

protected:
    void process_assertion(const Assertion &child_ass, LabTok label = 0);
    void process_sentence(const Sentence &sent, LabTok label = 0);
    void process_label(const LabTok label);
    void checkpoint();
    void commit();
    void rollback();

private:
    void push_stack(const SentType &sent, const std::set<std::pair<VarType, VarType> > &dists);
    void stack_resize(size_t size);
    void pop_stack();
    void check_stack_underflow();

    const Library &lib;
    bool gen_proof_tree;
    std::vector< SentType > stack;
    std::vector< std::set< std::pair< VarType, VarType > > > dists_stack;
    std::vector< SentType > saved_steps;
    std::vector< ProofTree > tree_stack;
    ProofTree proof_tree;
    //std::set< std::pair< SymTok, SymTok > > dists;
    std::vector< LabTok > proof;
    //std::vector< std::tuple< size_t, std::set< std::pair< SymTok, SymTok > >, size_t > > checkpoints;
    std::vector< std::tuple< size_t, size_t, size_t > > checkpoints;
    std::string debug_output;
};

template< typename SentType_ >
class ExtendedProofEngine : public ProofEngineBase< SentType_ > {
public:
    ExtendedProofEngine(const Library &lib, bool gen_proof_tree = false) : ProofEngineBase< SentType_ >(lib, gen_proof_tree) {}

    void process_label(const LabTok label) {
        this->ProofEngineBase< SentType_ >::process_label(label);
    }
    void process_sentence(const typename ProofEngineBase< SentType_ >::SentType &sent, LabTok label = 0) {
        this->ProofEngineBase< SentType_ >::process_sentence(sent, label);
    }
    void checkpoint() {
        this->ProofEngineBase< SentType_ >::checkpoint();
    }
    void commit() {
        this->ProofEngineBase< SentType_ >::commit();
    }
    void rollback() {
        this->ProofEngineBase< SentType_ >::rollback();
    }
};

template< typename SentType_ >
class CheckedProofEngine : public ProofEngineBase< SentType_ > {
public:
    CheckedProofEngine(const Library &lib, bool gen_proof_tree = false) : ProofEngineBase< SentType_ >(lib, gen_proof_tree) {}

    void process_label(const LabTok label) {
        this->ProofEngineBase< SentType_ >::process_label(label);
    }
};
