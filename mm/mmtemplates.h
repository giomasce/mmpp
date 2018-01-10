#pragma once

#include "funds.h"
#include "parsing/lr.h"
#include "parsing/unif.h"

extern template class std::vector< SymTok >;
extern template class std::vector< LabTok >;

extern template class LRParser< SymTok, LabTok >;
extern template class LRParsingHelper< SymTok, LabTok >;

extern template class UnilateralUnificator< SymTok, LabTok >;
extern template class BilateralUnificator< SymTok, LabTok >;

extern template class ParsingTree< SymTok, LabTok >;
extern template class ParsingTree2< SymTok, LabTok >;
extern template class ParsingTreeNode< SymTok, LabTok >;
