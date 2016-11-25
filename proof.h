#ifndef PROOF_H
#define PROOF_H

#include <vector>

#include "library.h"

class CompressedProof;
class UncompressedProof;

class Proof {
protected:
    Library &lib;
    Assertion &ass;
    Proof(Library &lib, Assertion &ass);
};

class CompressedProof : public Proof {
public:
    CompressedProof(Library &lib, Assertion &ass, std::vector< LabTok > refs, std::vector< int > codes);
    UncompressedProof uncompress();
    void execute();
private:
    std::vector< LabTok > refs;
    std::vector< int > codes;
};

class UncompressedProof : public Proof {
public:
    UncompressedProof(Library &lib, Assertion &ass, std::vector< LabTok > labels);
    CompressedProof compress();
    void execute();
private:
    std::vector< LabTok > labels;
};

#endif // PROOF_H
