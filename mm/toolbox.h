#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <string>
#include <memory>

#include <boost/filesystem.hpp>

#include <giolib/assert.h>
#include <giolib/stacktrace.h>

class LibraryToolbox;

#include "library.h"
#include "parsing/lr.h"
#include "parsing/unif.h"
#include "sentengine.h"
#include "mmtemplates.h"
#include "tempgen.h"

class LibraryToolbox;

struct SentenceTree {
    LabTok label;
    std::vector< SentenceTree > children;
};

template< typename Engine >
using Prover = std::function< bool(Engine&) >;
const Prover< ProofEngine > null_prover = [](ProofEngine&){ return false; };

Prover<ProofEngine> trivial_prover(LabTok label);

struct SentencePrinter {
    enum Style {
        STYLE_PLAIN,
        STYLE_NUMBERS,
        STYLE_HTML,
        STYLE_ALTHTML,
        STYLE_LATEX,
        STYLE_ANSI_COLORS_SET_MM,
    };
    const Sentence *sent;
    const ParsingTree<SymTok, LabTok> *pt;
    const ParsingTree2<SymTok, LabTok> *pt2;
    const LibraryToolbox &tb;
    const Style style;

    std::string to_string() const;
};
struct ProofPrinter {
    const std::vector< LabTok > *labels;
    const CompressedProof *comp_proof;
    const UncompressedProof *uncomp_proof;
    const LibraryToolbox &tb;
    bool only_assertions;

    std::string to_string() const;
};
std::ostream &operator<<(std::ostream &os, const SentencePrinter &sp);
std::ostream &operator<<(std::ostream &os, const ProofPrinter &sp);

static std::vector< size_t > invert_perm(const std::vector< size_t > &perm) {
    std::vector< size_t > ret(perm.size());
    for (size_t i = 0; i < perm.size(); i++) {
        ret[perm[i]] = i;
    }
    return ret;
}

struct RegisteredProver {
    size_t index;

    // Just for debug
    std::vector<std::string> templ_hyps;
    std::string templ_thesis;
};

struct RegisteredProverData {
    std::vector<std::string> templ_hyps;
    std::string templ_thesis;
};

struct RegisteredProverInstanceData {
    bool valid = false;
    LabTok label;
    std::vector< size_t > perm_inv;
    std::unordered_map< SymTok, std::vector< SymTok > > ass_map;

    // Just for debug
    std::string label_str;

    RegisteredProverInstanceData() = default;
    RegisteredProverInstanceData(const RegisteredProverInstanceData &x) = default;

    explicit RegisteredProverInstanceData(const std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, Sentence > > &data, std::string label_str = "")
        : valid(true), label(std::get<0>(data)), perm_inv(invert_perm(std::get<1>(data))), ass_map(std::get<2>(data)), label_str(label_str) {
    }
};

class ToolboxCache {
public:
    virtual ~ToolboxCache();
    virtual bool load() = 0;
    virtual bool store() = 0;
    virtual std::string get_digest() = 0;
    virtual void set_digest(std::string digest) = 0;
    virtual LRParser< SymTok, LabTok >::CachedData get_lr_parser_data() = 0;
    virtual void set_lr_parser_data(const LRParser< SymTok, LabTok >::CachedData &cached_data) = 0;
};

class FileToolboxCache : public ToolboxCache {
public:
    FileToolboxCache(const boost::filesystem::path &filename);
    bool load() override;
    bool store() override;
    std::string get_digest() override;
    void set_digest(std::string digest) override;
    LRParser< SymTok, LabTok >::CachedData get_lr_parser_data() override;
    void set_lr_parser_data(const LRParser< SymTok, LabTok >::CachedData &cached_data) override;

private:
    boost::filesystem::path filename;
    std::string digest;
    LRParser< SymTok, LabTok >::CachedData lr_parser_data;
};

class temp_allocator {
public:
    virtual ~temp_allocator() = default;
    virtual std::pair<LabTok, SymTok> new_temp_var(SymTok type) = 0;
};

