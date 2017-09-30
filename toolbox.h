#ifndef LIBRARYTOOLBOX_H
#define LIBRARYTOOLBOX_H

#include <vector>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "library.h"
#include "lr.h"

class LibraryToolbox;

struct SentenceTree {
    LabTok label;
    std::vector< SentenceTree > children;
};

typedef std::function< bool(ProofEngine&) > Prover;
const Prover null_prover = [](ProofEngine&){ return false; };
//Prover cascade_provers(const Prover &a, const Prover &b);

std::string test_prover(Prover prover, const LibraryToolbox &tb);

struct SentencePrinter {
    enum Style {
        STYLE_PLAIN,
        STYLE_HTML,
        STYLE_ALTHTML,
        STYLE_LATEX,
    };
    const std::vector< SymTok > &sent;
    const Library &lib;
    const Style style;

    std::string to_string() const;
};
struct ProofPrinter {
    const std::vector< LabTok > &proof;
    const Library &lib;

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

class LibraryToolbox
{
public:
    explicit LibraryToolbox(const Library &lib, bool compute=false, std::shared_ptr< ToolboxCache > cache = NULL);
    ~LibraryToolbox();
    const Library &get_library() const;
    void set_cache(std::shared_ptr< ToolboxCache > cache);
    std::vector< SymTok > substitute(const std::vector< SymTok > &orig, const std::unordered_map< SymTok, std::vector< SymTok > > &subst_map) const;
    std::unordered_map< SymTok, std::vector< SymTok > > compose_subst(const std::unordered_map< SymTok, std::vector< SymTok > > &first,
                                                                      const std::unordered_map< SymTok, std::vector< SymTok > > &second) const;

    Prover build_type_prover_from_strings(const std::string &type_sent, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;
    Prover build_classical_type_prover(const std::vector< SymTok > &type_sent, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;
    Prover build_earley_type_prover(const std::vector< SymTok > &type_sent, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;
    Prover build_type_prover(const std::vector< SymTok > &type_sent, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;
    bool classical_type_proving_helper(const std::vector< SymTok > &type_sent, ProofEngine &engine, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;
    bool earley_type_proving_helper(const std::vector< SymTok > &type_sent, ProofEngine &engine, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;

    std::vector< SymTok > read_sentence(const std::string &in) const;
    SentencePrinter print_sentence(const std::vector< SymTok > &sent, SentencePrinter::Style style=SentencePrinter::STYLE_PLAIN) const;
    ProofPrinter print_proof(const std::vector< LabTok > &proof) const;
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > unify_assertion(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first=true, bool up_to_hyps_perms=true) const;

    void compute_everything();
    const std::vector< LabTok > &get_types() const;
    void compute_types_by_var();
    const std::vector< LabTok > &get_types_by_var();
    const std::vector< LabTok > &get_types_by_var() const;
    void compute_assertions_by_type();
    const std::unordered_map< SymTok, std::vector< LabTok > > &get_assertions_by_type();
    const std::unordered_map< SymTok, std::vector< LabTok > > &get_assertions_by_type() const;
    void compute_derivations();
    const std::unordered_map<SymTok, std::vector<std::pair< LabTok, std::vector<SymTok> > > > &get_derivations();
    const std::unordered_map<SymTok, std::vector<std::pair<LabTok, std::vector<SymTok> > > > &get_derivations() const;

    static RegisteredProver register_prover(const std::vector< std::string > &templ_hyps, const std::string &templ_thesis);
    Prover build_registered_prover(const RegisteredProver &prover, const std::unordered_map< std::string, Prover > &types_provers, const std::vector< Prover > &hyps_provers) const;
    void compute_registered_provers();

    void compute_parser_initialization();
    const LRParser< SymTok, LabTok > &get_parser();
    const LRParser< SymTok, LabTok > &get_parser() const;
    ParsingTree< LabTok > parse_sentence(const std::vector<SymTok> &sent, SymTok type) const;

    bool proving_helper(const std::vector< Sentence > &templ_hyps,
                         const Sentence &templ_thesis,
                         const std::unordered_map< std::string, Prover > &types_provers,
                         const std::vector< Prover > &hyps_provers,
                         ProofEngine &engine) const;
    Prover build_prover(const std::vector< Sentence > &templ_hyps,
                         const Sentence &templ_thesis,
                         const std::unordered_map< std::string, Prover > &types_provers,
                         const std::vector< Prover > &hyps_provers) const;

private:
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > unify_assertion_uncached(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first=true, bool up_to_hyps_perms=true) const;
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > unify_assertion_cached(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first=true);
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > unify_assertion_cached(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first=true) const;

    const Library &lib;

    std::vector< LabTok > types_by_var;
    bool types_by_var_computed = false;

    std::unordered_map< SymTok, std::vector< LabTok > > assertions_by_type;
    bool assertions_by_type_computed = false;

    std::unordered_map<SymTok, std::vector<std::pair< LabTok, std::vector<SymTok> > > > derivations;
    bool derivations_computed = false;

    std::unordered_map< std::tuple< std::vector< std::vector< SymTok > >, std::vector< SymTok > >,
                        std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > >,
                        boost::hash< std::tuple< std::vector< std::vector< SymTok > >, std::vector< SymTok > > > > unification_cache;

    LRParser< SymTok, LabTok > *parser;
    bool parser_initialization_computed = false;

    // This is an instance of the Construct On First Use idiom, which prevents the static initialization fiasco; see https://isocpp.org/wiki/faq/ctors#static-init-order-on-first-use-members
    static std::vector< RegisteredProverData > &registered_provers() {
        static std::vector< RegisteredProverData > *ret = new std::vector< RegisteredProverData >();
        return *ret;
    }

    std::vector< RegisteredProverInstanceData > instance_registered_provers;
    void compute_registered_prover(size_t i);

    std::shared_ptr< ToolboxCache > cache;
};

#endif // LIBRARYTOOLBOX_H
