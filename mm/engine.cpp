
#include "engine.h"

#include "mm/toolbox.h"
#include "utils/utils.h"

using namespace std;

//#define PROOF_VERBOSE_DEBUG

template class VectorMap< SymTok, Sentence >;

template< typename SentType_ >
ProofEngineBase< SentType_ >::ProofEngineBase(const Library &lib, bool gen_proof_tree) :
    lib(lib), gen_proof_tree(gen_proof_tree)
{
}

template< typename SentType_ >
void ProofEngineBase< SentType_ >::push_stack(const SentType &sent, const std::set< std::pair< VarType, VarType > > &dists)
{
    this->stack.push_back(sent);
    this->dists_stack.push_back(dists);
}

template< typename SentType_ >
void ProofEngineBase< SentType_ >::check_stack_underflow()
{
    if (!this->checkpoints.empty() && this->stack.size() < get<0>(this->checkpoints.back())) {
        throw MMPPException("Checkpointed context exited without committing or rolling back");
    }
}

template< typename SentType_ >
void ProofEngineBase< SentType_ >::stack_resize(size_t size)
{
    this->stack.resize(size);
    this->dists_stack.resize(size);
    this->check_stack_underflow();
}

template< typename SentType_ >
void ProofEngineBase< SentType_ >::pop_stack()
{
    this->stack.pop_back();
    this->dists_stack.pop_back();
    this->check_stack_underflow();
}

template< typename SentType_ >
void ProofEngineBase< SentType_ >::set_gen_proof_tree(bool gen_proof_tree)
{
    this->gen_proof_tree = gen_proof_tree;
}

template< typename SentType_ >
const std::vector< typename ProofEngineBase< SentType_ >::SentType > &ProofEngineBase<SentType_>::get_stack() const
{
    return this->stack;
}

template< typename SentType_ >
const std::set<std::pair<typename ProofEngineBase< SentType_ >::VarType, typename ProofEngineBase< SentType_ >::VarType> > &ProofEngineBase<SentType_>::get_dists() const
{
    return *(this->dists_stack.end()-1);
}

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

