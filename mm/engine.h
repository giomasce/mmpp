#pragma once

#include <vector>
#include <map>

#include "library.h"
#include "utils/utils.h"
#include "funds.h"
#include "mmtypes.h"

//#define PROOF_VERBOSE_DEBUG

template< typename SentType_ >
struct ProofSentenceTraits;

template< typename SentType_ >
struct ProofError {
    typedef ProofSentenceTraits< SentType_ > TraitsType;
    typedef typename TraitsType::SentType SentType;
    typedef typename TraitsType::SubstMapType SubstMapType;
    typedef typename TraitsType::VarType VarType;

    LabTok label;
    SentType on_stack;
    SentType to_subst;
    SubstMapType subst_map;
};

template< typename SentType_ >
class ProofException {
public:
    typedef ProofSentenceTraits< SentType_ > TraitsType;
    typedef typename TraitsType::SentType SentType;
    typedef typename TraitsType::SubstMapType SubstMapType;
    typedef typename TraitsType::VarType VarType;

    ProofException(std::string reason, ProofError< SentType_ > error = {}): reason(reason), error(error) {}
    const std::string &get_reason() const {
        return this->reason;
    }
    const ProofError< SentType_ > &get_error() const {
        return this->error;
    }

private:
    const std::string reason;
    const ProofError< SentType_ > error;
};

template< typename SentType_ >
struct ProofTree {
    typedef ProofSentenceTraits< SentType_ > TraitsType;
    typedef typename TraitsType::SentType SentType;
    typedef typename TraitsType::SubstMapType SubstMapType;
    typedef typename TraitsType::VarType VarType;

    SentType sentence;
    LabTok label;
    std::vector< ProofTree< SentType_ > > children;
    std::set< std::pair< VarType, VarType > > dists;
    bool essential;

    // FIXME: Duplicated data
    LabTok number;
};

template< typename SentType_ >
class ProofEngineBase {
public:
    typedef ProofSentenceTraits< SentType_ > TraitsType;
    typedef typename TraitsType::SentType SentType;
    typedef typename TraitsType::SubstMapType SubstMapType;
    typedef typename TraitsType::VarType VarType;
    typedef typename TraitsType::LibType LibType;
    typedef typename TraitsType::AdvLibType AdvLibType;

    ProofEngineBase(const LibType &lib, bool gen_proof_tree=false) :
        lib(lib), gen_proof_tree(gen_proof_tree)
    {
    }

    void set_gen_proof_tree(bool gen_proof_tree)
    {
        this->gen_proof_tree = gen_proof_tree;
    }

    const std::vector< SentType > &get_stack() const
    {
        return this->stack;
    }

    const std::set< std::pair< VarType, VarType > > &get_dists() const
    {
        return *(this->dists_stack.end()-1);
    }

    const std::vector< LabTok > &get_proof_labels() const
    {
        return this->proof;
    }

    const ProofTree< SentType_ > &get_proof_tree() const
    {
        return this->proof_tree;
    }

    void set_debug_output(const std::string &debug_output)
    {
        this->debug_output = debug_output;
    }

    size_t save_step() {
        this->saved_steps.push_back(this->stack.back());
        return this->saved_steps.size() - 1;
    }

    void process_saved_step(size_t step_num) {
        this->process_sentence(this->saved_steps.at(step_num));
    }

protected:
    void process_assertion(const Assertion &child_ass, LabTok label = 0)
    {
        assert_or_throw< ProofException< SentType_ > >(this->stack.size() >= child_ass.get_mand_hyps_num(), "Stack too small to pop hypotheses");
        const size_t stack_base = this->stack.size() - child_ass.get_mand_hyps_num();
        //this->dists.clear();
        std::set< std::pair< VarType, VarType > > dists;

        // Use the first num_floating hypotheses to build the substitution map
        SubstMapType subst_map;
        //subst_map.reserve(child_ass.get_float_hyps().size());
        size_t i = 0;
        for (auto &hyp : child_ass.get_float_hyps()) {
            const SentType &stack_hyp_sent = this->stack[stack_base + i];
            assert(this->dists_stack.at(stack_base + i).empty());
            const SymTok subst_type = TraitsType::floating_to_type(this->lib, hyp);
            const SymTok stack_subst_type = TraitsType::sentence_to_type(this->lib, stack_hyp_sent);
            assert_or_throw< ProofException< SentType_ > >(subst_type == stack_subst_type, "Floating hypothesis does not match stack");
#ifdef PROOF_VERBOSE_DEBUG
            if (stack_hyp_sent.size() == 1) {
                cerr << "[" << this->debug_output << "] Matching an empty sentence" << endl;
            }
#endif
            const VarType subst_var = TraitsType::floating_to_var(this->lib, hyp);
            const SentType &subst_sent = stack_hyp_sent;
            auto res = subst_map.insert(std::make_pair(subst_var, subst_sent));
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
            std::copy(this->dists_stack.at(stack_base + i).begin(), this->dists_stack.at(stack_base + i).end(), std::inserter(dists, dists.begin()));
            TraitsType::check_match(this->lib, label, stack_hyp_sent, hyp_sent, subst_map);
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
                if (child_dists.find(std::minmax(var1, var2)) != child_dists.end()) {
                    for (auto tok1 : TraitsType::get_variable_iterator(this->lib, subst1)) {
                        if (!TraitsType::is_variable(this->lib, tok1)) {
                            continue;
                        }
                        for (auto tok2 : TraitsType::get_variable_iterator(this->lib, subst2)) {
                            if (!TraitsType::is_variable(this->lib, tok2)) {
                                continue;
                            }
                            assert_or_throw< ProofException< SentType_ > >(tok1 != tok2, "Distinct variable constraint violated");
                            dists.insert(std::minmax(tok1, tok2));
                        }
                    }
                }
            }
        }

