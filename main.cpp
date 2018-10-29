
#include <iostream>
#include <string>
#include <cstring>

#include <giolib/static_block.h>
#include <giolib/main.h>
#include <giolib/exception.h>

#include "utils/utils.h"
#include "platform.h"

// See the comment in test/test.cpp
#if (!defined(_WIN32)) || (!defined(TEST_BUILD))

int main(int argc, char *argv[]) {
    gio::install_default_terminator();
    platform_set_max_ram(4 * 1024 * 1024 * 1024LL);
    return gio::main(argc, argv);
}

#endif