template< typename SentType_ >
void ProofEngineBase< SentType_ >::process_assertion(const Assertion &child_ass, LabTok label)
{
    assert_or_throw< ProofException >(this->stack.size() >= child_ass.get_mand_hyps_num(), "Stack too small to pop hypotheses");
    const size_t stack_base = this->stack.size() - child_ass.get_mand_hyps_num();
    //this->dists.clear();
    set< pair< VarType, VarType > > dists;

    // Use the first num_floating hypotheses to build the substitution map
    SubstMapType subst_map;
    subst_map.reserve(child_ass.get_float_hyps().size());
    size_t i = 0;
    for (auto &hyp : child_ass.get_float_hyps()) {
        const SentType &stack_hyp_sent = this->stack[stack_base + i];
        assert(this->dists_stack.at(stack_base + i).empty());
        const SymTok subst_type = TraitsType::floating_to_type(this->lib, hyp);
        const SymTok stack_subst_type = TraitsType::sentence_to_type(this->lib, stack_hyp_sent);
        assert_or_throw< ProofException >(subst_type == stack_subst_type, "Floating hypothesis does not match stack");
#ifdef PROOF_VERBOSE_DEBUG
        if (stack_hyp_sent.size() == 1) {
            cerr << "[" << this->debug_output << "] Matching an empty sentence" << endl;
        }
#endif
        const VarType subst_var = TraitsType::floating_to_var(this->lib, hyp);
        const SentType &subst_sent = stack_hyp_sent;
        auto res = subst_map.insert(make_pair(subst_var, subst_sent));
        assert(res.second);
#ifdef PROOF_VERBOSE_DEBUG
        cerr << "    Hypothesis:     " << print_sentence(hyp_sent, this->lib) << endl << "      matched with: " << print_sentence(stack_hyp_sent, this->lib) << endl;
#endif
        i++;
    }

    // Then parse the other hypotheses and check them
    for (auto &hyp : child_ass.get_ess_hyps()) {
        const SentType &hyp_sent = TraitsType::get_sentence(this->lib, hyp);
        const SentType &stack_hyp_sent = this->stack.at(stack_base + i);
        copy(this->dists_stack.at(stack_base + i).begin(), this->dists_stack.at(stack_base + i).end(), inserter(dists, dists.begin()));
        TraitsType::check_match(this->lib, stack_hyp_sent, hyp_sent, subst_map);
#ifdef PROOF_VERBOSE_DEBUG
        cerr << "    Hypothesis:     " << print_sentence(hyp_sent, this->lib) << endl << "      matched with: " << print_sentence(stack_hyp_sent, this->lib) << endl;
#endif
        i++;
    }

    // Keep track of the distinct variables constraints in the substitution map
    const auto &child_dists = child_ass.get_dists();
    for (auto it1 = subst_map.begin(); it1 != subst_map.end(); it1++) {
        for (auto it2 = subst_map.begin(); it2 != it1; it2++) {
            auto &var1 = it1->first;
            auto &var2 = it2->first;
            auto &subst1 = it1->second;
            auto &subst2 = it2->second;
            if (child_dists.find(minmax(var1, var2)) != child_dists.end()) {
                for (auto &tok1 : subst1) {
                    if (this->lib.is_constant(tok1)) {
                        continue;
                    }
                    for (auto &tok2 : subst2) {
                        if (this->lib.is_constant(tok2)) {
                            continue;
                        }
                        assert_or_throw< ProofException >(tok1 != tok2, "Distinct variable constraint violated");
                        dists.insert(minmax(tok1, tok2));
                    }
                }
            }
        }
    }

    // Build the thesis
    LabTok thesis = child_ass.get_thesis();
    const SentType &thesis_sent = TraitsType::get_sentence(this->lib, thesis);
    vector< SymTok > stack_thesis_sent = TraitsType::substitute(this->lib, thesis_sent, subst_map);
#ifdef PROOF_VERBOSE_DEBUG
    cerr << "    Thesis:         " << print_sentence(thesis_sent, this->lib) << endl << "      becomes:      " << print_sentence(stack_thesis_sent, this->lib) << endl;
#endif

    // Finally do some popping and pushing
#ifdef PROOF_VERBOSE_DEBUG
    cerr << "    Popping from stack " << stack.size() - stack_base << " elements" << endl;
#endif
    this->stack_resize(stack_base);
#ifdef PROOF_VERBOSE_DEBUG
    cerr << "    Pushing on stack: " << print_sentence(stack_thesis_sent, this->lib) << endl;
#endif
    this->push_stack(stack_thesis_sent, dists);
    if (this->gen_proof_tree) {
        // Mark as non essential all the hypotheses that are not
        for (auto it = this->tree_stack.begin() + stack_base; it != this->tree_stack.begin() + stack_base + child_ass.get_float_hyps().size(); it++) {
            it->essential = false;
        }
        vector< ProofTree > children(this->tree_stack.begin() + stack_base, this->tree_stack.end());
        this->tree_stack.resize(stack_base);
        this->proof_tree = { stack_thesis_sent, label, children, dists, true, child_ass.get_number() };
        this->tree_stack.push_back(this->proof_tree);
    }
    this->proof.push_back(label);
}

template< typename SentType_ >
void ProofEngineBase< SentType_ >::process_sentence(const Sentence &sent, LabTok label)
{
    this->push_stack(sent, {});
    if (this->gen_proof_tree) {
        this->proof_tree = { sent, label, {}, {}, true, 0 };
        this->tree_stack.push_back(this->proof_tree);
    }
    this->proof.push_back(label);
}

template< typename SentType_ >
void ProofEngineBase< SentType_ >::process_label(const LabTok label)
{
    const Assertion &child_ass = this->lib.get_assertion(label);
#ifdef PROOF_VERBOSE_DEBUG
    cerr << "  Considering label " << this->lib.resolve_label(label);
#endif
    if (child_ass.is_valid()) {
#ifdef PROOF_VERBOSE_DEBUG
        cerr << ", which is a previous assertion" << endl;
#endif
        this->process_assertion(child_ass, label);
    } else {
#ifdef PROOF_VERBOSE_DEBUG
        cerr << ", which is an hypothesis" << endl;
#endif
        const vector< SymTok > &sent = this->lib.get_sentence(label);
        this->process_sentence(sent, label);
    }
}

