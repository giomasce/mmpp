
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

int main(int argc, char *argv[]) {
    set_max_ram(4 * 1024 * 1024 * 1024LL);
    boost::filesystem::path exec_path(argv[0]);
    std::string bname = exec_path.filename().string();
    std::function< int(int, char*[]) > main_func;
    try {
        main_func = get_main_functions().at(bname);
    } catch (std::out_of_range&) {
        // Return a default one
        try {
            main_func = get_main_functions().at(DEFAULT_MAIN_FUNCTION);
        } catch (std::out_of_range &e) {
            list_subcommands();
            return 1;
        }
    }
    //return main_func(argc, argv);
    try {
        return main_func(argc, argv);
    } catch (const MMPPException &e) {
        std::cerr << "Dying because of MMPPException with reason '" << e.get_reason() << "'..." << std::endl;
        return 1;
    } catch (const char* &e) {
        std::cerr << "Dying because of string exception '" << e << "'..." << std::endl;
        return 1;
    } catch (std::string &e) {
        std::cerr << "Dying because of string exception '" << e << "'..." << std::endl;
        return 1;
    }
}
