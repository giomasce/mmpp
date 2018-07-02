
#include <iostream>
#include <string>
#include <cstring>

#include <boost/filesystem.hpp>

#include "utils/utils.h"
#include "platform.h"

bool mmpp_abort = false;

//const string DEFAULT_MAIN_FUNCTION = "mmpp_test_z3";
//const string DEFAULT_MAIN_FUNCTION = "mmpp_test_setmm";
const std::string DEFAULT_MAIN_FUNCTION = "mmpp";

void list_subcommands() {
    std::cout << "Supported subcommands:" << std::endl;
    for (const auto &subcmd : get_main_functions()) {
        std::cout << " * " << subcmd.first << std::endl;
    }
}

int main_help(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    list_subcommands();

    return 0;
}
static_block {
    register_main_function("help", main_help);
}

int main_mmpp(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    if (argc == 1) {
        return main_help(argc, argv);
    }
    std::string subcmd(argv[1]);
    std::function< int(int, char*[]) > main_func;
    try {
        main_func = get_main_functions().at(subcmd);
    } catch (std::out_of_range&) {
        list_subcommands();
        return 1;
    }
    return main_func(argc-1, argv+1);
}
static_block {
    register_main_function("mmpp", main_mmpp);
}

void terminator() {
    std::cerr << "Program terminated by an uncaught exception" << std::endl;
    default_exception_handler(std::current_exception());
    platform_dump_stack_trace(std::cerr, platform_get_stack_trace());
    std::abort();
}

// See the comment in test/test.cpp
#if (!defined(_WIN32)) || (!defined(TEST_BUILD))

int main(int argc, char *argv[]) {
    std::set_terminate(terminator);
    platform_set_max_ram(4 * 1024 * 1024 * 1024LL);
    boost::filesystem::path exec_path(argv[0]);
    std::string bname = exec_path.filename().string();
    std::function< int(int, char*[]) > main_func;
    try {
        main_func = get_main_functions().at(bname);
    } catch (std::out_of_range&) {
        // Return a default one
        try {
            main_func = get_main_functions().at(DEFAULT_MAIN_FUNCTION);
        } catch (std::out_of_range&) {
            list_subcommands();
            return 1;
        }
    }
    return main_func(argc, argv);
}

#endif
