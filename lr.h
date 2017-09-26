#ifndef LR_H
#define LR_H

#include <vector>
#include <unordered_map>
#include <utility>

#include "library.h"

template< typename SymType, typename LabType >
void process_derivations(const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector< SymType > > > > &derivations);

#endif // LR_H
