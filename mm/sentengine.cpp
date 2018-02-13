
#include "sentengine.h"

#include <vector>

#include "library.h"
#include "utils/utils.h"

using namespace std;

template< typename Map >
static Sentence do_subst(const Sentence &sent, const Map &subst_map, const Library &lib) {
    (void) lib;

    Sentence new_sent;
    size_t new_size = 0;
    for (auto it = sent.begin(); it != sent.end(); it++) {
        const SymTok &tok = *it;
        auto it2 = subst_map.find(tok);
        if (it2 == subst_map.end()) {
            new_size += 1;
        } else {
            const vector< SymTok > &subst = it2->second;
            new_size += subst.size() - 1;
        }
    }
    new_sent.reserve(new_size);
    for (auto it = sent.begin(); it != sent.end(); it++) {
        const SymTok &tok = *it;
        auto it2 = subst_map.find(tok);
        if (it2 == subst_map.end()) {
            //assert(lib.is_constant(tok));
            new_sent.push_back(tok);
        } else {
            const vector< SymTok > &subst = it2->second;
            new_sent.insert(new_sent.end(), subst.begin() + 1, subst.end());
        }
    }
    return new_sent;
}

ProofSentenceTraits<Sentence>::VarType ProofSentenceTraits<Sentence>::floating_to_var(const LibType &lib, LabTok label) {
    return lib.get_sentence(label).at(1);
}

SymTok ProofSentenceTraits<Sentence>::floating_to_type(const LibType &lib, LabTok label)
{
    return lib.get_sentence(label).at(0);
}

SymTok ProofSentenceTraits<Sentence>::sentence_to_type(const LibType &lib, const ProofSentenceTraits<Sentence>::SentType &sent)
{
    (void) lib;
    return sent.at(0);
}

const ProofSentenceTraits<Sentence>::SentType &ProofSentenceTraits<Sentence>::get_sentence(const LibType &lib, LabTok label)
{
    return lib.get_sentence(label);
}

void ProofSentenceTraits<Sentence>::check_match(const LibType &lib, const ProofSentenceTraits<Sentence>::SentType &stack, const ProofSentenceTraits<Sentence>::SentType &templ, const ProofSentenceTraits<Sentence>::SubstMapType &subst_map)
{
    // FIXME label
    ProofError< Sentence > err = { /* label */ {}, stack, templ, subst_map };
    auto stack_it = stack.begin();
    for (auto it = templ.begin(); it != templ.end(); it++) {
        const SymTok &tok = *it;
        if (lib.is_constant(tok)) {
            assert_or_throw< ProofException< Sentence > >(tok == *stack_it, "Essential hypothesis does not match stack beacuse of wrong constant", err);
            stack_it++;
        } else {
            const Sentence &subst = subst_map.at(tok);
            assert(distance(stack_it, stack.end()) >= 0);
            assert_or_throw< ProofException< Sentence > >(subst.size() - 1 <= (size_t) distance(stack_it, stack.end()), "Essential hypothesis does not match stack because stack is shorter", err);
            assert_or_throw< ProofException< Sentence > >(equal(subst.begin() + 1, subst.end(), stack_it), "Essential hypothesis does not match stack because of wrong variable substitution", err);
            stack_it += subst.size() - 1;
        }
    }
    assert_or_throw< ProofException< Sentence > >(stack_it == stack.end(), "Essential hypothesis does not match stack because stack is longer", err);
}

ProofSentenceTraits<Sentence>::SentType ProofSentenceTraits<Sentence>::substitute(const LibType &lib, const ProofSentenceTraits<Sentence>::SentType &templ, const ProofSentenceTraits<Sentence>::SubstMapType &subst_map)
{
    return do_subst(templ, subst_map, lib);
}

ProofSentenceTraits<Sentence>::SentGenerator ProofSentenceTraits<Sentence>::get_variable_iterator(const LibType &lib, const ProofSentenceTraits<Sentence>::SentType &sent)
{
    return SentGenerator(lib, sent);
}

bool ProofSentenceTraits<Sentence>::is_variable(const LibType &lib, ProofSentenceTraits<Sentence>::VarType var)
{
    return !lib.is_constant(var);
}

ProofSentenceTraits<Sentence>::SentGenerator::SentGenerator(const Library &lib, const Sentence &sent) : sentence(sent) {
    (void) lib;
}

Sentence::const_iterator ProofSentenceTraits<Sentence>::SentGenerator::begin() const
{
    return this->sentence.begin();
}

Sentence::const_iterator ProofSentenceTraits<Sentence>::SentGenerator::end() const
{
    return this->sentence.end();
}

template class VectorMap< SymTok, Sentence >;

template struct ProofError< Sentence >;
template class ProofException< Sentence >;
template struct ProofTree< Sentence >;
template class ProofEngineBase< Sentence >;
template class CheckedProofEngine< Sentence >;
template class ExtendedProofEngine< Sentence >;
