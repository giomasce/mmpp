#pragma once

#include <unordered_map>

#include "engine.h"
#include "funds.h"
#include "utils/vectormap.h"
#include "mmtypes.h"

template<>
struct ProofSentenceTraits< Sentence > {
    typedef Sentence SentType;
    typedef VectorMap< SymTok, Sentence > SubstMapType;
    //typedef std::unordered_map< SymTok, Sentence > SubstMapType;
    typedef SymTok VarType;
    typedef Library LibType;
    typedef LibraryToolbox AdvLibType;

    class SentGenerator {
    public:
        SentGenerator(const Library &lib, const Sentence &sent);
        Sentence::const_iterator begin() const;
        Sentence::const_iterator end() const;

    private:
        //const Library &lib;
        const Sentence &sentence;
    };

    static VarType floating_to_var(const LibType &lib, LabTok label);
    static SymTok floating_to_type(const LibType &lib, LabTok label);
    static SymTok sentence_to_type(const LibType &lib, const SentType &sent);
    static const SentType &get_sentence(const LibType &lib, LabTok label);
    static void check_match(const LibType &lib, LabTok label, const SentType &stack, const SentType &templ, const SubstMapType &subst_map);
    static SentType substitute(const LibType &lib, const SentType &templ, const SubstMapType &subst_map);
    static SentGenerator get_variable_iterator(const LibType &lib, const SentType &sent);
    static bool is_variable(const LibType &lib, VarType var);
};

extern template class VectorMap< SymTok, Sentence >;

extern template struct ProofError< Sentence >;
extern template class ProofException< Sentence >;
extern template struct ProofTree< Sentence >;
extern template class ProofEngineBase< Sentence >;
extern template class CreativeProofEngineImpl< Sentence >;
extern template class ProofEngineImpl< Sentence >;
