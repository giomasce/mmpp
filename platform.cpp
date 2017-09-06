
#include "platform.h"

#if defined(__linux) || defined(__linux__)
#include <csignal>
#include <atomic>
#include <cstdlib>

using namespace std;

atomic< bool > signalled;
void int_handler(int signal) {
    (void) signal;
    signalled = true;
}

bool platform_init(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    struct sigaction act;
    act.sa_handler = int_handler;
    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    return true;
}

bool platform_should_stop() {
    return signalled;
}

bool platform_open_browser(string browser_url) {
    system(("xdg-open " + browser_url).c_str());
    //system(("chromium " + browser_url).c_str());
    return true;
}

// FIXME
string platform_get_resources_base() {
    return "./resources";
}

#else
#error Current platform is not supported. Please add support in plaftorm.cpp.
#endif
