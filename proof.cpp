
#include "proof.h"

#include <algorithm>

using namespace std;

Proof::Proof(Library &lib, Assertion &ass) :
    lib(lib), ass(ass)
{
}

CompressedProof::CompressedProof(Library &lib, Assertion &ass, std::vector<LabTok> refs, std::vector<int> codes) :
    Proof(lib, ass), refs(refs), codes(codes)
{
}

UncompressedProof CompressedProof::uncompress()
{

}

void CompressedProof::execute()
{
    vector< vector< SymTok > > temps;
    vector< vector< SymTok > > stack;
}

UncompressedProof::UncompressedProof(Library &lib, Assertion &ass, std::vector<LabTok> labels) :
    Proof(lib, ass), labels(labels)
{
}

CompressedProof UncompressedProof::compress()
{

}

void UncompressedProof::execute()
{
    vector< vector< SymTok > > stack;
    for (auto &label : this->labels) {
        Assertion child = this->lib.get_assertion(label);
        if (child.is_valid()) {

        } else {
            // In line of principle searching in a set would be faster, but since usually hypotheses are not many this is not a problem
            assert(find(this->ass.get_hyps().begin(), this->ass.get_hyps().end(), label) != this->ass.get_hyps().end());
            stack.push_back(this->lib.get_sentence(label));
        }
    }
}