class LibraryToolbox : public Library
{
public:
    explicit LibraryToolbox(const ExtendedLibrary &lib, std::string turnstile, std::shared_ptr< ToolboxCache > cache = nullptr);
private:
    void compute_everything();
    std::shared_ptr< ToolboxCache > cache;

    // Essentials
public:
    const ExtendedLibrary &get_library() const;
    const std::function<bool (LabTok)> &get_standard_is_var() const;
    const std::function<bool (SymTok)> &get_standard_is_var_sym() const;
    const std::function< std::pair< SymTok, std::vector< SymTok > >(LabTok) > &get_validation_rule() const;
    SymTok get_turnstile() const;
    SymTok get_turnstile_alias() const;
    LabTok get_imp_label() const;
private:
    const ExtendedLibrary &lib;
    std::function< bool(LabTok) > standard_is_var;
    std::function< bool(SymTok) > standard_is_var_sym;
    std::function< std::pair< SymTok, std::vector< SymTok > >(LabTok) > validation_rule;

    SymTok turnstile;
    SymTok turnstile_alias;

    // Types
/*public:
    const std::vector< LabTok > &get_type_labels() const;
    const std::set< LabTok > &get_type_labels_set() const;
private:
    std::vector< LabTok > type_labels;
    std::set< LabTok > type_labels_set;*/

    // Labels and symbols
public:
    Generator< SymTok > gen_symbols() const;
    Generator< LabTok > gen_labels() const;

    // Type proving
public:
    template< typename Engine = ProofEngine, typename std::enable_if< std::is_base_of< ProofEngine, Engine >::value >::type* = nullptr >
    Prover< Engine > build_type_prover(const std::vector< SymTok > &type_sent, const std::unordered_map< SymTok, Prover< Engine > > &var_provers = {}) const
    {
        return [=](Engine &engine){
            return this->type_proving_helper(type_sent, engine, var_provers);
        };
    }
    template< typename Engine = ProofEngine, typename std::enable_if< std::is_base_of< ProofEngine, Engine >::value >::type* = nullptr >
    Prover< Engine > build_type_prover(const ParsingTree2< SymTok, LabTok > &pt, const std::unordered_map< LabTok, Prover< Engine > > &var_provers = {}) const
    {
        return [=](Engine &engine){
            return this->type_proving_helper(pt, engine, var_provers);
        };
    }
private:
    template< typename Engine = ProofEngine, typename std::enable_if< std::is_base_of< ProofEngine, Engine >::value >::type* = nullptr >
    void type_proving_helper_unwind_tree(const ParsingTree< SymTok, LabTok > &tree, Engine &engine, const std::unordered_map<LabTok, const Prover< Engine >* > &var_provers) const {
        // We need to sort children according to their order as floating hypotheses of this assertion
        // If this is not an assertion, then there are no children
        const Assertion &ass = this->get_assertion(tree.label);
        auto it = var_provers.find(tree.label);
        if (ass.is_valid()) {
            assert(it == var_provers.end());
            std::unordered_map< SymTok, const ParsingTree< SymTok, LabTok >* > children;
            auto it2 = tree.children.begin();
            for (auto &tok : this->lib.get_sentence(tree.label)) {
                if (!this->lib.is_constant(tok)) {
                    children[tok] = &(*it2);
                    it2++;
                }
            }
            assert(it2 == tree.children.end());
            for (auto &hyp : ass.get_float_hyps()) {
                SymTok tok = this->lib.get_sentence(hyp).at(1);
                this->type_proving_helper_unwind_tree(*children.at(tok), engine, var_provers);
            }
            engine.process_label(tree.label);
        } else {
            assert(tree.children.size() == 0);
            if (it == var_provers.end()) {
                engine.process_label(tree.label);
            } else {
                (*it->second)(engine);
            }
        }
    }
public:
    template< typename Engine = ProofEngine, typename std::enable_if< std::is_base_of< ProofEngine, Engine >::value >::type* = nullptr >
    bool type_proving_helper(const std::vector< SymTok > &type_sent, Engine &engine, const std::unordered_map< SymTok, Prover< Engine > > &var_provers = {}) const
    {
        SymTok type = type_sent[0];
        auto tree = this->parse_sentence(type_sent.begin()+1, type_sent.end(), type);
        if (tree.label == LabTok{}) {
            return false;
        } else {
            std::unordered_map<LabTok, const Prover< Engine >* > lab_var_provers;
            for (const auto &x : var_provers) {
                lab_var_provers.insert(std::make_pair(this->get_var_sym_to_lab(x.first), &x.second));
            }
            this->type_proving_helper_unwind_tree(tree, engine, lab_var_provers);
            return true;
        }
    }
    template< typename Engine = ProofEngine, typename std::enable_if< std::is_base_of< ProofEngine, Engine >::value >::type* = nullptr >
    bool type_proving_helper(const ParsingTree2< SymTok, LabTok > &pt, Engine &engine, const std::unordered_map< LabTok, Prover< Engine > > &var_provers = {}) const
    {
        std::unordered_map<LabTok, const Prover< Engine >* > var_provers_ptr;
        for (const auto &x : var_provers) {
            var_provers_ptr.insert(make_pair(x.first, &x.second));
        }
        this->type_proving_helper_unwind_tree(pt2_to_pt(pt), engine, var_provers_ptr);
        return true;
    }

