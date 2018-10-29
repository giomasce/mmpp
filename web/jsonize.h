#pragma once

#include <giolib/containers.h>

#include "libs/json.h"
#include "mm/library.h"
#include "step.h"

template< typename TokType >
decltype(auto) tok_to_int_vect(const std::vector< TokType > &x) {
    return gio::vector_map(x.begin(), x.end(), [](const auto &x) { return x.val(); });
}

nlohmann::json jsonize(const ExtendedLibraryAddendum &addendum);
nlohmann::json jsonize(const Assertion &assertion);
nlohmann::json jsonize(const ProofTree< Sentence > &proof_tree);
nlohmann::json jsonize(Step &step);
