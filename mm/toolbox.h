#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <string>

#include <boost/filesystem.hpp>

#include "library.h"
#include "parsing/lr.h"
#include "parsing/unif.h"
#include "engine.h"
#include "mmtemplates.h"

class LibraryToolbox;

struct SentenceTree {
    LabTok label;
    std::vector< SentenceTree > children;
};

typedef std::function< bool(ProofEngine&) > Prover;
const Prover null_prover = [](ProofEngine&){ return false; };
//Prover cascade_provers(const Prover &a, const Prover &b);

std::string test_prover(Prover prover, const LibraryToolbox &tb);
Prover checked_prover(Prover prover, size_t hyp_num, Sentence thesis);

struct SentencePrinter {
    enum Style {
        STYLE_PLAIN,
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
    const std::vector< LabTok > &proof;
    const LibraryToolbox &tb;
    bool only_assertions;

    std::string to_string() const;
};
std::ostream &operator<<(std::ostream &os, const SentencePrinter &sp);
std::ostream &operator<<(std::ostream &os, const ProofPrinter &sp);

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
    bool load();
    bool store();
    std::string get_digest();
    void set_digest(std::string digest);
    LRParser< SymTok, LabTok >::CachedData get_lr_parser_data();
    void set_lr_parser_data(const LRParser< SymTok, LabTok >::CachedData &cached_data);

private:
    boost::filesystem::path filename;
    std::string digest;
    LRParser< SymTok, LabTok >::CachedData lr_parser_data;
};

class LibraryToolbox : public Library
{
public:
    explicit LibraryToolbox(const ExtendedLibrary &lib, std::string turnstile, bool compute = false, std::shared_ptr< ToolboxCache > cache = NULL);
    ~LibraryToolbox();
    const ExtendedLibrary &get_library() const {
        return this->lib;
    }
    void set_cache(std::shared_ptr< ToolboxCache > cache);
    std::vector< SymTok > substitute(const std::vector< SymTok > &orig, const std::unordered_map< SymTok, std::vector< SymTok > > &subst_map) const;
    std::unordered_map< SymTok, std::vector< SymTok > > compose_subst(const std::unordered_map< SymTok, std::vector< SymTok > > &first,
                                                                      const std::unordered_map< SymTok, std::vector< SymTok > > &second) const;

