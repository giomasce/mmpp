#ifndef PROOF_H
#define PROOF_H

#include <vector>
#include <functional>
#include <limits>
#include <type_traits>

typedef uint16_t CodeTok;

static_assert(std::is_integral< CodeTok >::value);
static_assert(std::is_unsigned< CodeTok >::value);
const CodeTok INVALID_CODE = std::numeric_limits< CodeTok >::max();

class Proof;
class CompressedProof;
class UncompressedProof;

#include "library.h"

class ProofExecutor {
public:
    ProofExecutor(const Library &lib, const Assertion &ass);
    void process_assertion(const Assertion &child_ass);
    void process_sentence(const std::vector< SymTok > &sent);
    void process_label(const LabTok label);
    const std::vector< std::vector< SymTok > > &get_stack();
private:
    const Library &lib;
    const Assertion &ass;
    std::vector< std::vector< SymTok > > stack;
};

class Proof {
public:
    virtual void execute() const = 0;
    virtual const CompressedProof compress() const = 0;
    virtual const UncompressedProof uncompress() const = 0;
    virtual bool check_syntax() const = 0;
    virtual ~Proof();

protected:
    const Library &lib;
    const Assertion *ass;
    Proof(const Library &lib, const Assertion &ass);
};

class CompressedProof : public Proof {
public:
    CompressedProof(const Library &lib, const Assertion &ass, const std::vector< LabTok > &refs, const std::vector< CodeTok > &codes);
    const CompressedProof compress() const;
    const UncompressedProof uncompress() const;
    void execute() const;
    bool check_syntax() const;
    virtual ~CompressedProof();
private:
    const std::vector< LabTok > refs;
    const std::vector< CodeTok > codes;
};

class UncompressedProof : public Proof {
public:
    UncompressedProof(const Library &lib, const Assertion &ass, const std::vector< LabTok > &labels);
    const CompressedProof compress() const;
    const UncompressedProof uncompress() const;
    void execute() const;
    bool check_syntax() const;
    virtual ~UncompressedProof();
private:
    const std::vector< LabTok > labels;
};

#endif // PROOF_H
