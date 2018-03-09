#pragma once

#include "engine.h"
#include "funds.h"
#include "parsing/parser.h"
#include "parsing/unif.h"
#include "utils/vectormap.h"
#include "toolbox.h"

template<>
struct ProofSentenceTraits< ParsingTree2< SymTok, LabTok > > {
    typedef ParsingTree2< SymTok, LabTok > SentType;
    typedef SubstMap2< SymTok, LabTok > SubstMapType;
    typedef LabTok VarType;
    typedef LibraryToolbox LibType;
    typedef LibraryToolbox AdvLibType;

    class PTIterator {
    public:
        PTIterator(const ParsingTreeNode< SymTok, LabTok > *it);
        bool operator!=(const PTIterator &x) const;
        PTIterator &operator++();
        VarType operator*() const;

    private:
        const ParsingTreeNode< SymTok, LabTok > *it;
    };

    class PTGenerator {
    public:
        PTGenerator(const SentType &sent);
        PTIterator begin() const;
        PTIterator end() const;

    private:
        const SentType &sent;
    };

    static VarType floating_to_var(const LibType &lib, LabTok label);
    static SymTok floating_to_type(const LibType &lib, LabTok label);
    static SymTok sentence_to_type(const LibType &lib, const SentType &sent);
    static const SentType &get_sentence(const LibType &lib, LabTok label);
    static void check_match(const LibType &lib, LabTok label, const SentType &stack, const SentType &templ, const SubstMapType &subst_map);
    static SentType substitute(const LibType &lib, const SentType &templ, const SubstMapType &subst_map);
    static PTGenerator get_variable_iterator(const LibType &lib, const SentType &sent);
    static bool is_variable(const LibType &lib, VarType var);
};

extern template class VectorMap< SymTok, ParsingTree2< SymTok, LabTok > >;

extern template struct ProofError< ParsingTree2< SymTok, LabTok > >;
extern template class ProofException< ParsingTree2< SymTok, LabTok > >;
extern template struct ProofTree< ParsingTree2< SymTok, LabTok > >;
extern template class ProofEngineBase< ParsingTree2< SymTok, LabTok > >;
extern template class CreativeProofEngineImpl< ParsingTree2< SymTok, LabTok > >;
extern template class ProofEngineImpl< ParsingTree2< SymTok, LabTok > >;
