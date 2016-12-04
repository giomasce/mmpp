#ifndef LIBRARYTOOLBOX_H
#define LIBRARYTOOLBOX_H

#include <vector>
#include <unordered_map>
#include <functional>

#include "library.h"

typedef std::function< bool(const LibraryInterface&, ProofEngine&) > Prover;
const Prover null_prover = [](const LibraryInterface&, ProofEngine&){ return false; };

class LibraryToolbox
{
public:
    LibraryToolbox(const LibraryInterface &lib);
    std::vector< SymTok > substitute(const std::vector< SymTok > &orig, const std::unordered_map< SymTok, std::vector< SymTok > > &subst_map) const;
    std::unordered_map< SymTok, std::vector< SymTok > > compose_subst(const std::unordered_map< SymTok, std::vector< SymTok > > &first,
                                                                      const std::unordered_map< SymTok, std::vector< SymTok > > &second) const;

    static std::function< bool(const LibraryInterface&, ProofEngine&) > build_prover4(const std::vector< std::string > &templ_hyps,
                         const std::string &templ_thesis,
                         const std::unordered_map< std::string, Prover > &types_provers,
                         const std::vector< Prover > &hyps_provers);
    static std::function< bool(const LibraryInterface&, ProofEngine&) > build_type_prover2(const std::string &type_sent, const std::unordered_map< SymTok, Prover > &var_provers = {});
    static std::function< bool(const LibraryInterface&, ProofEngine&) > build_type_prover(const std::vector< SymTok > &type_sent, const std::unordered_map< SymTok, Prover > &var_provers = {});
    bool type_proving_helper(const std::vector< SymTok > &type_sent, ProofEngine &engine, const std::unordered_map< SymTok, Prover > &var_provers = {}) const;

    static Prover compose_provers(const Prover &a, const Prover &b);

private:
    static std::function< bool(const LibraryInterface&, ProofEngine&) > build_prover2(const std::vector< std::vector< SymTok > > &templ_hyps,
                             const std::vector< SymTok > &templ_thesis,
                             const std::vector< std::function< bool(const LibraryInterface&, ProofEngine&) > > &hyps_provers,
                             const std::unordered_map< SymTok, std::vector< SymTok > > &subst_map);
    static std::function< bool(const LibraryInterface&, ProofEngine&) > build_prover3(const std::vector< std::vector< SymTok > > &templ_hyps,
                         const std::vector< SymTok > &templ_thesis,
                         const std::unordered_map< SymTok, Prover > &types_provers,
                         const std::vector< Prover > &hyps_provers);
private:
    std::vector< LabTok > proving_helper(const std::vector< std::vector< SymTok > > &templ_hyps, const std::vector< SymTok > &templ_thesis,
                                         const std::vector< std::vector< LabTok > > &hyps_proofs, const std::unordered_map< SymTok, std::vector< SymTok > > &subst_map) const;
    bool proving_helper2(const std::vector< std::vector< SymTok > > &templ_hyps,
                         const std::vector< SymTok > &templ_thesis,
                         const std::vector< std::function< bool(const LibraryInterface&, ProofEngine&) > > &hyps_provers,
                         const std::unordered_map< SymTok, std::vector< SymTok > > &subst_map,
                         ProofEngine &engine) const;
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
    const LibraryInterface &lib;
};

#endif // LIBRARYTOOLBOX_H
