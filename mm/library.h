#pragma once

#include <unordered_map>
#include <map>
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

#include "funds.h"
#include "mmtypes.h"

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
    virtual const std::string &get_htmldef(SymTok tok) const override
    {
        return this->htmldefs.at(tok.val());
    }
    virtual const std::string &get_althtmldef(SymTok tok) const override
    {
        return this->althtmldefs.at(tok.val());
    }
    virtual const std::string &get_latexdef(SymTok tok) const override
    {
        return this->latexdefs.at(tok.val());
    }
    const std::vector< std::string > &get_htmldefs() const override
    {
        return this->htmldefs;
    }
    const std::vector< std::string > &get_althtmldefs() const override
    {
        return this->althtmldefs;
    }
    const std::vector< std::string > &get_latexdefs() const override
    {
        return this->latexdefs;
    }
    const std::string &get_htmlcss() const override
    {
        return this->htmlcss;
    }
    const std::string &get_htmlfont() const override
    {
        return this->htmlfont;
    }
    const std::string &get_htmltitle() const override
    {
        return this->htmltitle;
    }
    const std::string &get_htmlhome() const override
    {
        return this->htmlhome;
    }
    const std::string &get_htmlbibliography() const override
    {
        return this->htmlbibliography;
    }
    const std::string &get_exthtmltitle() const override
    {
        return this->exthtmltitle;
    }
    const std::string &get_exthtmlhome() const override
    {
        return this->exthtmlhome;
    }
    const std::string &get_exthtmllabel() const override
    {
        return this->exthtmllabel;
    }
    const std::string &get_exthtmlbibliography() const override
    {
        return this->exthtmlbibliography;
    }
    const std::string &get_htmlvarcolor() const override
    {
        return this->htmlvarcolor;
    }
    const std::string &get_htmldir() const override
    {
        return this->htmldir;
    }
    const std::string &get_althtmldir() const override
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
    const std::map< SymTok, SymTok > &get_syntax() const override {
        return this->syntax;
    }
    const std::string &get_unambiguous() const override {
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
              bool _has_proof,
              const std::set< std::pair< SymTok, SymTok > > &mand_dists,
              const std::set< std::pair< SymTok, SymTok > > &opt_dists,
              const std::vector< LabTok > &float_hyps,
              const std::vector< LabTok > &ess_hyps,
              const std::set< LabTok > &opt_hyps,
              LabTok thesis,
              LabTok number,
              const std::string &comment = "");
    Assertion(const std::vector< LabTok > &float_hyps, const std::vector< LabTok > &ess_hyps);
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
    bool has_proof() const
    {
        return this->_has_proof;
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
    size_t get_mand_hyps_num() const
    {
        return this->get_float_hyps().size() + this->get_ess_hyps().size();
    }
    LabTok get_mand_hyp(size_t i) const;
    const std::vector<LabTok> &get_float_hyps() const
    {
        return this->float_hyps;
    }
    const std::vector<LabTok> &get_ess_hyps() const
    {
        return this->ess_hyps;
    }
    const std::set< LabTok > &get_opt_hyps() const
    {
        return this->opt_hyps;
    }
    LabTok get_thesis() const {
        return this->thesis;
    }
    template< typename SentType_ >
    std::shared_ptr< ProofExecutor< SentType_ > > get_proof_executor(const Library &lib, bool gen_proof_tree=false) const;
    std::shared_ptr< ProofOperator > get_proof_operator(const Library &lib) const;
    void set_proof(std::shared_ptr<const Proof> proof)
    {
        assert(this->theorem);
        this->proof = proof;
    }
    std::shared_ptr< const Proof > get_proof() const {
        return this->proof;
    }

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
    std::shared_ptr< const Proof > proof;
    std::string comment;
    bool modif_disc;
    bool usage_disc;
    bool _has_proof;
};

enum SentenceType {
    UNKNOWN = 0,
    ESSENTIAL_HYP,
    FLOATING_HYP,
    PROPOSITION,
    AXIOM,
};

