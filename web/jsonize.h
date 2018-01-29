#pragma once

#include "libs/json.h"
#include "mm/library.h"
#include "step.h"

nlohmann::json jsonize(const ExtendedLibraryAddendum &addendum);
nlohmann::json jsonize(const Assertion &assertion);
nlohmann::json jsonize(const ProofTree< Sentence > &proof_tree);
nlohmann::json jsonize(Step &step);
