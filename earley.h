#ifndef EARLEY_H
#define EARLEY_H

#include "library.h"

#include <vector>
#include <unordered_map>

bool earley(const std::vector<SymTok> &sent, SymTok type, const std::unordered_map<SymTok, std::vector<std::pair<LabTok, std::vector<SymTok> > > > &derivations);

#endif // EARLEY_H
