
#include "ptengine.h"

ProofSentenceTraits<ParsingTree2<SymTok, LabTok>>::VarType ProofSentenceTraits<ParsingTree2<SymTok, LabTok>>::sym_to_var(const LibType &lib, SymTok sym) {
    return lib.get_var_sym_to_lab(sym);
}

SymTok ProofSentenceTraits<ParsingTree2<SymTok, LabTok>>::var_to_sym(const LibType &lib, VarType var) {
    return lib.get_var_lab_to_sym(var);
}

ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::VarType ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::floating_to_var(const LibType &lib, LabTok label)
{
    /* Sometimes the defining inference for a symbol is repeated more
     * than once (see for example wcel.cA and cA in set.mm), so we do
     * a round of normalization with get_var_sym_to_lab.
     */
    return lib.get_var_sym_to_lab(lib.get_sentence(label).at(1));
}

SymTok ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::floating_to_type(const LibType &lib, LabTok label)
{
    return lib.get_sentence(label).at(0);
}

SymTok ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::sentence_to_type(const LibType &lib, const ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::SentType &sent)
{
    (void) lib;
    return sent.get_root().get_node().type;
}

const ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::SentType &ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::get_sentence(const LibType &lib, LabTok label)
{
    return lib.get_parsed_sents2().at(label.val());
}

void ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::check_match(const LibType &lib, LabTok label, const ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::SentType &stack, const ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::SentType &templ, const ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::SubstMapType &subst_map)
{
    ProofError< ParsingTree2<SymTok, LabTok> > err = { label, stack, templ, subst_map };
    assert_or_throw< ProofException< ParsingTree2< SymTok, LabTok > > >(substitute2(templ, lib.get_standard_is_var(), subst_map) == stack, "Essential hypothesis does not match stack", err);
}

ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::SentType ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::substitute(const LibType &lib, const ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::SentType &templ, const ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::SubstMapType &subst_map)
{
    return substitute2(templ, lib.get_standard_is_var(), subst_map);
}

ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::PTGenerator ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::get_variable_iterator(const LibType &lib, const ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::SentType &sent)
{
    (void) lib;
    return PTGenerator(sent);
}

bool ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::is_variable(const LibType &lib, ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::VarType var)
{
    return lib.get_standard_is_var()(var);
}

ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::PTGenerator::PTGenerator(const ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::SentType &sent) : sent(sent) {}


ProofSentenceTraits< ParsingTree2< SymTok, LabTok > >::PTIterator ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::PTGenerator::begin() const
{
    return this->sent.get_nodes();
}

ProofSentenceTraits< ParsingTree2< SymTok, LabTok > >::PTIterator ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::PTGenerator::end() const
{
    return this->sent.get_nodes() + this->sent.get_nodes_len();
}

ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::PTIterator::PTIterator(const ParsingTreeNode<SymTok, LabTok> *it) : it(it) {}

bool ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::PTIterator::operator!=(const ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::PTIterator &x) const {
    return this->it != x.it;
}

ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::PTIterator &ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::PTIterator::operator++() {
    ++this->it;
    return *this;
}

ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::VarType ProofSentenceTraits<ParsingTree2<SymTok, LabTok> >::PTIterator::operator*() const {
    return this->it->label;
}

template class VectorMap< SymTok, ParsingTree2< SymTok, LabTok > >;

template struct ProofError< ParsingTree2< SymTok, LabTok > >;
template class ProofException< ParsingTree2< SymTok, LabTok > >;
template struct ProofTree< ParsingTree2< SymTok, LabTok > >;
template class ProofEngineBase< ParsingTree2< SymTok, LabTok > >;
template class CreativeProofEngineImpl< ParsingTree2< SymTok, LabTok > >;
template class ProofEngineImpl< ParsingTree2< SymTok, LabTok > >;