class Library {
public:
    virtual SymTok get_symbol(std::string s) const = 0;
    virtual LabTok get_label(std::string s) const = 0;
    virtual std::string resolve_symbol(SymTok tok) const = 0;
    virtual std::string resolve_label(LabTok tok) const = 0;
    virtual size_t get_symbols_num() const = 0;
    virtual size_t get_labels_num() const = 0;
    virtual bool is_constant(SymTok c) const = 0;
    virtual const Sentence &get_sentence(LabTok label) const = 0;
    virtual SentenceType get_sentence_type(LabTok label) const = 0;
    virtual const Assertion &get_assertion(LabTok label) const = 0;
    //virtual std::function< const Assertion*() > list_assertions() const = 0;
    virtual Generator< std::reference_wrapper< const Assertion > > gen_assertions() const = 0;
    virtual const StackFrame &get_final_stack_frame() const = 0;
    virtual const LibraryAddendum &get_addendum() const = 0;
    virtual const ParsingAddendumImpl &get_parsing_addendum() const = 0;
    virtual bool is_immutable() const;
    virtual ~Library();
};

class ExtendedLibrary : public Library {
public:
    virtual const Sentence *get_sentence_ptr(LabTok label) const = 0;
    virtual const Assertion *get_assertion_ptr(LabTok label) const = 0;
    virtual const std::unordered_map< SymTok, std::string > &get_symbols() const = 0;
    virtual const std::unordered_map<LabTok, std::string> &get_labels() const = 0;
    virtual const std::vector< Sentence > &get_sentences() const = 0;
    virtual const std::vector< SentenceType > &get_sentence_types() const = 0;
    virtual const std::vector< Assertion > &get_assertions() const = 0;
    const ExtendedLibraryAddendum &get_addendum() const = 0;
    virtual LabTok get_max_number() const = 0;
};

class LibraryImpl final : public ExtendedLibrary
{
public:
    LibraryImpl();
    SymTok get_symbol(std::string s) const override;
    LabTok get_label(std::string s) const override;
    std::string resolve_symbol(SymTok tok) const override;
    std::string resolve_label(LabTok tok) const override;
    size_t get_symbols_num() const override;
    size_t get_labels_num() const override;
    const std::unordered_map< SymTok, std::string > &get_symbols() const override;
    const std::unordered_map<LabTok, std::string> &get_labels() const override;
    const Sentence &get_sentence(LabTok label) const override;
    const Sentence *get_sentence_ptr(LabTok label) const override;
    SentenceType get_sentence_type(LabTok label) const override;
    const Assertion &get_assertion(LabTok label) const override;
    const Assertion *get_assertion_ptr(LabTok label) const override;
    const std::vector< Sentence > &get_sentences() const override;
    const std::vector< SentenceType > &get_sentence_types() const override;
    const std::vector< Assertion > &get_assertions() const override;
    bool is_constant(SymTok c) const override;
    const StackFrame &get_final_stack_frame() const override;
    const LibraryAddendumImpl &get_addendum() const override;
    const ParsingAddendumImpl &get_parsing_addendum() const override;
    std::function< const Assertion*() > list_assertions() const;
    Generator< std::reference_wrapper< const Assertion > > gen_assertions() const override;
    virtual LabTok get_max_number() const override;
    virtual bool is_immutable() const override;

    SymTok create_symbol(std::string s);
    SymTok create_or_get_symbol(std::string s);
    LabTok create_label(std::string s);
    void add_sentence(LabTok label, const Sentence &content, SentenceType type);
    void add_assertion(LabTok label, const Assertion &ass);
    void set_constant(SymTok c, bool is_const);
    void set_final_stack_frame(const StackFrame &final_stack_frame);
    void set_max_number(LabTok max_number);
    void set_addendum(const LibraryAddendumImpl &add);
    void set_parsing_addendum(const ParsingAddendumImpl &add);

private:
    StringCache< SymTok > syms;
    StringCache< LabTok > labels;
    std::vector< bool > consts;
    //std::set< SymTok > consts;
    //std::set< SymTok > vars;

    // vector is more efficient than unordered_map if labels are known to be contiguous and starting from 1; in the general case the unordered_map might be better
    //std::unordered_map< LabTok, std::vector< SymTok > > sentences;
    std::vector< Sentence > sentences;
    std::vector< SentenceType > sentence_types;
    std::vector< Assertion > assertions;

    StackFrame final_stack_frame;
    LibraryAddendumImpl addendum;
    ParsingAddendumImpl parsing_addendum;

    LabTok max_number;
};