    Prover build_type_prover(const std::vector< SymTok > &type_sent, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;

    std::vector< SymTok > read_sentence(const std::string &in) const;
    SentencePrinter print_sentence(const std::vector< SymTok > &sent, SentencePrinter::Style style=SentencePrinter::STYLE_PLAIN) const;
    SentencePrinter print_sentence(const ParsingTree< SymTok, LabTok > &pt, SentencePrinter::Style style=SentencePrinter::STYLE_PLAIN) const;
    SentencePrinter print_sentence(const ParsingTree2< SymTok, LabTok > &pt, SentencePrinter::Style style=SentencePrinter::STYLE_PLAIN) const;
    ProofPrinter print_proof(const std::vector< LabTok > &proof, bool only_assertions = false) const;
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > unify_assertion(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first=true, bool up_to_hyps_perms=true) const;

    void compute_everything();
    const std::vector< LabTok > &get_type_labels() const;
    const std::set< LabTok > &get_type_labels_set() const;
    void compute_type_correspondance();
    const std::vector< LabTok > &get_var_sym_to_lab();
    const std::vector< LabTok > &get_var_sym_to_lab() const;
    const std::vector< SymTok > &get_var_lab_to_sym();
    const std::vector< SymTok > &get_var_lab_to_sym() const;
    const std::vector< SymTok > &get_var_sym_to_type_sym();
    const std::vector< SymTok > &get_var_sym_to_type_sym() const;
    const std::vector< SymTok > &get_var_lab_to_type_sym();
    const std::vector< SymTok > &get_var_lab_to_type_sym() const;
    void compute_is_var_by_type();
    const std::vector< bool > &get_is_var_by_type();
    const std::vector< bool > &get_is_var_by_type() const;
    void compute_assertions_by_type();
    const std::unordered_map< SymTok, std::vector< LabTok > > &get_assertions_by_type();
    const std::unordered_map< SymTok, std::vector< LabTok > > &get_assertions_by_type() const;
    void compute_derivations();
    const std::unordered_map<SymTok, std::vector<std::pair< LabTok, std::vector<SymTok> > > > &get_derivations();
    const std::unordered_map<SymTok, std::vector<std::pair<LabTok, std::vector<SymTok> > > > &get_derivations() const;
    void compute_ders_by_label();
    const std::unordered_map< LabTok, std::pair< SymTok, std::vector< SymTok > > > &get_ders_by_label();
    const std::unordered_map< LabTok, std::pair< SymTok, std::vector< SymTok > > > &get_ders_by_label() const;
    std::vector< SymTok > reconstruct_sentence(const ParsingTree< SymTok, LabTok > &pt);
    std::vector< SymTok > reconstruct_sentence(const ParsingTree< SymTok, LabTok > &pt) const;
    void compute_vars();
    const std::vector< std::set< LabTok > > &get_sentence_vars();
    const std::vector< std::set< LabTok > > &get_sentence_vars() const;
    const std::vector< std::set< LabTok > > &get_assertion_unconst_vars();
    const std::vector< std::set< LabTok > > &get_assertion_unconst_vars() const;
    const std::vector< std::set< LabTok > > &get_assertion_const_vars();
    const std::vector< std::set< LabTok > > &get_assertion_const_vars() const;

    void compute_parser_initialization();
    const LRParser< SymTok, LabTok > &get_parser();
    const LRParser< SymTok, LabTok > &get_parser() const;
    ParsingTree< SymTok, LabTok > parse_sentence(typename Sentence::const_iterator sent_begin, typename Sentence::const_iterator sent_end, SymTok type) const;
    ParsingTree< SymTok, LabTok > parse_sentence(const Sentence &sent, SymTok type) const;
    ParsingTree< SymTok, LabTok > parse_sentence(const Sentence &sent) const;
    ParsingTree< SymTok, LabTok > parse_sentence(typename Sentence::const_iterator sent_begin, typename Sentence::const_iterator sent_end, SymTok type);
    ParsingTree< SymTok, LabTok > parse_sentence(const Sentence &sent, SymTok type);
    ParsingTree< SymTok, LabTok > parse_sentence(const Sentence &sent);

    void compute_sentences_parsing();
    const std::vector< ParsingTree< SymTok, LabTok > > &get_parsed_sents();
    const std::vector< ParsingTree< SymTok, LabTok > > &get_parsed_sents() const;
    const std::vector<ParsingTree2<SymTok, LabTok> > &get_parsed_sents2();
    const std::vector<ParsingTree2<SymTok, LabTok> > &get_parsed_sents2() const;
    const std::vector<std::vector<std::pair< ParsingTreeMultiIterator< SymTok, LabTok >::Status, ParsingTreeNode< SymTok, LabTok > > > > &get_parsed_iters();
    const std::vector<std::vector<std::pair< ParsingTreeMultiIterator< SymTok, LabTok >::Status, ParsingTreeNode< SymTok, LabTok > > > > &get_parsed_iters() const;

    static RegisteredProver register_prover(const std::vector< std::string > &templ_hyps, const std::string &templ_thesis);
    Prover build_registered_prover(const RegisteredProver &prover, const std::unordered_map< std::string, Prover > &types_provers, const std::vector< Prover > &hyps_provers) const;
    void compute_registered_provers();

    Prover build_prover(const std::vector< Sentence > &templ_hyps,
                        const Sentence &templ_thesis,
                        const std::unordered_map< std::string, Prover > &types_provers,
                        const std::vector< Prover > &hyps_provers) const;

    bool proving_helper(const RegisteredProverInstanceData &inst_data, const std::unordered_map< std::string, Prover > &types_provers, const std::vector< Prover > &hyps_provers, ProofEngine &engine) const;

    std::pair< LabTok, SymTok > new_temp_var(SymTok type_sym);
    void new_temp_var_frame();
    void release_temp_var_frame();
    std::pair< std::vector< ParsingTree< SymTok, LabTok > >, ParsingTree< SymTok, LabTok > > refresh_assertion(const Assertion &ass);
    std::pair< std::vector< ParsingTree2< SymTok, LabTok > >, ParsingTree2< SymTok, LabTok > > refresh_assertion2(const Assertion &ass);
    ParsingTree< SymTok, LabTok > refresh_parsing_tree(const ParsingTree< SymTok, LabTok > &pt);
    ParsingTree2<SymTok, LabTok> refresh_parsing_tree2(const ParsingTree2< SymTok, LabTok > &pt);
    SubstMap< SymTok, LabTok > build_refreshing_subst_map(const std::set< LabTok > &vars);
    SimpleSubstMap2< SymTok, LabTok > build_refreshing_subst_map2(const std::set< LabTok > &vars);
    SubstMap2< SymTok, LabTok > build_refreshing_full_subst_map2(const std::set< LabTok > &vars);

    const std::function<bool (LabTok)> &get_standard_is_var() const;
    const std::function<bool (SymTok)> &get_standard_is_var_sym() const;
    SymTok get_turnstile() const;
    SymTok get_turnstile_alias() const;

    // Library interface
    SymTok get_symbol(std::string s) const;
    LabTok get_label(std::string s) const;
    std::string resolve_symbol(SymTok tok) const;
    std::string resolve_label(LabTok tok) const;
    std::size_t get_symbols_num() const;
    std::size_t get_labels_num() const;
    bool is_constant(SymTok c) const;
    const Sentence &get_sentence(LabTok label) const;
    SentenceType get_sentence_type(LabTok label) const;
    const Assertion &get_assertion(LabTok label) const;
    std::function< const Assertion*() > list_assertions() const;
    const StackFrame &get_final_stack_frame() const;
    const LibraryAddendum &get_addendum() const;
    const ParsingAddendumImpl &get_parsing_addendum() const;

private:
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > unify_assertion_uncached(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first=true, bool up_to_hyps_perms=true) const;
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > unify_assertion_uncached2(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first=true, bool up_to_hyps_perms=true) const;
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > unify_assertion_cached(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first=true);
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > unify_assertion_cached(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first=true) const;

    bool type_proving_helper(const std::vector< SymTok > &type_sent, ProofEngine &engine, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;

    const ExtendedLibrary &lib;
    SymTok turnstile;
    SymTok turnstile_alias;

    std::vector< LabTok > type_labels;
    std::set< LabTok > type_labels_set;

    std::vector< LabTok > var_sym_to_lab;
    std::vector< SymTok > var_lab_to_sym;
    std::vector< SymTok > var_sym_to_type_sym;
    std::vector< SymTok > var_lab_to_type_sym;
    bool type_corrsepondance_computed = false;

    std::vector< bool > is_var_by_type;
    bool is_var_by_type_computed = false;

    std::unordered_map< SymTok, std::vector< LabTok > > assertions_by_type;
    bool assertions_by_type_computed = false;

    std::unordered_map<SymTok, std::vector<std::pair< LabTok, std::vector<SymTok> > > > derivations;
    bool derivations_computed = false;

    std::unordered_map< LabTok, std::pair< SymTok, std::vector< SymTok > > > ders_by_label;
    bool ders_by_label_computed = false;

    std::unordered_map< std::tuple< std::vector< std::vector< SymTok > >, std::vector< SymTok > >,
                        std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > >,
                        boost::hash< std::tuple< std::vector< std::vector< SymTok > >, std::vector< SymTok > > > > unification_cache;

    LRParser< SymTok, LabTok > *parser = NULL;
    bool parser_initialization_computed = false;

    std::vector< ParsingTree< SymTok, LabTok > > parsed_sents;
    std::vector< ParsingTree2< SymTok, LabTok > > parsed_sents2;
    std::vector< std::vector< std::pair< ParsingTreeMultiIterator< SymTok, LabTok >::Status, ParsingTreeNode< SymTok, LabTok > > > > parsed_iters;
    bool sentences_parsing_computed = false;

    bool vars_computed = false;
    std::vector< std::set< LabTok > > sentence_vars;
    std::vector< std::set< LabTok > > assertion_unconst_vars;
    std::vector< std::set< LabTok > > assertion_const_vars;

    // This is an instance of the Construct On First Use idiom, which prevents the static initialization fiasco;
    // see https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use-members
    static std::vector< RegisteredProverData > &registered_provers() {
        static std::vector< RegisteredProverData > *ret = new std::vector< RegisteredProverData >();
        return *ret;
    }

    std::vector< RegisteredProverInstanceData > instance_registered_provers;
    void compute_registered_prover(size_t i, bool exception_on_failure = true);

    std::shared_ptr< ToolboxCache > cache;

    void create_temp_var(SymTok type_sym);
    std::map< SymTok, size_t > temp_idx;
    std::unordered_map< LabTok, Sentence > temp_types;
    StringCache< SymTok > temp_syms;
    StringCache< LabTok > temp_labs;
    std::map< SymTok, std::vector< std::pair< LabTok, SymTok > > > free_temp_vars;
    std::map< SymTok, std::vector< std::pair< LabTok, SymTok > > > used_temp_vars;
    std::vector< std::map< SymTok, size_t > > temp_vars_stack;
    std::function< bool(LabTok) > standard_is_var;
    std::function< bool(SymTok) > standard_is_var_sym;
};