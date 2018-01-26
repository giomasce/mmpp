#pragma once

#include <vector>
#include <functional>
#include <limits>
#include <type_traits>
#include <memory>

#include "utils/vectormap.h"
#include "funds.h"
#include "utils/utils.h"
#include "engine.h"
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
    const ProofTree &get_proof_tree() const;
    const std::vector< LabTok > &get_proof_labels() const;
    void set_debug_output(std::string debug_output);
    virtual void execute() = 0;
    virtual const CompressedProof compress(CompressionStrategy strategy=CS_ANY) = 0;
    virtual const UncompressedProof uncompress() = 0;
    virtual bool check_syntax() = 0;
    /*
     * A proof is trivial if it has only one essential step
     * (which can either be an hypothesis or a previous assertion).
     */
    virtual bool is_trivial() const = 0;
    virtual ~ProofExecutor();
protected:
    ProofExecutor(const Library &lib, const Assertion &ass, bool gen_proof_tree);
    void process_label(const LabTok label);
    size_t save_step() {
        return this->engine.save_step();
    }
    void process_saved_step(size_t step_num) {
        this->engine.process_saved_step(step_num);
    }
    size_t get_hyp_num(const LabTok label) const;
    void final_checks() const;

    const Library &lib;
    const Assertion &ass;
    CheckedProofEngine engine;
};

class CompressedProofExecutor : public ProofExecutor {
    friend class CompressedProof;
public:
    void execute();
    const CompressedProof compress(CompressionStrategy strategy=CS_ANY);
    const UncompressedProof uncompress();
    bool check_syntax();
    bool is_trivial() const;
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
    bool is_trivial() const;
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
