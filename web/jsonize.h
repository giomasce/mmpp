#ifndef JSONIZE_H
#define JSONIZE_H

#include "libs/json.h"
#include "library.h"
#include "step.h"

nlohmann::json jsonize(const ExtendedLibraryAddendum &addendum);
nlohmann::json jsonize(const Assertion &assertion);
nlohmann::json jsonize(const ProofTree &proof_tree);
nlohmann::json jsonize(const Step &step);

#endif // JSONIZE_H
