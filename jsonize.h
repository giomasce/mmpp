#ifndef JSONIZE_H
#define JSONIZE_H

#include "json.h"
#include "library.h"

nlohmann::json jsonize(const ExtendedLibraryAddendum &addendum);
nlohmann::json jsonize(const Assertion &assertion);
nlohmann::json jsonize(const ProofTree &proof_tree);

#endif // JSONIZE_H