    // Assertion unification
public:
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, Sentence > > > unify_assertion(const std::vector< Sentence > &hypotheses, const Sentence &thesis, bool just_first=true, bool up_to_hyps_perms=true, const std::set< std::pair< SymTok, SymTok > > &antidists = {}) const;
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, Sentence > > > unify_assertion(const std::vector< std::pair< SymTok, ParsingTree< SymTok, LabTok > > > &hypotheses, const std::pair< SymTok, ParsingTree< SymTok, LabTok > > &thesis, bool just_first=true, bool up_to_hyps_perms=true, const std::set< std::pair< SymTok, SymTok > > &antidists = {}) const;

    // Reading and printing
public:
    std::vector< SymTok > read_sentence(const std::string &in) const;
    SentencePrinter print_sentence(const std::vector< SymTok > &sent, SentencePrinter::Style style=SentencePrinter::STYLE_PLAIN) const;
    SentencePrinter print_sentence(const ParsingTree< SymTok, LabTok > &pt, SentencePrinter::Style style=SentencePrinter::STYLE_PLAIN) const;
    SentencePrinter print_sentence(const ParsingTree2< SymTok, LabTok > &pt, SentencePrinter::Style style=SentencePrinter::STYLE_PLAIN) const;
    ProofPrinter print_proof(const std::vector< LabTok > &proof, bool only_assertions = false) const;
    ProofPrinter print_proof(const CompressedProof &proof, bool only_assertions = false) const;
    ProofPrinter print_proof(const UncompressedProof &proof, bool only_assertions = false) const;
    Sentence reconstruct_sentence(const ParsingTree< SymTok, LabTok > &pt, SymTok first_sym = {}) const;
private:
    void reconstruct_sentence_internal(const ParsingTree< SymTok, LabTok > &pt, std::back_insert_iterator< std::vector< SymTok > > it) const;

    // Type correspondance
public:
    LabTok get_var_sym_to_lab(SymTok sym) const;
    SymTok get_var_lab_to_sym(LabTok lab) const;
    SymTok get_var_sym_to_type_sym(SymTok sym) const;
    SymTok get_var_lab_to_type_sym(LabTok lab) const;
private:
    void compute_type_correspondance();
    std::vector< LabTok > var_sym_to_lab;
    std::vector< SymTok > var_lab_to_sym;
    std::vector< SymTok > var_sym_to_type_sym;
    std::vector< SymTok > var_lab_to_type_sym;

