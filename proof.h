#ifndef PROOF_H
#define PROOF_H

#include <vector>

class Proof;
class CompressedProof;
class UncompressedProof;

#include "library.h"

class Proof {
public:
    virtual void execute() = 0;
    virtual CompressedProof &compress() = 0;
    virtual UncompressedProof &uncompress() = 0;
    virtual bool check_syntax() const = 0;
protected:
    Library &lib;
    Assertion &ass;
    Proof(Library &lib, Assertion &ass);
};

class CompressedProof : public Proof {
public:
    CompressedProof(Library &lib, Assertion &ass, std::vector< LabTok > refs, std::vector< int > codes);
    CompressedProof &compress();
    UncompressedProof &uncompress();
    void execute();
    bool check_syntax() const;
private:
    std::vector< LabTok > refs;
    std::vector< int > codes;
};

class UncompressedProof : public Proof {
public:
    UncompressedProof(Library &lib, Assertion &ass, std::vector< LabTok > labels);
    CompressedProof &compress();
    UncompressedProof &uncompress();
    void execute();
    bool check_syntax() const;
private:
    std::vector< LabTok > labels;
};

#endif // PROOF_H
