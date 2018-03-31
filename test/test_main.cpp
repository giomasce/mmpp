
#define BOOST_TEST_MODULE mmpp
#define BOOST_TEST_NO_MAIN

#include "utils/utils.h"

int test_main(int argc, char *argv[]) {
    return boost::unit_test::unit_test_main(&init_unit_test, argc, argv);
}
static_block {
    register_main_function("test", test_main);
}