        // Build the thesis
        LabTok thesis = child_ass.get_thesis();
        const SentType &thesis_sent = TraitsType::get_sentence(this->lib, thesis);
        SentType stack_thesis_sent = TraitsType::substitute(this->lib, thesis_sent, subst_map);
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
            std::vector< ProofTree< SentType_ > > children(this->tree_stack.begin() + stack_base, this->tree_stack.end());
            this->tree_stack.resize(stack_base);
            this->proof_tree = { stack_thesis_sent, label, children, dists, true, child_ass.get_number() };
            this->tree_stack.push_back(this->proof_tree);
        }
        this->proof.push_back(label);
    }

    void process_sentence(const SentType &sent, LabTok label = 0)
    {
        this->push_stack(sent, {});
        if (this->gen_proof_tree) {
            this->proof_tree = { sent, label, {}, {}, true, 0 };
            this->tree_stack.push_back(this->proof_tree);
        }
        this->proof.push_back(label);
    }

    void process_label(const LabTok label)
    {
#ifdef PROOF_VERBOSE_DEBUG
        cerr << "  Considering label " << this->lib.resolve_label(label);
#endif
        try {
            const Assertion &child_ass = this->lib.get_assertion(label);
            if (child_ass.is_valid()) {
#ifdef PROOF_VERBOSE_DEBUG
                cerr << ", which is a previous assertion" << endl;
#endif
                this->process_assertion(child_ass, label);
                return;
            }
        } catch (std::out_of_range&) {
            // We could not find the assertion, so we carry on with the following possibilities
        }
        const auto *sentp = this->get_sentence(label);
        if (sentp != NULL) {
#ifdef PROOF_VERBOSE_DEBUG
            cerr << ", which is an internal hypothesis" << endl;
#endif
            const auto &sent = *sentp;
            this->process_sentence(sent, label);
        } else {
#ifdef PROOF_VERBOSE_DEBUG
            cerr << ", which is an hypothesis" << endl;
#endif
            const auto &sent = TraitsType::get_sentence(this->lib, label);
            this->process_sentence(sent, label);
        }
    }

    void checkpoint()
    {
        this->checkpoints.emplace_back(this->stack.size(), this->proof.size(), this->saved_steps.size());
    }

    void commit()
    {
        this->checkpoints.pop_back();
    }

    void rollback()
    {
        this->stack.resize(std::get<0>(this->checkpoints.back()));
        this->dists_stack.resize(std::get<0>(this->checkpoints.back()));
        this->proof.resize(std::get<1>(this->checkpoints.back()));
        this->saved_steps.resize(std::get<2>(this->checkpoints.back()));
        this->checkpoints.pop_back();
    }

    // Left to subclasses implementation
    virtual const SentType *get_sentence(LabTok label) {
        (void) label;

        return NULL;
    }

private:
    void push_stack(const SentType &sent, const std::set<std::pair<VarType, VarType> > &dists)
    {
        this->stack.push_back(sent);
        this->dists_stack.push_back(dists);
    }
    void stack_resize(size_t size)
    {
        this->stack.resize(size);
        this->dists_stack.resize(size);
        this->check_stack_underflow();
    }
    void pop_stack()
    {
        this->stack.pop_back();
        this->dists_stack.pop_back();
        this->check_stack_underflow();
    }
    void check_stack_underflow()
    {
        if (!this->checkpoints.empty() && this->stack.size() < std::get<0>(this->checkpoints.back())) {
            throw MMPPException("Checkpointed context exited without committing or rolling back");
        }
    }

    const LibType &lib;
    bool gen_proof_tree;
    std::vector< SentType > stack;
    std::vector< std::set< std::pair< VarType, VarType > > > dists_stack;
    std::vector< SentType > saved_steps;
    std::vector< ProofTree< SentType_ > > tree_stack;
    ProofTree< SentType_ > proof_tree;
    //std::set< std::pair< SymTok, SymTok > > dists;
    std::vector< LabTok > proof;
    //std::vector< std::tuple< size_t, std::set< std::pair< SymTok, SymTok > >, size_t > > checkpoints;
    std::vector< std::tuple< size_t, size_t, size_t > > checkpoints;
    std::string debug_output;
};