    // Quick bitmap to know which types correspond to variables
public:
    const std::vector< bool > &get_is_var_by_type() const;
private:
    void compute_is_var_by_type();
    std::vector< bool > is_var_by_type;

    // Assertions sorted according to the type of their thesis
public:
    const std::unordered_map< SymTok, std::vector< LabTok > > &get_assertions_by_type() const;
private:
    void compute_assertions_by_type();
    std::unordered_map< SymTok, std::vector< LabTok > > assertions_by_type;

    // Formal grammar derivations
public:
    const std::unordered_map<SymTok, std::vector<std::pair<LabTok, std::vector<SymTok> > > > &get_derivations() const;
private:
    void compute_derivations();
    std::unordered_map<SymTok, std::vector<std::pair< LabTok, std::vector<SymTok> > > > derivations;

    // Derivations sorted according to their label
public:
    const std::pair< SymTok, Sentence > &get_derivation_rule(LabTok lab) const;
private:
    void compute_ders_by_label();
    std::unordered_map< LabTok, std::pair< SymTok, std::vector< SymTok > > > ders_by_label;

    // Variables in sentences and assertions
public:
    const std::vector< std::set< LabTok > > &get_sentence_vars() const;
    const std::vector< std::set< LabTok > > &get_assertion_unconst_vars() const;
    const std::vector< std::set< LabTok > > &get_assertion_const_vars() const;
private:
    void compute_vars();
    std::vector< std::set< LabTok > > sentence_vars;
    std::vector< std::set< LabTok > > assertion_unconst_vars;
    std::vector< std::set< LabTok > > assertion_const_vars;

    // Lookup tables for assertions' theses sorted according their root grammar derivation
public:
    const std::unordered_map< LabTok, std::vector< LabTok > > &get_root_labels_to_theses() const;
    const std::unordered_map< LabTok, std::vector< LabTok > > &get_imp_ant_labels_to_theses() const;
    const std::unordered_map< LabTok, std::vector< LabTok > > &get_imp_con_labels_to_theses() const;
private:
    void compute_labels_to_theses();
    std::unordered_map< LabTok, std::vector< LabTok > > root_labels_to_theses;
    std::unordered_map< LabTok, std::vector< LabTok > > imp_ant_labels_to_theses;
    std::unordered_map< LabTok, std::vector< LabTok > > imp_con_labels_to_theses;

    // LR parsing
public:
    const LRParser< SymTok, LabTok > &get_parser() const;
    ParsingTree< SymTok, LabTok > parse_sentence(typename Sentence::const_iterator sent_begin, typename Sentence::const_iterator sent_end, SymTok type) const;
    ParsingTree< SymTok, LabTok > parse_sentence(const Sentence &sent, SymTok type) const;
    ParsingTree< SymTok, LabTok > parse_sentence(const Sentence &sent) const;
private:
    void compute_parser_initialization();
    std::unique_ptr< LRParser< SymTok, LabTok > > parser;

    // Preparsed sentences
public:
    const ParsingTree< SymTok, LabTok > &get_parsed_sent(LabTok label) const;
    const ParsingTree2<SymTok, LabTok> &get_parsed_sent2(LabTok label) const;
    const std::vector<std::pair< ParsingTreeMultiIterator< SymTok, LabTok >::Status, ParsingTreeNode< SymTok, LabTok > > > &get_parsed_iter(LabTok label) const;
    Generator<std::reference_wrapper<const ParsingTree<SymTok, LabTok> > > gen_parsed_sents() const;
    Generator<std::reference_wrapper<const ParsingTree2<SymTok, LabTok> > > gen_parsed_sents2() const;
    Generator<std::pair<LabTok, std::reference_wrapper<const ParsingTree<SymTok, LabTok> > > > enum_parsed_sents() const;
    Generator<std::pair<LabTok, std::reference_wrapper<const ParsingTree2<SymTok, LabTok> > > > enum_parsed_sents2() const;
private:
    void compute_sentences_parsing();
    std::vector< ParsingTree< SymTok, LabTok > > parsed_sents;
    std::vector< ParsingTree2< SymTok, LabTok > > parsed_sents2;
    std::vector< std::vector< std::pair< ParsingTreeMultiIterator< SymTok, LabTok >::Status, ParsingTreeNode< SymTok, LabTok > > > > parsed_iters;

