#pragma once

#include <string>

#include "mmtemplates.h"
#include "toolbox.h"

ParsingTree< SymTok, LabTok > create_var_pt(const LibraryToolbox &tb, SymTok var);
ParsingTree< SymTok, LabTok > create_temp_var_pt(const LibraryToolbox &tb, SymTok type);
ParsingTree< SymTok, LabTok > create_pt(const LibraryToolbox &tb, const std::string &templ_str, const std::map< std::string, ParsingTree< SymTok, LabTok > > &subst_str);
