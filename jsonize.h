#ifndef JSONIZE_H
#define JSONIZE_H

#include "json.h"
#include "library.h"

nlohmann::json jsonize(const ExtendedLibraryAddendum &addendum);
nlohmann::json jsonize(const Assertion &assertion);

#endif // JSONIZE_H
