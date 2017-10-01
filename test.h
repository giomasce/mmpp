#ifndef TEST_H
#define TEST_H

#include <string>

#include "library.h"
#include "toolbox.h"

/* This funny construction with two classes is the only way I found to have lib and tb
 * as references in TestEnvironment, while being at the same time able to do the
 * (somewhat) complex construction procedure they need. I wonder if there is some better way.
 */
struct TestEnvironmentInner {
    TestEnvironmentInner(const std::string &filename, const std::string &cache_filename);
    ~TestEnvironmentInner();

    LibraryImpl *lib;
    LibraryToolbox *tb;
    std::string *lib_digest;
};

struct TestEnvironment {
    TestEnvironment(const std::string &filename, const std::string &cache_filename);

    TestEnvironmentInner inner;
    LibraryImpl &lib;
    LibraryToolbox &tb;
    std::string &lib_digest;
};

const TestEnvironment &get_set_mm();

#endif // TEST_H
