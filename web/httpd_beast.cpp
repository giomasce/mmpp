
#include "httpd_beast.h"

#include "utils/utils.h"

int beast_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    return 0;
}
static_block {
    register_main_function("beast", beast_main);
}
