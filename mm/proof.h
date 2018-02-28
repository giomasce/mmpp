#pragma once

#include <vector>
#include <functional>
#include <limits>
#include <type_traits>
#include <memory>

#include "utils/vectormap.h"
#include "funds.h"
#include "utils/utils.h"
#include "sentengine.h"
#include "mmtypes.h"

template< typename SentType_ >
class ProofExecutor {
public:
    enum CompressionStrategy {
        CS_ANY,
        CS_NO_BACKREFS,
        CS_BACKREFS_ON_IDENTICAL_TREE,
        CS_BACKREFS_ON_IDENTICAL_SENTENCE,
    };

    const std::vector< std::vector< SymTok > > &get_stack() const
    {
        return this->engine.get_stack();
    }
    const ProofTree< Sentence > &get_proof_tree() const
    {
        return this->engine.get_proof_tree();
    }
    const std::vector< LabTok > &get_proof_labels() const
    {
        return this->engine.get_proof_labels();
    }
    void set_debug_output(std::string debug_output)
    {
        this->engine.set_debug_output(debug_output);
    }
    virtual void execute() = 0;
    virtual ~ProofExecutor()
    {
    }

    void set_new_hypothesis(LabTok label, const typename ProofEngineBase< SentType_ >::SentType &sent) {
        return this->engine.set_new_hypothesis(label, sent);
    }

protected:
    ProofExecutor(const Library &lib, const Assertion &ass, bool gen_proof_tree) :
        lib(lib), ass(ass), engine(lib, gen_proof_tree), relax_checks(false) {
    }

    void process_label(const LabTok label)
    {
        if (!this->relax_checks) {
            const Assertion &child_ass = this->lib.get_assertion(label);
            if (!child_ass.is_valid()) {
                // In line of principle searching in a set would be faster, but since usually hypotheses are not many the vector is probably better
                assert_or_throw< ProofException< SentType_ > >(find(this->ass.get_float_hyps().begin(), this->ass.get_float_hyps().end(), label) != this->ass.get_float_hyps().end() ||
                        find(this->ass.get_ess_hyps().begin(), this->ass.get_ess_hyps().end(), label) != this->ass.get_ess_hyps().end() ||
                        find(this->ass.get_opt_hyps().begin(), this->ass.get_opt_hyps().end(), label) != this->ass.get_opt_hyps().end(),
                                                              "Requested label cannot be used by this theorem");
            }
        }
        this->engine.process_label(label);
    }

    size_t save_step() {
        return this->engine.save_step();
    }

    void process_saved_step(size_t step_num) {
        this->engine.process_saved_step(step_num);
    }

    void final_checks() const
    {
        if (!this->relax_checks) {
            assert_or_throw< ProofException< SentType_ > >(this->get_stack().size() == 1, "Proof execution did not end with a single element on the stack");
            assert_or_throw< ProofException< SentType_ > >(this->get_stack().at(0) == this->lib.get_sentence(this->ass.get_thesis()), "Proof does not prove the thesis");
            assert_or_throw< ProofException< SentType_ > >(includes(this->ass.get_dists().begin(), this->ass.get_dists().end(),
                                                                   this->engine.get_dists().begin(), this->engine.get_dists().end()),
                                                          "Distinct variables constraints are too wide");
        }
    }

    void set_relax_checks(bool x) {
        this->relax_checks = x;
    }

    const Library &lib;
    const Assertion &ass;
    SemiCreativeProofEngineImpl< SentType_ > engine;
    bool relax_checks;
};

template< typename SentType_ >
class CompressedProofExecutor : virtual public ProofExecutor< SentType_ > {
public:
    CompressedProofExecutor(const Library &lib, const Assertion &ass, const CompressedProof &proof, bool gen_proof_tree=false) :
        ProofExecutor< SentType_ >(lib, ass, gen_proof_tree), proof(proof)
    {
    }
    void execute();

protected:
    const CompressedProof &proof;
};

template< typename SentType_ >
class UncompressedProofExecutor : virtual public ProofExecutor< SentType_ > {
public:
    UncompressedProofExecutor(const Library &lib, const Assertion &ass, const UncompressedProof &proof, bool gen_proof_tree=false) :
        ProofExecutor< SentType_ >(lib, ass, gen_proof_tree), proof(proof)
    {
    }
    void execute();

protected:
    const UncompressedProof &proof;
};

class ProofOperator : virtual public ProofExecutor< Sentence > {
public:
    enum CompressionStrategy {
        CS_ANY,
        CS_NO_BACKREFS,
        CS_BACKREFS_ON_IDENTICAL_TREE,
        CS_BACKREFS_ON_IDENTICAL_SENTENCE,
    };

    virtual const CompressedProof compress(CompressionStrategy strategy=CS_ANY) = 0;
    virtual const UncompressedProof uncompress() = 0;
    virtual bool check_syntax() = 0;
    /*
     * A proof is trivial if it has only one essential step
     * (which can either be an hypothesis or a previous assertion).
     */
    virtual bool is_trivial() const = 0;
    virtual ~ProofOperator();

protected:
    // Useless constructor required by g++-6
    ProofOperator(const Library &lib, const Assertion &ass, bool gen_proof_tree=false);
    size_t get_hyp_num(const LabTok label) const;
};

