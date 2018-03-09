
#include "platform.h"

#if defined(__linux) || defined(__linux__)

#include <csignal>
#include <atomic>
#include <cstdlib>
#include <unistd.h>
#include <sys/resource.h>
#include <cstdio>
#include <pthread.h>
#include <sys/syscall.h>

using namespace std;

void set_max_ram(uint64_t bytes) {
    struct rlimit64 limit;
    limit.rlim_cur = bytes;
    limit.rlim_max = bytes;
    setrlimit64(RLIMIT_AS, &limit);
}

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
    //system(("setsid xdg-open " + browser_url+ "&").c_str());
    system(("setsid chromium " + browser_url + "&").c_str());
    return true;
}

// FIXME
boost::filesystem::path platform_get_resources_base() {
    return boost::filesystem::path(__FILE__).parent_path() / "resources";
}

// Here we depend a lot on implementation details of C++ threads
void set_thread_name(std::thread &t, const string &name) {
    pthread_t handle = t.native_handle();
    pthread_setname_np(handle, name.c_str());
}

void set_current_thread_low_priority() {
    pthread_t handle = pthread_self();
    int policy;
    sched_param sched;
    pthread_getschedparam(handle, &policy, &sched);
    // The meaning of these parameters is somewhat complicated; see sched(7)
    policy = SCHED_BATCH;
    sched.sched_priority = 0;
    // We also set the nice level, that on Linux is thread-specific
    setpriority(PRIO_PROCESS, syscall(SYS_gettid), 19);
}


// Memory functions taken from http://nadeausoftware.com/articles/2012/07/c_c_tip_how_get_process_resident_set_size_physical_memory_use

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

uint64_t platform_get_peak_used_ram( )
{
    struct rusage rusage;
    getrusage( RUSAGE_SELF, &rusage );
    return (size_t)(rusage.ru_maxrss * 1024L);
}

uint64_t platform_get_current_used_ram( )
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

#elif (defined(__APPLE__) && defined(__MACH__))

#include <csignal>
#include <atomic>
#include <cstdlib>
#include <unistd.h>
#include <sys/resource.h>
#include <cstdio>
#include <pthread.h>
#include <mach/mach.h>

using namespace std;

void set_max_ram(uint64_t bytes) {
    struct rlimit limit;
    limit.rlim_cur = bytes;
    limit.rlim_max = bytes;
    setrlimit(RLIMIT_RSS, &limit);
}

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
    system(("open " + browser_url).c_str());
    return true;
}

// FIXME
boost::filesystem::path platform_get_resources_base() {
    //return boost::filesystem::path("./resources");
    return boost::filesystem::path(__FILE__).parent_path() / "resources";
}

// Here we depend a lot on implementation details of C++ threads
void set_thread_name(std::thread &t, const string &name) {
    (void) t;
    (void) name;
    /* Apparently in macOS pthread_setname_np() can only be used 
       to change the name of the calling thread. */
}

void set_current_thread_low_priority() {
    // TODO
}


// Memory functions taken from http://nadeausoftware.com/articles/2012/07/c_c_tip_how_get_process_resident_set_size_physical_memory_use

/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

uint64_t platform_get_peak_used_ram( )
{
    struct rusage rusage;
    getrusage( RUSAGE_SELF, &rusage );
    return (size_t)rusage.ru_maxrss;
}

uint64_t platform_get_current_used_ram( )
{
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount ) != KERN_SUCCESS ) {
        return (size_t)0L;              /* Can't access? */
    } else {
        return (size_t)info.resident_size;
    }
}

#elif (defined(_WIN32))

#include <atomic>

#include <windows.h>
#include <psapi.h>

using namespace std;

void set_max_ram(uint64_t bytes){
    auto job_handle = CreateJobObjectA(nullptr, nullptr);
    AssignProcessToJobObject(job_handle, GetCurrentProcess());
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ext_limit_info;
    DWORD set_len;
    QueryInformationJobObject(job_handle, JobObjectExtendedLimitInformation, &ext_limit_info, sizeof(ext_limit_info), &set_len);
    ext_limit_info.JobMemoryLimit = bytes;
    ext_limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_JOB_MEMORY;
    SetInformationJobObject(job_handle, JobObjectExtendedLimitInformation, &ext_limit_info, sizeof(ext_limit_info));
    // FIXME When does the Job get destroyed?
}

atomic< bool > signalled;
BOOL ctrl_handler(DWORD type) {
    if (type == CTRL_C_EVENT) {
        signalled = true;
        return true;
    }
    return false;
}

bool platform_init(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    SetConsoleCtrlHandler((PHANDLER_ROUTINE) ctrl_handler, TRUE);

    return true;
}

bool platform_should_stop() {
    return signalled;
}

#pragma comment(lib, "shell32.lib")
bool platform_open_browser(std::string browser_url) {
    ShellExecuteA(NULL, NULL, browser_url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return true;
}

// FIXME
boost::filesystem::path platform_get_resources_base() {
    return boost::filesystem::path(__FILE__).parent_path() / "resources";
}

// From https://stackoverflow.com/a/64166 and MSDN docs
uint64_t platform_get_peak_used_ram() {
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.PeakWorkingSetSize;
}

uint64_t platform_get_current_used_ram() {
    PROCESS_MEMORY_COUNTERS pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.WorkingSetSize;
}

// It does not seem to be meaningful on Windows machines
void set_thread_name(std::thread &t, const std::string &name) {
    (void) t;
    (void) name;
}

void set_current_thread_low_priority() {
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
}

#else
#error Current platform is not supported. Please add support in plaftorm.cpp.
#endif
