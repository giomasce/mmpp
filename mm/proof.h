#pragma once

#include <vector>
#include <functional>
#include <limits>
#include <type_traits>
#include <memory>

#include "utils/vectormap.h"
#include "funds.h"
#include "utils/utils.h"
#include "sentengine.h"
#include "mmtypes.h"

class ProofExecutor {
public:
    enum CompressionStrategy {
        CS_ANY,
        CS_NO_BACKREFS,
        CS_BACKREFS_ON_IDENTICAL_TREE,
        CS_BACKREFS_ON_IDENTICAL_SENTENCE,
    };

    const std::vector< std::vector< SymTok > > &get_stack() const;
    const ProofTree< Sentence > &get_proof_tree() const;
    const std::vector< LabTok > &get_proof_labels() const;
    void set_debug_output(std::string debug_output);
    virtual void execute() = 0;
    virtual ~ProofExecutor();

protected:
    ProofExecutor(const Library &lib, const Assertion &ass, bool gen_proof_tree);
    void process_label(const LabTok label);
    size_t save_step();
    void process_saved_step(size_t step_num);
    void final_checks() const;

    const Library &lib;
    const Assertion &ass;
    CheckedProofEngine< Sentence > engine;
};

class CompressedProofExecutor : virtual public ProofExecutor {
public:
    CompressedProofExecutor(const Library &lib, const Assertion &ass, const CompressedProof &proof, bool gen_proof_tree=false);
    void execute();

protected:
    const CompressedProof &proof;
};

class UncompressedProofExecutor : virtual public ProofExecutor {
public:
    UncompressedProofExecutor(const Library &lib, const Assertion &ass, const UncompressedProof &proof, bool gen_proof_tree=false);
    void execute();

protected:
    const UncompressedProof &proof;
};

class ProofOperator : virtual public ProofExecutor {
public:
    enum CompressionStrategy {
        CS_ANY,
        CS_NO_BACKREFS,
        CS_BACKREFS_ON_IDENTICAL_TREE,
        CS_BACKREFS_ON_IDENTICAL_SENTENCE,
    };

    virtual const CompressedProof compress(CompressionStrategy strategy=CS_ANY) = 0;
    virtual const UncompressedProof uncompress() = 0;
    virtual bool check_syntax() = 0;
    /*
     * A proof is trivial if it has only one essential step
     * (which can either be an hypothesis or a previous assertion).
     */
    virtual bool is_trivial() const = 0;
    virtual ~ProofOperator();

protected:
    size_t get_hyp_num(const LabTok label) const;
};

class CompressedProofOperator : public ProofOperator, public CompressedProofExecutor {
public:
    CompressedProofOperator(const Library &lib, const Assertion &ass, const CompressedProof &proof);
    const CompressedProof compress(CompressionStrategy strategy=CS_ANY);
    const UncompressedProof uncompress();
    bool check_syntax();
    bool is_trivial() const;
};

class UncompressedProofOperator : public ProofOperator, public UncompressedProofExecutor {
public:
    UncompressedProofOperator(const Library &lib, const Assertion &ass, const UncompressedProof &proof);
    const CompressedProof compress(CompressionStrategy strategy=CS_ANY);
    const UncompressedProof uncompress();
    bool check_syntax();
    bool is_trivial() const;
};

class Proof {
public:
    virtual std::shared_ptr< ProofExecutor > get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree=false) const = 0;
    virtual std::shared_ptr< ProofOperator > get_operator(const Library &lib, const Assertion &ass) const = 0;
    virtual ~Proof();
};

class CompressedProof : public Proof {
public:
    CompressedProof(const std::vector< LabTok > &refs, const std::vector< CodeTok > &codes);
    std::shared_ptr< ProofExecutor > get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree=false) const;
    std::shared_ptr< ProofOperator > get_operator(const Library &lib, const Assertion &ass) const;
    const std::vector< LabTok > &get_refs() const;
    const std::vector< CodeTok > &get_codes() const;

private:
    const std::vector< LabTok > refs;
    const std::vector< CodeTok > codes;
};

class UncompressedProof : public Proof {
public:
    UncompressedProof(const std::vector< LabTok > &labels);
    std::shared_ptr< ProofExecutor > get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree=false) const;
    std::shared_ptr< ProofOperator > get_operator(const Library &lib, const Assertion &ass) const;
    const std::vector< LabTok > &get_labels() const;

private:
    const std::vector< LabTok > labels;
};

class CompressedEncoder {
public:
    std::string push_code(CodeTok x);
};

class CompressedDecoder {
public:
    CodeTok push_char(char c);

private:
    uint32_t current = 0;
};
