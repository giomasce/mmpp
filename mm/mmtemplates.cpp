
#include "mmtemplates.h"

template class std::vector< SymTok >;
//template class std::vector< LabTok >;

template class LRParser< SymTok, LabTok >;
template class LRParsingHelper< SymTok, LabTok >;

template class UnilateralUnificator< SymTok, LabTok >;
template class BilateralUnificator< SymTok, LabTok >;

template struct ParsingTree< SymTok, LabTok >;
template struct ParsingTree2< SymTok, LabTok >;
template struct ParsingTreeNode< SymTok, LabTok >;
