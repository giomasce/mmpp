#pragma once

#include <string>

#include "mm/library.h"
#include "mm/toolbox.h"

/* This funny construction with two classes is the only way I found to have lib and tb
 * as references in TestEnvironment, while being at the same time able to do the
 * (somewhat) complex construction procedure they need. I wonder if there is some better way.
 */
struct TestEnvironmentInner {
    TestEnvironmentInner(const boost::filesystem::path &filename, const boost::filesystem::path &cache_filename);
    ~TestEnvironmentInner();

    LibraryImpl *lib;
    LibraryToolbox *tb;
};

struct TestEnvironment {
    TestEnvironment(const boost::filesystem::path &filename, const boost::filesystem::path &cache_filename);

    TestEnvironmentInner inner;
    LibraryImpl &lib;
    LibraryToolbox &tb;
};

const TestEnvironment &get_set_mm();
