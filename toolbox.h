#ifndef LIBRARYTOOLBOX_H
#define LIBRARYTOOLBOX_H

#include "library.h"

#include <vector>
#include <unordered_map>

class LibraryToolbox
{
public:
    LibraryToolbox(const LibraryInterface &lib);
    std::vector< SymTok > substitute(const std::vector< SymTok > &orig, const std::unordered_map< SymTok, std::vector< SymTok > > &subst_map) const;
    std::unordered_map< SymTok, std::vector< SymTok > > compose_subst(const std::unordered_map< SymTok, std::vector< SymTok > > &first,
                                                                      const std::unordered_map< SymTok, std::vector< SymTok > > &second) const;
    std::vector< LabTok > proving_helper(const std::vector< std::vector< SymTok > > &templ_hyps, const std::vector< SymTok > &templ_thesis,
                                         const std::vector< std::vector< LabTok > > &hyps_proofs, const std::unordered_map< SymTok, std::vector< SymTok > > &subst_map) const;
private:
    const LibraryInterface &lib;
};

#endif // LIBRARYTOOLBOX_H
