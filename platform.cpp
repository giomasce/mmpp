
#include "platform.h"

#if defined(__linux) || defined(__linux__)
#include <csignal>
#include <atomic>
#include <cstdlib>
#include <unistd.h>
#include <sys/resource.h>
#include <cstdio>

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
boost::filesystem::path platform_get_resources_base() {
    return boost::filesystem::path("./resources");
}


// Memory functions taken from http://nadeausoftware.com/articles/2012/07/c_c_tip_how_get_process_resident_set_size_physical_memory_use

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
size_t platform_get_peak_rss( )
{
    struct rusage rusage;
    getrusage( RUSAGE_SELF, &rusage );
    return (size_t)(rusage.ru_maxrss * 1024L);
}

/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t platform_get_current_rss( )
{
    long rss = 0L;
    FILE* fp = NULL;
    if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL )
        return (size_t)0L;		/* Can't open? */
    if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
    {
        fclose( fp );
        return (size_t)0L;		/* Can't read? */
    }
    fclose( fp );
    return (size_t)rss * (size_t)sysconf( _SC_PAGESIZE);
}

#else
#error Current platform is not supported. Please add support in plaftorm.cpp.
#endif
