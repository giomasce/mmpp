
#include <iostream>

#include "mm/library.h"
#include "utils/utils.h"
#include "test/test_env.h"

int resolver_main(int argc, char *argv[]) {
    if (argc == 1) {
        std::cerr << "Specify some symbol on the command line" << std::endl;
        return 1;
    }

    auto &data = get_set_mm();
    auto &lib = data.lib;

    for (int i = 1; i < argc; i++) {
        auto arg = argv[i];
        int num = atoi(arg);
        if (num != 0) {
            std::cout << "Resolving " << arg << " as symbol: " << lib.resolve_symbol(num) << std::endl;
            std::cout << "Resolving " << arg << " as label: " << lib.resolve_label(num) << std::endl;
        } else {
            std::cout << "Resolving " << arg << " as symbol: " << lib.get_symbol(arg) << std::endl;
            std::cout << "Resolving " << arg << " as label: " << lib.get_label(arg) << std::endl;
        }
    }
    return 0;
}
static_block {
    register_main_function("resolver", resolver_main);
}
