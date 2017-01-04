#ifndef LIBRARYTOOLBOX_H
#define LIBRARYTOOLBOX_H

#include <vector>
#include <unordered_map>
#include <functional>

#include "library.h"

class LibraryToolbox;

typedef std::function< bool(ProofEngine&) > Prover;
const Prover null_prover = [](ProofEngine&){ return false; };
Prover cascade_provers(const Prover &a, const Prover &b);

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
};
struct ProofPrinter {
    const std::vector< LabTok > &proof;
    const Library &lib;
};
std::ostream &operator<<(std::ostream &os, const SentencePrinter &sp);
std::ostream &operator<<(std::ostream &os, const ProofPrinter &sp);

class LibraryToolbox
{
public:
    explicit LibraryToolbox(const Library &lib, bool compute=false);
    const Library &get_library() const;
    std::vector< SymTok > substitute(const std::vector< SymTok > &orig, const std::unordered_map< SymTok, std::vector< SymTok > > &subst_map) const;
    std::unordered_map< SymTok, std::vector< SymTok > > compose_subst(const std::unordered_map< SymTok, std::vector< SymTok > > &first,
                                                                      const std::unordered_map< SymTok, std::vector< SymTok > > &second) const;

    Prover build_prover4(const std::vector< std::string > &templ_hyps,
                         const std::string &templ_thesis,
                         const std::unordered_map< std::string, Prover > &types_provers,
                         const std::vector< Prover > &hyps_provers) const;
    Prover build_type_prover2(const std::string &type_sent, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;
    Prover build_classical_type_prover(const std::vector< SymTok > &type_sent, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;
    Prover build_earley_type_prover(const std::vector< SymTok > &type_sent, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;
    Prover build_type_prover(const std::vector< SymTok > &type_sent, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;
    bool classical_type_proving_helper(const std::vector< SymTok > &type_sent, ProofEngine &engine, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;
    bool earley_type_proving_helper(const std::vector< SymTok > &type_sent, ProofEngine &engine, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;

    std::vector< SymTok > parse_sentence(const std::string &in) const;
    SentencePrinter print_sentence(const std::vector< SymTok > &sent, SentencePrinter::Style style=SentencePrinter::STYLE_PLAIN) const;
    ProofPrinter print_proof(const std::vector< LabTok > &proof) const;
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > unify_assertion(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first=true) const;

    void compute_everything();
    const std::vector< LabTok > &get_types() const;
    void compute_types_by_var();
    const std::vector< LabTok > &get_types_by_var();
    const std::vector< LabTok > &get_types_by_var() const;
    void compute_assertions_by_type();
    const std::unordered_map< SymTok, std::vector< LabTok > > &get_assertions_by_type();
    const std::unordered_map< SymTok, std::vector< LabTok > > &get_assertions_by_type() const;

private:
    bool proving_helper3(const std::vector< std::vector< SymTok > > &templ_hyps,
                         const std::vector< SymTok > &templ_thesis,
                         const std::unordered_map< SymTok, Prover > &types_provers,
                         const std::vector< Prover > &hyps_provers,
                         ProofEngine &engine) const;
    bool proving_helper4(const std::vector< std::string > &templ_hyps,
                         const std::string &templ_thesis,
                         const std::unordered_map< std::string, Prover > &types_provers,
                         const std::vector< Prover > &hyps_provers,
                         ProofEngine &engine) const;
private:
    const Library &lib;

    std::vector< LabTok > types_by_var;
    bool types_by_var_computed = false;

    std::unordered_map< SymTok, std::vector< LabTok > > assertions_by_type;
    bool assertions_by_type_computed = false;
};

#endif // LIBRARYTOOLBOX_H
