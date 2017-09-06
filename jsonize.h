#ifndef JSONIZE_H
#define JSONIZE_H

#include "json.h"
#include "library.h"

nlohmann::json jsonize(const ExtendedLibraryAddendum &addendum);

#endif // JSONIZE_H