class ProofEngine {
public:
    virtual void process_label(const LabTok label) = 0;
};

template< typename SentType_ >
class CreativeProofEngine : virtual public ProofEngine {
public:
    virtual void process_new_hypothesis(const typename ProofEngineBase< SentType_ >::SentType &sent) = 0;
};

class CheckpointedProofEngine : virtual public ProofEngine {
public:
    virtual void checkpoint() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
};

template< typename SentType_ >
class CreativeCheckpointedProofEngine : virtual public CheckpointedProofEngine, virtual public CreativeProofEngine< SentType_ > {
};

template< typename SentType_ >
class SemiCreativeProofEngineImpl : public ProofEngineBase< SentType_ >, virtual public ProofEngine {
public:
    SemiCreativeProofEngineImpl(const typename ProofEngineBase< SentType_ >::LibType &lib, bool gen_proof_tree = false) : ProofEngineBase< SentType_ >(lib, gen_proof_tree) {}

    void process_label(const LabTok label) override {
        this->ProofEngineBase< SentType_ >::process_label(label);
    }

    const typename ProofEngineBase< SentType_ >::SentType *get_sentence(LabTok label) override {
        auto it = this->new_hypotheses.find(label);
        if (it != this->new_hypotheses.end()) {
            return &it->second;
        } else {
            return NULL;
        }
    }

    void set_new_hypothesis(LabTok label, const typename ProofEngineBase< SentType_ >::SentType &sent) {
        this->new_hypotheses[label] = sent;
    }

protected:
    std::map< LabTok, typename ProofEngineBase< SentType_ >::SentType > new_hypotheses;
};

template< typename SentType_ >
class CreativeProofEngineImpl final : public SemiCreativeProofEngineImpl< SentType_ >, virtual public CreativeCheckpointedProofEngine< SentType_ > {
public:
    CreativeProofEngineImpl(const typename ProofEngineBase< SentType_ >::AdvLibType &lib, bool gen_proof_tree = false) : SemiCreativeProofEngineImpl< SentType_ >(lib, gen_proof_tree), lib(lib) {}

    LabTok create_new_hypothesis(const typename ProofEngineBase< SentType_ >::SentType &sent);

    void process_new_hypothesis(const typename ProofEngineBase< SentType_ >::SentType &sent) {
        LabTok label = this->create_new_hypothesis(sent);
        this->ProofEngineBase< SentType_ >::process_label(label);
    }

    void checkpoint() {
        this->ProofEngineBase< SentType_ >::checkpoint();
    }

    void commit() {
        this->ProofEngineBase< SentType_ >::commit();
    }

    void rollback() {
        this->ProofEngineBase< SentType_ >::rollback();
    }

    const std::map< LabTok, typename ProofEngineBase< SentType_ >::SentType > &get_new_hypotheses() const {
        return this->new_hypotheses;
    }

private:
    const typename ProofEngineBase< SentType_ >::AdvLibType &lib;
    std::map< typename ProofEngineBase< SentType_ >::SentType, LabTok > new_hypotheses_rev;
};

template<typename SentType_>
LabTok CreativeProofEngineImpl< SentType_ >::create_new_hypothesis(const typename ProofEngineBase< SentType_ >::SentType &sent) {
    auto it = this->new_hypotheses_rev.find(sent);
    LabTok label = it->second;
    if (it == this->new_hypotheses_rev.end()) {
        size_t num = this->new_hypotheses_rev.size();
        std::ostringstream buf;
        buf << "hypothesis." << num;
        auto name = buf.str();
        label = this->lib.new_temp_label(name);
        this->new_hypotheses_rev[sent] = label;
        this->set_new_hypothesis(label, sent);
    }
    return label;
}

template< typename SentType_ >
class ProofEngineImpl final : public ProofEngineBase< SentType_ >, virtual public ProofEngine {
public:
    ProofEngineImpl(const typename ProofEngineBase< SentType_ >::LibType &lib, bool gen_proof_tree = false) : ProofEngineBase< SentType_ >(lib, gen_proof_tree) {}

    void process_label(const LabTok label) {
        this->ProofEngineBase< SentType_ >::process_label(label);
    }
};