    // Provers utilities
public:
    LabTok get_registered_prover_label(const RegisteredProver &prover) const;
    static RegisteredProver register_prover(const std::vector< std::string > &templ_hyps, const std::string &templ_thesis);
    // From https://stackoverflow.com/a/30687399/807307
    template< typename Engine = CheckpointedProofEngine, typename std::enable_if< std::is_base_of< CheckpointedProofEngine, Engine >::value >::type* = nullptr >
    Prover< Engine > build_registered_prover(const RegisteredProver &prover, const std::unordered_map< std::string, Prover< Engine > > &types_provers, const std::vector< Prover< Engine > > &hyps_provers) const
    {
        const size_t &index = prover.index;
        if (index >= this->instance_registered_provers.size() || !this->instance_registered_provers[index].valid) {
            throw std::runtime_error("cannot modify const object");
        }
        const RegisteredProverInstanceData &inst_data = this->instance_registered_provers[index];

        return [=](Engine &engine){
            return this->proving_helper(inst_data, types_provers, hyps_provers, engine);
        };
    }
    void compute_registered_provers();
    template< typename Engine = CheckpointedProofEngine, typename std::enable_if< std::is_base_of< CheckpointedProofEngine, Engine >::value >::type* = nullptr >
    Prover< Engine > build_prover(const std::vector< Sentence > &templ_hyps,
                        const Sentence &templ_thesis,
                        const std::unordered_map< std::string, Prover< Engine > > &types_provers,
                        const std::vector< Prover< Engine > > &hyps_provers) const
    {
        auto res = this->unify_assertion(templ_hyps, templ_thesis, true);
        if (res.empty()) {
            std::cerr << std::string("Could not find the template assertion: ") + this->print_sentence(templ_thesis).to_string() << std::endl;
        }
        gio::assert_or_throw< std::runtime_error >(!res.empty(), std::string("Could not find the template assertion: ") + this->print_sentence(templ_thesis).to_string());
        const auto &res1 = res[0];
        return [=](Engine &engine){
            RegisteredProverInstanceData inst_data(res1);
            return this->proving_helper(inst_data, types_provers, hyps_provers, engine);
        };
    }
    template< typename Engine = CheckpointedProofEngine, typename std::enable_if< std::is_base_of< CheckpointedProofEngine, Engine >::value >::type* = nullptr >
    bool proving_helper(const RegisteredProverInstanceData &inst_data, const std::unordered_map< std::string, Prover< Engine > > &types_provers, const std::vector< Prover< Engine > > &hyps_provers, Engine &engine) const {
        std::unordered_map<SymTok, Prover< Engine > > types_provers_sym;
        for (auto &type_pair : types_provers) {
            auto res = types_provers_sym.insert(make_pair(this->get_symbol(type_pair.first), type_pair.second));
            assert(res.second);
        }
        return this->proving_helper(inst_data, types_provers_sym, hyps_provers, engine);
    }
    template< typename Engine = CheckpointedProofEngine, typename std::enable_if< std::is_base_of< CheckpointedProofEngine, Engine >::value >::type* = nullptr >
    bool proving_helper(const RegisteredProverInstanceData &inst_data, const std::unordered_map< SymTok, Prover< Engine > > &types_provers, const std::vector< Prover< Engine > > &hyps_provers, Engine &engine) const
    {
        const Assertion &ass = this->get_assertion(inst_data.label);
        assert(ass.is_valid());

        engine.checkpoint();

        // Compute floating hypotheses
        for (auto &hyp : ass.get_float_hyps()) {
            bool res = this->type_proving_helper(substitute(this->get_sentence(hyp), inst_data.ass_map, this->get_standard_is_var_sym()), engine, types_provers);
            if (!res) {
                //std::cerr << "Applying " << inst_data.label_str << " a floating hypothesis failed..." << std::endl;
                engine.rollback();
                return false;
            }
        }

        // Compute essential hypotheses
        for (size_t i = 0; i < ass.get_ess_hyps().size(); i++) {
            bool res = hyps_provers[inst_data.perm_inv[i]](engine);
            if (!res) {
                //std::cerr << "Applying " << inst_data.label_str << " an essential hypothesis failed..." << std::endl;
                engine.rollback();
                return false;
            }
        }

        // Finally add this assertion's label
        try {
            engine.process_label(ass.get_thesis());
        } catch (const ProofException< Sentence > &e) {
            this->dump_proof_exception(e, std::cerr);
            engine.rollback();
            return false;
        }

        engine.commit();
        return true;
    }
private:
    void compute_registered_prover(size_t i, bool exception_on_failure = true);
    // This is an instance of the Construct On First Use idiom, which prevents the static initialization fiasco;
    // see https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use-members
    static std::vector< RegisteredProverData > &registered_provers() {
        static auto ret = std::make_unique< std::vector< RegisteredProverData > >();
        return *ret;
    }
    std::vector< RegisteredProverInstanceData > instance_registered_provers;

