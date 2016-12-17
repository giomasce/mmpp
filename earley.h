#ifndef EARLEY_H
#define EARLEY_H

#include "library.h"

#include <vector>
#include <unordered_map>

struct EarleyTreeItem {
    LabTok label;
    std::vector< EarleyTreeItem > children;
};

EarleyTreeItem earley(const std::vector<SymTok> &sent, SymTok type, const std::unordered_map<SymTok, std::vector<std::pair<LabTok, std::vector<SymTok> > > > &derivations);

#endif // EARLEY_H
