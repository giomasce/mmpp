#ifndef PROOF_H
#define PROOF_H

#include <vector>
#include <functional>
#include <limits>
#include <type_traits>
#include <memory>

typedef uint16_t CodeTok;

static_assert(std::is_integral< CodeTok >::value);
static_assert(std::is_unsigned< CodeTok >::value);
const CodeTok INVALID_CODE = std::numeric_limits< CodeTok >::max();

class Proof;
class CompressedProof;
class UncompressedProof;
class ProofExecutor;

#include "library.h"

struct ProofError {
    Sentence on_stack;
    Sentence to_subst;
    std::unordered_map< SymTok, Sentence > subst_map;
};

class ProofException {
public:
    ProofException(std::string reason, ProofError error);
    const std::string &get_reason() const;
    const ProofError &get_error() const;

private:
    const std::string reason;
    const ProofError error;
};

inline static void assert_or_throw_pe(bool cond, const std::string &reason="", const ProofError &error={}) {
    if (!cond) {
        throw ProofException(reason, error);
    }
}

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

class ProofExecutor {
public:
    enum CompressionStrategy {
        CS_ANY,
        CS_NO_BACKREFS,
        CS_BACKREFS_ON_IDENTICAL_TREE,
        CS_BACKREFS_ON_IDENTICAL_SENTENCE,
    };

    const std::vector< std::vector< SymTok > > &get_stack() const;
    const ProofTree &get_proof_tree() const;
    const std::vector< LabTok > &get_proof_labels() const;
    void set_debug_output(std::string debug_output);
    virtual void execute() = 0;
    virtual const CompressedProof compress(CompressionStrategy strategy=CS_ANY) = 0;
    virtual const UncompressedProof uncompress() = 0;
    virtual bool check_syntax() = 0;
    virtual ~ProofExecutor();
protected:
    ProofExecutor(const Library &lib, const Assertion &ass, bool gen_proof_tree);
    void process_sentence(const std::vector< SymTok > &sent);
    void process_label(const LabTok label);
    size_t get_hyp_num(const LabTok label) const;
    void final_checks() const;

    const Library &lib;
    const Assertion &ass;
    ProofEngine engine;
};

class CompressedProofExecutor : public ProofExecutor {
    friend class CompressedProof;
public:
    void execute();
    const CompressedProof compress(CompressionStrategy strategy=CS_ANY);
    const UncompressedProof uncompress();
    bool check_syntax();
private:
    CompressedProofExecutor(const Library &lib, const Assertion &ass, const CompressedProof &proof, bool gen_proof_tree=false);

    const CompressedProof &proof;
};

class UncompressedProofExecutor : public ProofExecutor {
    friend class UncompressedProof;
public:
    void execute();
    const CompressedProof compress(CompressionStrategy strategy=CS_ANY);
    const UncompressedProof uncompress();
    bool check_syntax();
private:
    UncompressedProofExecutor(const Library &lib, const Assertion &ass, const UncompressedProof &proof, bool gen_proof_tree=false);

    const UncompressedProof &proof;
};

class Proof {
public:
    virtual std::shared_ptr< ProofExecutor > get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree=false) const = 0;
    virtual ~Proof();
};

class CompressedProof : public Proof {
    friend class CompressedProofExecutor;
public:
    CompressedProof(const std::vector< LabTok > &refs, const std::vector< CodeTok > &codes);
    std::shared_ptr< ProofExecutor > get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree=false) const;
private:
    const std::vector< LabTok > refs;
    const std::vector< CodeTok > codes;
};

class UncompressedProof : public Proof {
    friend class UncompressedProofExecutor;
public:
    UncompressedProof(const std::vector< LabTok > &labels);
    std::shared_ptr< ProofExecutor > get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree=false) const;
    const std::vector< LabTok > &get_labels() const;
private:
    const std::vector< LabTok > labels;
};

#endif // PROOF_H
