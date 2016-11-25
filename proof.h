#ifndef PROOF_H
#define PROOF_H

#include <vector>
#include <functional>

class Proof;
class CompressedProof;
class UncompressedProof;

#include "library.h"

class Proof {
public:
    virtual void execute() const = 0;
    virtual const CompressedProof &compress() const = 0;
    virtual const UncompressedProof &uncompress() const = 0;
    virtual bool check_syntax() const = 0;
protected:
    Library &lib;
    Assertion &ass;
    Proof(Library &lib, Assertion &ass);
    void execute_internal(std::function< SymTok() > label_gen) const;
};

class CompressedProof : public Proof {
public:
    CompressedProof(Library &lib, Assertion &ass, const std::vector< LabTok > &refs, const std::vector< int > &codes);
    const CompressedProof &compress() const;
    const UncompressedProof &uncompress() const;
    void execute() const;
    bool check_syntax() const;
private:
    const std::vector< LabTok > &refs;
    const std::vector< int > &codes;
};

class UncompressedProof : public Proof {
public:
    UncompressedProof(Library &lib, Assertion &ass, const std::vector< LabTok > &labels);
    const CompressedProof &compress() const;
    const UncompressedProof &uncompress() const;
    void execute() const;
    bool check_syntax() const;
private:
    const std::vector< LabTok > &labels;
};

#endif // PROOF_H
