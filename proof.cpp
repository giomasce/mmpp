
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

CompressedProof &CompressedProof::compress()
{
    return *this;
}

UncompressedProof &CompressedProof::uncompress()
{

}

void CompressedProof::execute()
{
    vector< vector< SymTok > > temps;
    vector< vector< SymTok > > stack;
}

bool CompressedProof::check_syntax() const
{
    for (auto &ref : this->refs) {
        if (!this->lib.get_assertion(ref).is_valid()) {
            assert(false);
            return false;
        }
    }
    // TODO check proof execution
    return true;
}

UncompressedProof::UncompressedProof(Library &lib, Assertion &ass, std::vector<LabTok> labels) :
    Proof(lib, ass), labels(labels)
{
}

CompressedProof &UncompressedProof::compress()
{

}

UncompressedProof &UncompressedProof::uncompress()
{
    return *this;
}

// Taken from http://stackoverflow.com/a/5405434/807307
template <class InputIterator1, class InputIterator2>
bool safe_equal( InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2 )
{
  return ( std::distance( first1, last1 ) == std::distance( first2, last2 ) )
     && std::equal( first1, last1, first2 );
}

void UncompressedProof::execute()
{
    // FIXME check distinct variables
    vector< vector< SymTok > > stack;
    for (auto &label : this->labels) {
        const Assertion &child = this->lib.get_assertion(label);
        if (child.is_valid()) {
            assert(stack.size() >= child.get_hyps().size());
            assert(child.get_num_floating() <= child.get_hyps().size());
            const size_t stack_base = stack.size() - child.get_hyps().size();

            // Use the first num_floating hypotheses to build the substitution map
            unordered_map< SymTok, vector< SymTok > > subst_map;
            size_t i;
            for (i = 0; i < child.get_num_floating(); i++) {
                SymTok hyp = child.get_hyps().at(i);
                const vector< SymTok > &hyp_sent = this->lib.get_sentence(hyp);
                // Some extra checks
                assert(hyp_sent.size() == 2);
                assert(this->lib.is_constant(hyp_sent.at(0)));
                assert(!this->lib.is_constant(hyp_sent.at(1)));
                const vector< SymTok > &stack_hyp_sent = stack.at(stack_base + i);
                assert(hyp_sent.at(0) == stack_hyp_sent.at(0));
                auto res = subst_map.insert(make_pair(hyp_sent.at(1), vector< SymTok >(stack_hyp_sent.begin()+1, stack_hyp_sent.end())));
                assert(res.second);
            }

            // Then parse the other hypotheses and check them
            for (; i < child.get_hyps().size(); i++) {
                SymTok hyp = child.get_hyps().at(i);
                const vector< SymTok > &hyp_sent = this->lib.get_sentence(hyp);
                assert(this->lib.is_constant(hyp_sent.at(0)));
                const vector< SymTok > &stack_hyp_sent = stack.at(stack_base + i);
                auto stack_it = stack_hyp_sent.begin();
                for (auto it = hyp_sent.begin(); it != hyp_sent.end(); it++) {
                    const SymTok &tok = *it;
                    if (this->lib.is_constant(tok)) {
                        assert(tok == *stack_it);
                        stack_it++;
                    } else {
                        const vector< SymTok > &subst = subst_map.at(tok);
                        assert(distance(stack_it, stack_hyp_sent.end()) >= 0);
                        assert(subst.size() <= (size_t) distance(stack_it, stack_hyp_sent.end()));
                        assert(equal(subst.begin(), subst.end(), stack_it));
                        stack_it += subst.size();
                    }
                }
                assert(stack_it == stack_hyp_sent.end());
            }

            // Build the thesis
            SymTok thesis = child.get_thesis();
            const vector< SymTok > &thesis_sent = this->lib.get_sentence(thesis);
            vector< SymTok > stack_thesis_sent;
            for (auto it = thesis_sent.begin(); it != thesis_sent.end(); it++) {
                const SymTok &tok = *it;
                if (this->lib.is_constant(tok)) {
                    stack_thesis_sent.push_back(tok);
                } else {
                    const vector< SymTok > &subst = subst_map.at(tok);
                    copy(subst.begin(), subst.end(), back_inserter(stack_thesis_sent));
                }
            }

            // Finally do some popping and pushing
            stack.reserve(stack_base);
            stack.push_back(stack_thesis_sent);
        } else {
            // In line of principle searching in a set would be faster, but since usually hypotheses are not many the vector is probably better
            assert(find(this->ass.get_hyps().begin(), this->ass.get_hyps().end(), label) != this->ass.get_hyps().end());
            stack.push_back(this->lib.get_sentence(label));
        }
    }
}

bool UncompressedProof::check_syntax() const
{
    const set< LabTok > mand_hyps_set(this->ass.get_hyps().begin(), this->ass.get_hyps().end());
    for (auto &label : this->labels) {
        if (!this->lib.get_assertion(label).is_valid() && mand_hyps_set.find(label) == mand_hyps_set.end()) {
            return false;
        }
    }
    return true;
}
