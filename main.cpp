
#include <iostream>
#include <string>
#include <cstring>

#include <giolib/static_block.h>
#include <giolib/main.h>

#include "utils/utils.h"
#include "platform.h"

bool mmpp_abort = false;

[[noreturn]] void terminator() {
    std::cerr << "Program terminated\n";
    default_exception_handler(std::current_exception());
    platform_dump_stack_trace(std::cerr, platform_get_stack_trace());
    std::abort();
}

// See the comment in test/test.cpp
#if (!defined(_WIN32)) || (!defined(TEST_BUILD))

int main(int argc, char *argv[]) {
    std::set_terminate(terminator);
    platform_set_max_ram(4 * 1024 * 1024 * 1024LL);
    return gio::main(argc, argv);
}

#endif