    // Dynamic generation of temporary variables and labels
public:
    std::pair< LabTok, SymTok > new_temp_var(SymTok type_sym) const;
    LabTok new_temp_label(std::string name) const;
    void release_temp_var(LabTok lab) const;
private:
    std::unique_ptr< TempGenerator > temp_generator;

    // Replace variables with fresh new ones
public:
    std::pair< std::vector< ParsingTree< SymTok, LabTok > >, ParsingTree< SymTok, LabTok > > refresh_assertion(const Assertion &ass, temp_allocator &ta) const;
    std::pair< std::vector< ParsingTree2< SymTok, LabTok > >, ParsingTree2< SymTok, LabTok > > refresh_assertion2(const Assertion &ass, temp_allocator &ta) const;
    ParsingTree< SymTok, LabTok > refresh_parsing_tree(const ParsingTree< SymTok, LabTok > &pt, temp_allocator &ta) const;
    ParsingTree2<SymTok, LabTok> refresh_parsing_tree2(const ParsingTree2< SymTok, LabTok > &pt, temp_allocator &ta) const;
    std::pair<SubstMap<SymTok, LabTok>, std::set<LabTok> > build_refreshing_subst_map(const std::set< LabTok > &vars, temp_allocator &ta) const;
    SimpleSubstMap2< SymTok, LabTok > build_refreshing_subst_map2(const std::set< LabTok > &vars, temp_allocator &ta) const;
    std::pair<SubstMap2<SymTok, LabTok>, std::set<LabTok> > build_refreshing_full_subst_map2(const std::set< LabTok > &vars, temp_allocator &ta) const;

    // Misc
public:
    void dump_proof_exception(const ProofException< Sentence > &e, std::ostream &out) const;

    // Library interface
public:
    SymTok get_symbol(std::string s) const override;
    LabTok get_label(std::string s) const override;
    std::string resolve_symbol(SymTok tok) const override;
    std::string resolve_label(LabTok tok) const override;
    size_t get_symbols_num() const override;
    size_t get_labels_num() const override;
    bool is_constant(SymTok c) const override;
    const Sentence &get_sentence(LabTok label) const override;
    SentenceType get_sentence_type(LabTok label) const override;
    const Assertion &get_assertion(LabTok label) const override;
    //std::function< const Assertion*() > list_assertions() const;
    Generator< std::reference_wrapper< const Assertion > > gen_assertions() const override;
    const StackFrame &get_final_stack_frame() const override;
    const LibraryAddendum &get_addendum() const override;
    const ParsingAddendumImpl &get_parsing_addendum() const override;
};

