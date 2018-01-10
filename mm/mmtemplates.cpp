
#include "mmtemplates.h"

template class std::vector< SymTok >;
template class std::vector< LabTok >;

template class LRParser< SymTok, LabTok >;
template class LRParsingHelper< SymTok, LabTok >;

template class UnilateralUnificator< SymTok, LabTok >;
template class BilateralUnificator< SymTok, LabTok >;

template class ParsingTree< SymTok, LabTok >;
template class ParsingTree2< SymTok, LabTok >;
template class ParsingTreeNode< SymTok, LabTok >;
