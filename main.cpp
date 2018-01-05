
#include <iostream>
#include <string>
#include <cstring>

#include <libgen.h>

#include "utils/utils.h"
#include "platform.h"

using namespace std;

bool mmpp_abort = false;

//const string DEFAULT_MAIN_FUNCTION = "mmpp_test_z3";
//const string DEFAULT_MAIN_FUNCTION = "mmpp_test_setmm";
const string DEFAULT_MAIN_FUNCTION = "mmpp";

void list_subcommands() {
    cout << "Supported subcommands:" << endl;
    for (const auto &subcmd : get_main_functions()) {
        cout << " * " << subcmd.first << endl;
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
    string subcmd(argv[1]);
    function< int(int, char*[]) > main_func;
    try {
        main_func = get_main_functions().at(subcmd);
    } catch (out_of_range) {
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
    char *tmp = strdup(argv[0]);
    string bname(basename(tmp));
    // string constructor should have made a copy
    free(tmp);
    function< int(int, char*[]) > main_func;
    try {
        main_func = get_main_functions().at(bname);
    } catch (out_of_range) {
        // Return a default one
        try {
            main_func = get_main_functions().at(DEFAULT_MAIN_FUNCTION);
        } catch (out_of_range e) {
            list_subcommands();
            return 1;
        }
    }
    //return main_func(argc, argv);
    try {
        return main_func(argc, argv);
    } catch (const MMPPException &e) {
        cerr << "Dying because of MMPPException with reason '" << e.get_reason() << "'..." << endl;
        return 1;
    } catch (const char* &e) {
        cerr << "Dying because of string exception '" << e << "'..." << endl;
        return 1;
    }
}
