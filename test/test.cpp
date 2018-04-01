
#define BOOST_TEST_MODULE mmpp

/* On Windows we link statically to boost, because I have not found a way
 * to link dynamically. Unfortunately boost::test, when linked statically,
 * pretends to install its own main() function, and it requires recompilation
 * of the static library to prevent it from doing so. This would be terrible.
 * This means that on Windows we have to either compile the main() function
 * in main.cpp or the tests all around. We regulate this thing with the
 * COMPILE_TEST macro: if it is defined, the tests are compiled and main()
 * is not; if it not defined, the other way around.
 *
 * On all the other platforms, where it is easy to link boost dynamically,
 * there is not such requirement. So we just expose the test framework in
 * the "test" subcommand.
 */

#if (!(defined(_WIN32)))

#define BOOST_TEST_NO_MAIN

#include "test.h"
#include "utils/utils.h"

#include "mm/setmm.h"

int test_main(int argc, char *argv[]) {
    // Load immediately set.mm, so it loading bar does not mix with testing output
    get_set_mm();
    return boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
static_block {
    register_main_function("test", test_main);
}

#else

#include "test/test.h"

#endif
