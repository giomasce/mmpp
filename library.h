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
#include <functional>

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
#include "utils/stringcache.h"

struct StackFrame {
    std::set< SymTok > vars;
    std::set< std::pair< SymTok, SymTok > > dists;     // Must be stored ordered, for example using minmax()
    std::vector< LabTok > types;                       // Sentence is of type (constant, variable)
    std::set< LabTok > types_set;                      // For faster searching
    std::vector< LabTok > hyps;
};

class LibraryAddendum {
public:
    virtual const std::string &get_htmldef(SymTok tok) const = 0;
    virtual const std::string &get_althtmldef(SymTok tok) const = 0;
    virtual const std::string &get_latexdef(SymTok tok) const = 0;
    virtual const std::string &get_htmlcss() const = 0;
    virtual const std::string &get_htmlfont() const = 0;
    virtual const std::string &get_htmltitle() const = 0;
    virtual const std::string &get_htmlhome() const = 0;
    virtual const std::string &get_htmlbibliography() const = 0;
    virtual const std::string &get_exthtmltitle() const = 0;
    virtual const std::string &get_exthtmlhome() const = 0;
    virtual const std::string &get_exthtmllabel() const = 0;
    virtual const std::string &get_exthtmlbibliography() const = 0;
    virtual const std::string &get_htmlvarcolor() const = 0;
    virtual const std::string &get_htmldir() const = 0;
    virtual const std::string &get_althtmldir() const = 0;
};

class ExtendedLibraryAddendum : public LibraryAddendum {
public:
    virtual const std::vector< std::string > &get_htmldefs() const = 0;
    virtual const std::vector< std::string > &get_althtmldefs() const= 0;
    virtual const std::vector< std::string > &get_latexdefs() const = 0;
};

class LibraryAddendumImpl : public ExtendedLibraryAddendum {
    friend class Reader;
public:
    virtual const std::string &get_htmldef(SymTok tok) const
    {
        return this->htmldefs.at(tok);
    }
    virtual const std::string &get_althtmldef(SymTok tok) const
    {
        return this->althtmldefs.at(tok);
    }
    virtual const std::string &get_latexdef(SymTok tok) const
    {
        return this->latexdefs.at(tok);
    }
    const std::vector< std::string > &get_htmldefs() const
    {
        return this->htmldefs;
    }
    const std::vector< std::string > &get_althtmldefs() const
    {
        return this->althtmldefs;
    }
    const std::vector< std::string > &get_latexdefs() const
    {
        return this->latexdefs;
    }
    const std::string &get_htmlcss() const
    {
        return this->htmlcss;
    }
    const std::string &get_htmlfont() const
    {
        return this->htmlfont;
    }
    const std::string &get_htmltitle() const
    {
        return this->htmltitle;
    }
    const std::string &get_htmlhome() const
    {
        return this->htmlhome;
    }
    const std::string &get_htmlbibliography() const
    {
        return this->htmlbibliography;
    }
    const std::string &get_exthtmltitle() const
    {
        return this->exthtmltitle;
    }
    const std::string &get_exthtmlhome() const
    {
        return this->exthtmlhome;
    }
    const std::string &get_exthtmllabel() const
    {
        return this->exthtmllabel;
    }
    const std::string &get_exthtmlbibliography() const
    {
        return this->exthtmlbibliography;
    }
    const std::string &get_htmlvarcolor() const
    {
        return this->htmlvarcolor;
    }
    const std::string &get_htmldir() const
    {
        return this->htmldir;
    }
    const std::string &get_althtmldir() const
    {
        return this->althtmldir;
    }
private:
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

class ParsingAddendum {
    virtual const std::map< SymTok, SymTok > &get_syntax() const = 0;
    virtual const std::string &get_unambiguous() const = 0;
};

class ParsingAddendumImpl : public ParsingAddendum {
    friend class Reader;
public:
    const std::map< SymTok, SymTok > &get_syntax() const {
        return this->syntax;
    }
    const std::string &get_unambiguous() const {
        return this->unambiguous;
    }
private:
    std::map< SymTok, SymTok > syntax;
    std::string unambiguous;
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
              LabTok number,
              std::string comment = "");
    ~Assertion();
    bool is_valid() const
    {
        return this->valid;
    }
    bool is_theorem() const {
        return this->theorem;
    }
    bool is_modif_disc() const
    {
        return this->modif_disc;
    }
    bool is_usage_disc() const
    {
        return this->usage_disc;
    }
    const std::set< std::pair< SymTok, SymTok > > &get_mand_dists() const {
        return this->mand_dists;
    }
    const std::set< std::pair< SymTok, SymTok > > &get_opt_dists() const
    {
        return this->opt_dists;
    }
    LabTok get_number() const {
        return this->number;
    }
    const std::string &get_comment() const {
        return this->comment;
    }
    const std::set<std::pair<SymTok, SymTok> > get_dists() const;
    size_t get_mand_hyps_num() const;
    LabTok get_mand_hyp(size_t i) const;
    const std::vector<LabTok> &get_float_hyps() const;
    const std::vector<LabTok> &get_ess_hyps() const;
    const std::set< LabTok > &get_opt_hyps() const;
    LabTok get_thesis() const;
    std::shared_ptr< ProofExecutor > get_proof_executor(const Library &lib, bool gen_proof_tree=false) const;
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
    LabTok number;
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
    virtual const Sentence &get_sentence(LabTok label) const = 0;
    virtual const Assertion &get_assertion(LabTok label) const = 0;
    virtual std::function< const Assertion*() > list_assertions() const = 0;
    virtual const StackFrame &get_final_stack_frame() const = 0;
    virtual const LibraryAddendum &get_addendum() const = 0;
    virtual const ParsingAddendumImpl &get_parsing_addendum() const = 0;
    virtual bool is_immutable() const;
    virtual ~Library();
};