template< typename SentType_ >
const std::vector<LabTok> &ProofEngineBase< SentType_ >::get_proof_labels() const
{
    return this->proof;
}

template< typename SentType_ >
const ProofTree &ProofEngineBase< SentType_ >::get_proof_tree() const
{
    return this->proof_tree;
}

template< typename SentType_ >
void ProofEngineBase< SentType_ >::checkpoint()
{
    this->checkpoints.emplace_back(this->stack.size(), this->proof.size(), this->saved_steps.size());
}

template< typename SentType_ >
void ProofEngineBase< SentType_ >::commit()
{
    this->checkpoints.pop_back();
}

template< typename SentType_ >
void ProofEngineBase< SentType_ >::rollback()
{
    this->stack.resize(get<0>(this->checkpoints.back()));
    this->dists_stack.resize(get<0>(this->checkpoints.back()));
    this->proof.resize(get<1>(this->checkpoints.back()));
    this->saved_steps.resize(get<2>(this->checkpoints.back()));
    this->checkpoints.pop_back();
}

template< typename SentType_ >
void ProofEngineBase< SentType_ >::set_debug_output(const string &debug_output)
{
    this->debug_output = debug_output;
}

ProofException::ProofException(string reason, ProofError error) :
    reason(reason), error(error)
{
}

const string &ProofException::get_reason() const
{
    return this->reason;
}

const ProofError &ProofException::get_error() const
{
    return this->error;
}

template class ProofEngineBase< Sentence >;

ProofSentenceTraits<Sentence>::VarType ProofSentenceTraits<Sentence>::floating_to_var(const Library &lib, LabTok label) {
    return lib.get_sentence(label).at(1);
}

SymTok ProofSentenceTraits<Sentence>::floating_to_type(const Library &lib, LabTok label)
{
    return lib.get_sentence(label).at(0);
}

SymTok ProofSentenceTraits<Sentence>::sentence_to_type(const Library &lib, const ProofSentenceTraits<Sentence>::SentType &sent)
{
    (void) lib;
    return sent.at(0);
}

const ProofSentenceTraits<Sentence>::SentType &ProofSentenceTraits<Sentence>::get_sentence(const Library &lib, LabTok label)
{
    return lib.get_sentence(label);
}

void ProofSentenceTraits<Sentence>::check_match(const Library &lib, const ProofSentenceTraits<Sentence>::SentType &stack, const ProofSentenceTraits<Sentence>::SentType &templ, const ProofSentenceTraits<Sentence>::SubstMapType &subst_map)
{
    ProofError err = { /* label */ {}, stack, templ, subst_map };
    auto stack_it = stack.begin();
    for (auto it = templ.begin(); it != templ.end(); it++) {
        const SymTok &tok = *it;
        if (lib.is_constant(tok)) {
            assert_or_throw< ProofException >(tok == *stack_it, "Essential hypothesis does not match stack beacuse of wrong constant", err);
            stack_it++;
        } else {
            const Sentence &subst = subst_map.at(tok);
            assert(distance(stack_it, stack.end()) >= 0);
            assert_or_throw< ProofException >(subst.size() - 1 <= (size_t) distance(stack_it, stack.end()), "Essential hypothesis does not match stack because stack is shorter", err);
            assert_or_throw< ProofException >(equal(subst.begin() + 1, subst.end(), stack_it), "Essential hypothesis does not match stack because of wrong variable substitution", err);
            stack_it += subst.size() - 1;
        }
    }
    assert_or_throw< ProofException >(stack_it == stack.end(), "Essential hypothesis does not match stack because stack is longer", err);
}

ProofSentenceTraits<Sentence>::SentType ProofSentenceTraits<Sentence>::substitute(const Library &lib, const ProofSentenceTraits<Sentence>::SentType &templ, const ProofSentenceTraits<Sentence>::SubstMapType &subst_map)
{
    return do_subst(templ, subst_map, lib);
}
