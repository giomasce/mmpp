#pragma once

#include <vector>
#include <set>

#include "utils/vectormap.h"
#include "funds.h"
#include "mmtypes.h"

//typedef std::unordered_map< SymTok, std::vector< SymTok > > SubstMapType;
typedef VectorMap< SymTok, Sentence > SubstMapType;

struct ProofError {
    LabTok label;
    Sentence on_stack;
    Sentence to_subst;
    SubstMapType subst_map;
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
    std::vector< SymTok > sentence;
    LabTok label;
    std::vector< ProofTree > children;
    std::set< std::pair< SymTok, SymTok > > dists;
    bool essential;

    // FIXME: Duplicated data
    LabTok number;
};

class ProofEngine {
public:
    ProofEngine(const Library &lib, bool gen_proof_tree=false);
    void set_gen_proof_tree(bool gen_proof_tree);
    const std::vector< std::vector< SymTok > > &get_stack() const;
    const std::set< std::pair< SymTok, SymTok > > &get_dists() const;
    void process_assertion(const Assertion &child_ass, LabTok label = 0);
    void process_sentence(const std::vector< SymTok > &sent, LabTok label = 0);
    void process_label(const LabTok label);
    const std::vector< LabTok > &get_proof_labels() const;
    UncompressedProof get_proof() const;
    const ProofTree &get_proof_tree() const;
    void checkpoint();
    void commit();
    void rollback();
    void set_debug_output(const std::string &debug_output);

private:
    void push_stack(const std::vector<SymTok> &sent, const std::set<std::pair<SymTok, SymTok> > &dists);
    void stack_resize(size_t size);
    void pop_stack();
    void check_stack_underflow();

    const Library &lib;
    bool gen_proof_tree;
    std::vector< std::vector< SymTok > > stack;
    std::vector< std::set< std::pair< SymTok, SymTok > > > dists_stack;
    std::vector< ProofTree > tree_stack;
    ProofTree proof_tree;
    //std::set< std::pair< SymTok, SymTok > > dists;
    std::vector< LabTok > proof;
    //std::vector< std::tuple< size_t, std::set< std::pair< SymTok, SymTok > >, size_t > > checkpoints;
    std::vector< std::tuple< size_t, size_t > > checkpoints;
    std::string debug_output;
};