class ExtendedLibrary : public Library {
public:
    virtual const std::unordered_map< SymTok, std::string > &get_symbols() const = 0;
    virtual const std::unordered_map<LabTok, std::string> &get_labels() const = 0;
    virtual const std::vector< Assertion > &get_assertions() const = 0;
    const ExtendedLibraryAddendum &get_addendum() const = 0;
    virtual LabTok get_max_number() const = 0;
};

class LibraryImpl : public ExtendedLibrary
{
public:
    LibraryImpl();
    SymTok get_symbol(std::string s) const;
    LabTok get_label(std::string s) const;
    std::string resolve_symbol(SymTok tok) const;
    std::string resolve_label(LabTok tok) const;
    std::size_t get_symbols_num() const;
    std::size_t get_labels_num() const;
    const std::unordered_map< SymTok, std::string > &get_symbols() const;
    const std::unordered_map<LabTok, std::string> &get_labels() const;
    const std::vector<SymTok> &get_sentence(LabTok label) const;
    const Assertion &get_assertion(LabTok label) const;
    const std::vector< Assertion > &get_assertions() const;
    bool is_constant(SymTok c) const;
    const StackFrame &get_final_stack_frame() const;
    const LibraryAddendumImpl &get_addendum() const;
    const ParsingAddendumImpl &get_parsing_addendum() const;
    std::function< const Assertion*() > list_assertions() const;
    virtual LabTok get_max_number() const;
    virtual bool is_immutable() const;

    SymTok create_symbol(std::string s);
    LabTok create_label(std::string s);
    void add_sentence(LabTok label, std::vector< SymTok > content);
    void add_assertion(LabTok label, const Assertion &ass);
    void add_constant(SymTok c);
    void set_final_stack_frame(const StackFrame &final_stack_frame);
    void set_max_number(LabTok max_number);
    void set_addendum(const LibraryAddendumImpl &add);
    void set_parsing_addendum(const ParsingAddendumImpl &add);

private:
    StringCache< SymTok > syms;
    StringCache< LabTok > labels;
    std::set< SymTok > consts;

    // vector is more efficient than unordered_map if labels are known to be contiguous and starting from 1; in the general case the unordered_map might be better
    //std::unordered_map< LabTok, std::vector< SymTok > > sentences;
    std::vector< std::vector< SymTok > > sentences;
    std::vector< Assertion > assertions;

    StackFrame final_stack_frame;
    LibraryAddendumImpl addendum;
    ParsingAddendumImpl parsing_addendum;

    LabTok max_number;
};

#endif // LIBRARY_H