template< typename Engine, typename std::enable_if< std::is_base_of< ProofEngine, Engine >::value >::type* = nullptr >
Prover< Engine > cascade_provers(const Prover< Engine > &a,  const Prover< Engine > &b)
{
    return [=](Engine &engine) {
        bool res;
        res = a(engine);
        if (res) {
            return true;
        }
        res = b(engine);
        return res;
    };
}

template< typename Engine, typename std::enable_if< std::is_base_of< ProofEngine, Engine >::value >::type* = nullptr >
std::string test_prover(Prover< Engine > prover, const LibraryToolbox &tb) {
    Engine engine(tb);
    bool res = prover(engine);
    if (!res) {
        return "(prover failed)";
    } else {
        if (engine.get_stack().size() == 0) {
            return "(prover did not produce a result)";
        } else {
            std::string ret = tb.print_sentence(engine.get_stack().back()).to_string();
            if (engine.get_stack().size() > 1) {
                ret += " (prover did produce other stack entries)";
            }
            return ret;
        }
    }
}

template< typename SentType >
class CheckedProverException {
public:
    CheckedProverException(const SentType &on_stack, const SentType &expected, const gio::stacktrace &stacktrace) : on_stack(on_stack), expected(expected), stacktrace(stacktrace) {}
    const SentType &get_on_stack() const {
        return this->on_stack;
    }
    const SentType &get_expected() const {
        return this->expected;
    }
    const gio::stacktrace &get_stacktrace() const {
        return this->stacktrace;
    }

private:
    SentType on_stack;
    SentType expected;
    gio::stacktrace stacktrace;
};

template< typename Engine, typename std::enable_if< std::is_base_of< InspectableProofEngine< typename Engine::SentType >, Engine >::value >::type* = nullptr >
Prover< Engine > checked_prover(Prover< Engine > prover, typename Engine::SentType thesis)
{
    auto stacktrace = gio::get_stacktrace();
    return [=](Engine &engine)->bool {
        size_t stack_len_before = engine.get_stack().size();
        bool res = prover(engine);
        size_t stack_len_after = engine.get_stack().size();
#ifdef NDEBUG
        (void) stack_len_before;
        (void) stack_len_after;
#endif
        if (res) {
            assert(stack_len_after >= 1);
            assert(stack_len_after + 1 >= stack_len_before);
            gio::assert_or_throw< CheckedProverException< typename Engine::SentType > >(engine.get_stack().back() == thesis, engine.get_stack().back(), thesis, stacktrace);
        } else {
            assert(stack_len_after == stack_len_before);
        }
        return res;
    };
}

template< typename Engine, typename std::enable_if< std::is_base_of< InspectableProofEngine< typename Engine::SentType >, Engine >::value >::type* = nullptr >
Prover< Engine > checked_prover(Prover< Engine > prover, size_t hyp_num, typename Engine::SentType thesis)
{
    auto stacktrace = gio::get_stacktrace();
    return [=](Engine &engine)->bool {
        size_t stack_len_before = engine.get_stack().size();
        bool res = prover(engine);
        size_t stack_len_after = engine.get_stack().size();
        if (res) {
            assert(stack_len_after >= 1);
            assert(stack_len_after + 1 == stack_len_before + hyp_num);
            gio::assert_or_throw< CheckedProverException< typename Engine::SentType > >(engine.get_stack().back() == thesis, engine.get_stack().back(), thesis, stacktrace);
        } else {
            assert(stack_len_after == stack_len_before);
        }
        return res;
    };
}

class temp_stacked_allocator : public temp_allocator {
public:
    temp_stacked_allocator(const LibraryToolbox &tb);
    ~temp_stacked_allocator();
    std::pair<LabTok, SymTok> new_temp_var(SymTok type_sym);
    void new_temp_var_frame();
    void release_temp_var_frame();

private:
    std::vector<std::vector<LabTok>> stack;
    const LibraryToolbox &tb;
};
