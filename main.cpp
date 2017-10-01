
#include <iostream>
#include <string>
#include <cstring>

#include <libgen.h>

#include "utils.h"

using namespace std;

bool mmpp_abort = false;

//const string DEFAULT_MAIN_FUNCTION = "mmpp_test_z3";
//const string DEFAULT_MAIN_FUNCTION = "mmpp_test_all";
const string DEFAULT_MAIN_FUNCTION = "unification_test";

int main(int argc, char *argv[]) {
    char *tmp = strdup(argv[0]);
    string bname(basename(tmp));
    // string constructor should have made a copy
    free(tmp);
    function< int(int, char*[]) > main_func;
    try {
        main_func = get_main_functions().at(bname);
    } catch (out_of_range e) {
        (void) e;
        // Return a default one
        try {
            main_func = get_main_functions().at(DEFAULT_MAIN_FUNCTION);
        } catch (out_of_range e) {
            cerr << "Could not find the main function..." << endl;
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