class CompressedProofOperator : public ProofOperator, public CompressedProofExecutor< Sentence > {
public:
    CompressedProofOperator(const Library &lib, const Assertion &ass, const CompressedProof &proof);
    const CompressedProof compress(CompressionStrategy strategy=CS_ANY);
    const UncompressedProof uncompress();
    bool check_syntax();
    bool is_trivial() const;
};

class UncompressedProofOperator : public ProofOperator, public UncompressedProofExecutor< Sentence > {
public:
    UncompressedProofOperator(const Library &lib, const Assertion &ass, const UncompressedProof &proof);
    const CompressedProof compress(CompressionStrategy strategy=CS_ANY);
    const UncompressedProof uncompress();
    bool check_syntax();
    bool is_trivial() const;
};

class Proof {
public:
    template< typename SentType_ >
    std::shared_ptr< ProofExecutor< SentType_ > > get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree=false) const;
    virtual std::shared_ptr< ProofOperator > get_operator(const Library &lib, const Assertion &ass) const = 0;
    virtual ~Proof();
};

class CompressedProof : public Proof {
public:
    CompressedProof(const std::vector< LabTok > &refs, const std::vector< CodeTok > &codes);
    template< typename SentType_ >
    std::shared_ptr< ProofExecutor< SentType_ > > get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree=false) const;
    std::shared_ptr< ProofOperator > get_operator(const Library &lib, const Assertion &ass) const;
    const std::vector< LabTok > &get_refs() const;
    const std::vector< CodeTok > &get_codes() const;

private:
    const std::vector< LabTok > refs;
    const std::vector< CodeTok > codes;
};

class UncompressedProof : public Proof {
public:
    UncompressedProof(const std::vector< LabTok > &labels);
    template< typename SentType_ >
    std::shared_ptr< ProofExecutor< SentType_ > > get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree=false) const;
    std::shared_ptr< ProofOperator > get_operator(const Library &lib, const Assertion &ass) const;
    const std::vector< LabTok > &get_labels() const;

private:
    const std::vector< LabTok > labels;
};

class CompressedEncoder {
public:
    std::string push_code(CodeTok x);
};

class CompressedDecoder {
public:
    CodeTok push_char(char c);

private:
    uint32_t current = 0;
};

template<typename SentType_>
std::shared_ptr<ProofExecutor<SentType_> > Proof::get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree) const
{
    // Unfortunately here we have to enumerate the known descendants of Proof.
    auto comp_proof = dynamic_cast< const CompressedProof* >(this);
    if (comp_proof) {
        return comp_proof->get_executor< SentType_ >(lib, ass, gen_proof_tree);
    }
    auto uncomp_proof = dynamic_cast< const UncompressedProof* >(this);
    if (uncomp_proof) {
        return uncomp_proof->get_executor< SentType_ >(lib, ass, gen_proof_tree);
    }
    throw std::bad_cast();
}

template< typename SentType_ >
std::shared_ptr<ProofExecutor< SentType_ >> CompressedProof::get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree) const
{
    return std::make_shared< CompressedProofExecutor< SentType_ > >(lib, ass, *this, gen_proof_tree);
}

template< typename SentType_ >
std::shared_ptr<ProofExecutor< SentType_ >> UncompressedProof::get_executor(const Library &lib, const Assertion &ass, bool gen_proof_tree) const
{
    return std::make_shared< UncompressedProofExecutor< SentType_ > >(lib, ass, *this, gen_proof_tree);
}

template<typename SentType_>
void CompressedProofExecutor< SentType_ >::execute()
{
    //cerr << "Executing proof of " << this->lib.resolve_label(this->ass.get_thesis()) << endl;
    for (auto &code : this->proof.get_codes()) {
        if (code == 0) {
            this->save_step();
        } else if (code <= this->ass.get_mand_hyps_num()) {
            LabTok label = this->ass.get_mand_hyp(code-1);
            this->process_label(label);
        } else if (code <= this->ass.get_mand_hyps_num() + this->proof.get_refs().size()) {
            LabTok label = this->proof.get_refs().at(code-this->ass.get_mand_hyps_num()-1);
            this->process_label(label);
        } else {
            try {
                this->process_saved_step(code - this->ass.get_mand_hyps_num()-this->proof.get_refs().size()-1);
            } catch (std::out_of_range) {
                throw ProofException< Sentence >("Code too big in compressed proof");
            }
        }
    }
    this->final_checks();
}

template<typename SentType_>
void UncompressedProofExecutor< SentType_ >::execute()
{
    //cerr << "Executing proof of " << this->lib.resolve_label(this->ass.get_thesis()) << endl;
    for (auto &label : this->proof.get_labels()) {
        this->process_label(label);
    }
    this->final_checks();
}

template< typename SentType_ >
std::shared_ptr< ProofExecutor< SentType_ > > Assertion::get_proof_executor(const Library &lib, bool gen_proof_tree) const
{
    return this->proof->get_executor< SentType_ >(lib, *this, gen_proof_tree);
}
