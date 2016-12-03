#ifndef UNIFICATION_H
#define UNIFICATION_H

#include "library.h"

#include <vector>
#include <unordered_map>

std::vector< std::unordered_map< SymTok, std::vector< SymTok > > > unify(const std::vector< SymTok > &sent, const std::vector< SymTok > &templ, const LibraryInterface &lib, bool allow_empty=false);

#endif // UNIFICATION_H
