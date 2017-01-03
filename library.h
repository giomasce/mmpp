#ifndef LIBRARY_H
#define LIBRARY_H

#include <unordered_map>
#include <vector>
#include <utility>
#include <tuple>
#include <limits>
#include <set>
#include <type_traits>
#include <memory>

#include <boost/functional/hash.hpp>

#include <cassert>

class Library;
class LibraryImpl;
class Assertion;

typedef uint16_t SymTok;
typedef uint32_t LabTok;

static_assert(std::is_integral< SymTok >::value);
static_assert(std::is_unsigned< SymTok >::value);
static_assert(std::is_integral< LabTok >::value);
static_assert(std::is_unsigned< LabTok >::value);

typedef std::vector< SymTok > Sentence;
typedef std::vector< LabTok > Procedure;

#include "proof.h"
#include "stringcache.h"

struct StackFrame {
    std::set< SymTok > vars;
    std::set< std::pair< SymTok, SymTok > > dists;     // Must be stored ordered, for example using minmax()
    std::vector< LabTok > types;                       // Sentence is of type (constant, variable)
    std::set< LabTok > types_set;                      // For faster searching
    std::vector< LabTok > hyps;
};

struct LibraryAddendum {
    std::vector< std::string > htmldefs;
    std::vector< std::string > althtmldefs;
    std::vector< std::string > latexdefs;
    std::string htmlcss;
    std::string htmlfont;
    std::string htmltitle;
    std::string htmlhome;
    std::string htmlbibliography;
    std::string exthtmltitle;
    std::string exthtmlhome;
    std::string exthtmllabel;
    std::string exthtmlbibliography;
    std::string htmlvarcolor;
    std::string htmldir;
    std::string althtmldir;
};

std::string fix_htmlcss_for_qt(std::string s);

class Assertion {
public:
    Assertion();
    Assertion(bool theorem,
              std::set< std::pair< SymTok, SymTok > > mand_dists,
              std::set< std::pair< SymTok, SymTok > > opt_dists,
              std::vector< LabTok > float_hyps,
              std::vector< LabTok > ess_hyps,
              std::set< LabTok > opt_hyps,
              LabTok thesis,
              std::string comment = "");
    ~Assertion();
    bool is_valid() const;
    bool is_theorem() const;
    bool is_modif_disc() const;
    bool is_usage_disc() const;
    const std::set< std::pair< SymTok, SymTok > > &get_mand_dists() const;
    const std::set< std::pair< SymTok, SymTok > > &get_opt_dists() const;
    const std::set<std::pair<SymTok, SymTok> > get_dists() const;
    size_t get_mand_hyps_num() const;
    LabTok get_mand_hyp(size_t i) const;
    const std::vector<LabTok> &get_float_hyps() const;
    const std::vector<LabTok> &get_ess_hyps() const;
    const std::set< LabTok > &get_opt_hyps() const;
    LabTok get_thesis() const;
    std::shared_ptr< ProofExecutor > get_proof_executor(const Library &lib) const;
    void set_proof(std::shared_ptr<Proof> proof);

private:
    bool valid;
    bool theorem;
    std::set< std::pair< SymTok, SymTok > > mand_dists;
    std::set< std::pair< SymTok, SymTok > > opt_dists;
    std::vector< LabTok > float_hyps;
    std::vector< LabTok > ess_hyps;
    std::set< LabTok > opt_hyps;
    LabTok thesis;
    std::shared_ptr< Proof > proof;
    std::string comment;
    bool modif_disc;
    bool usage_disc;

    std::shared_ptr< Proof > get_proof() const;
};

class Library {
public:
    virtual SymTok get_symbol(std::string s) const = 0;
    virtual LabTok get_label(std::string s) const = 0;
    virtual std::string resolve_symbol(SymTok tok) const = 0;
    virtual std::string resolve_label(LabTok tok) const = 0;
    virtual std::size_t get_symbols_num() const = 0;
    virtual std::size_t get_labels_num() const = 0;
    virtual bool is_constant(SymTok c) const = 0;

    virtual const std::vector<SymTok> &get_sentence(LabTok label) const = 0;
    virtual const Assertion &get_assertion(LabTok label) const = 0;
    virtual const std::vector< Assertion > &get_assertions() const = 0;

    virtual const StackFrame &get_final_stack_frame() const = 0;
    virtual const LibraryAddendum &get_addendum() const = 0;

    virtual ~Library();
};

class LibraryImpl : public Library
{
public:
    LibraryImpl();
    SymTok get_symbol(std::string s) const;
    LabTok get_label(std::string s) const;
    std::string resolve_symbol(SymTok tok) const;
    std::string resolve_label(LabTok tok) const;
    std::size_t get_symbols_num() const;
    std::size_t get_labels_num() const;
    const std::vector<SymTok> &get_sentence(LabTok label) const;
    const Assertion &get_assertion(LabTok label) const;
    const std::vector< Assertion > &get_assertions() const;
    bool is_constant(SymTok c) const;
    const StackFrame &get_final_stack_frame() const;
    const LibraryAddendum &get_addendum() const;

    SymTok create_symbol(std::string s);
    LabTok create_label(std::string s);
    void add_sentence(LabTok label, std::vector< SymTok > content);
    void add_assertion(LabTok label, const Assertion &ass);
    void add_constant(SymTok c);
    void set_final_stack_frame(const StackFrame &final_stack_frame);
    void set_addendum(const LibraryAddendum &add);

private:
    StringCache< SymTok > syms;
    StringCache< LabTok > labels;
    std::set< SymTok > consts;

    // vector is more efficient than unordered_map if labels are known to be contiguous and starting from 1; in the general case the unordered_map might be better
    //std::unordered_map< LabTok, std::vector< SymTok > > sentences;
    std::vector< std::vector< SymTok > > sentences;
    std::vector< Assertion > assertions;

    StackFrame final_stack_frame;
    LibraryAddendum addendum;

    //std::vector< LabTok > prove_type2(const std::vector< SymTok > &type_sent) const;
};

#endif // LIBRARY_H
