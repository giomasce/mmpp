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

class ProofEngine {
public:
    ProofEngine(const LibraryInterface &lib);
    const std::vector< std::vector< SymTok > > &get_stack() const;
    const std::set< std::pair< SymTok, SymTok > > &get_dists() const;
    void process_assertion(const Assertion &child_ass, LabTok label = 0);
    void process_sentence(const std::vector< SymTok > &sent, LabTok label = 0);
    void process_label(const LabTok label);
private:
    const LibraryInterface &lib;
    std::vector< std::vector< SymTok > > stack;
    std::set< std::pair< SymTok, SymTok > > dists;
    std::vector< LabTok > labels;
};

class ProofExecutor {
public:
    const std::vector< std::vector< SymTok > > &get_stack() const;
    virtual void execute() = 0;
    virtual const CompressedProof compress() = 0;
    virtual const UncompressedProof uncompress() = 0;
    virtual bool check_syntax() = 0;
    virtual ~ProofExecutor();
protected:
    ProofExecutor(const LibraryInterface &lib, const Assertion &ass);
    void process_sentence(const std::vector< SymTok > &sent);
    void process_label(const LabTok label);
    size_t get_hyp_num(const LabTok label) const;
    void final_checks() const;

    const LibraryInterface &lib;
    const Assertion &ass;
    ProofEngine engine;
};

class CompressedProofExecutor : public ProofExecutor {
    friend class CompressedProof;
public:
    void execute();
    const CompressedProof compress();
    const UncompressedProof uncompress();
    bool check_syntax();
private:
    CompressedProofExecutor(const LibraryInterface &lib, const Assertion &ass, const CompressedProof &proof);

    const CompressedProof &proof;
};

class UncompressedProofExecutor : public ProofExecutor {
    friend class UncompressedProof;
public:
    void execute();
    const CompressedProof compress();
    const UncompressedProof uncompress();
    bool check_syntax();
private:
    UncompressedProofExecutor(const LibraryInterface &lib, const Assertion &ass, const UncompressedProof &proof);

    const UncompressedProof &proof;
};

class Proof {
public:
    virtual std::shared_ptr< ProofExecutor > get_executor(const LibraryInterface &lib, const Assertion &ass) const = 0;
    virtual ~Proof();
};

class CompressedProof : public Proof {
    friend class CompressedProofExecutor;
public:
    CompressedProof(const std::vector< LabTok > &refs, const std::vector< CodeTok > &codes);
    std::shared_ptr< ProofExecutor > get_executor(const LibraryInterface &lib, const Assertion &ass) const;
private:
    const std::vector< LabTok > refs;
    const std::vector< CodeTok > codes;
};

class UncompressedProof : public Proof {
    friend class UncompressedProofExecutor;
public:
    UncompressedProof(const std::vector< LabTok > &labels);
    std::shared_ptr< ProofExecutor > get_executor(const LibraryInterface &lib, const Assertion &ass) const;
private:
    const std::vector< LabTok > labels;
};

#endif // PROOF_H
