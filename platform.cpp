
#include "platform.h"

#if (defined(__linux) || defined(__linux__))

#include <csignal>
#include <atomic>
#include <cstdlib>
#include <unistd.h>
#include <sys/resource.h>
#include <cstdio>
#include <pthread.h>
#include <sys/syscall.h>
#include <iostream>

void platform_set_max_ram(uint64_t bytes) {
    struct rlimit64 limit;
    limit.rlim_cur = bytes;
    limit.rlim_max = bytes;
    setrlimit64(RLIMIT_AS, &limit);
}

std::vector< int > signals = { SIGTERM, SIGHUP, SIGUSR1 };
sigset_t create_sigset() {
    sigset_t sigset;
    int res = sigemptyset(&sigset);
    if (res) {
        throw "Error when calling sigemptyset()";
    }
    for (const auto signal : signals) {
        res = sigaddset(&sigset, signal);
        if (res) {
            throw "Error when calling sigaddset()";
        }
    }
    return sigset;
}

bool platform_webmmpp_init(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    /*struct sigaction act;
    act.sa_handler = int_handler;
    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);*/

    /* Block the signals we want to receive synchronously;
     * this should be done before any thread is created,
     * so that all threads inherit the same blocking mask (I hope) */
    int res;
    sigset_t sigset = create_sigset();
    res = sigprocmask(SIG_BLOCK, &sigset, nullptr);
    if (res) {
        throw "Error when calling sigprocmask()";
    }

    return true;
}

void platform_webmmpp_main_loop(const std::function<void()> &new_session_callback) {
    sigset_t sigset = create_sigset();
    bool running = true;
    while (running) {
        siginfo_t siginfo;
        int res = sigwaitinfo(&sigset, &siginfo);
        if (res < 0) {
            continue;
        }
        switch (siginfo.si_signo) {
        case SIGTERM:
            std::cerr << "SIGTERM received!" << std::endl;
            running = false;
            break;
        case SIGHUP:
            std::cerr << "SIGHUP received!" << std::endl;
            running = false;
            break;
        case SIGUSR1:
            std::cerr << "SIGUSR1 received!" << std::endl;
            new_session_callback();
            break;
        }
    }
}

bool platform_open_browser(std::string browser_url) {
    const char *browser = std::getenv("BROWSER");
    if (!browser) {
        browser = "xdg-open";
    }
    system((std::string("setsid ") + browser + " " + browser_url + "&").c_str());
    return true;
}

boost::filesystem::path platform_get_resources_base() {
    return boost::filesystem::path(PROJ_DIR) / "resources";
}

// Here we depend a lot on implementation details of C++ threads
void platform_set_current_thread_name(const std::string &name) {
    pthread_t handle = pthread_self();
    pthread_setname_np(handle, name.c_str());
}

void platform_set_current_thread_low_priority() {
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
    FILE* fp = nullptr;
    if ( (fp = fopen( "/proc/self/statm", "r" )) == nullptr )
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
#include <iostream>

void platform_set_max_ram(uint64_t bytes) {
    struct rlimit limit;
    limit.rlim_cur = bytes;
    limit.rlim_max = bytes;
    setrlimit(RLIMIT_RSS, &limit);
}

std::vector< int > signals = { SIGTERM, SIGUSR1 };
sigset_t create_sigset() {
    sigset_t sigset;
    int res = sigemptyset(&sigset);
    if (res) {
        throw "Error when calling sigemptyset()";
    }
    for (const auto signal : signals) {
        res = sigaddset(&sigset, signal);
        if (res) {
            throw "Error when calling sigaddset()";
        }
    }
    return sigset;
}

bool platform_webmmpp_init(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    /*struct sigaction act;
    act.sa_handler = int_handler;
    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    sigaction(SIGINT, &act, nullptr);
    sigaction(SIGTERM, &act, nullptr);*/

    /* Block the signals we want to receive synchronously;
     * this should be done before any thread is created,
     * so that all threads inherit the same blocking mask (I hope) */
    int res;
    sigset_t sigset = create_sigset();
    res = sigprocmask(SIG_BLOCK, &sigset, nullptr);
    if (res) {
        throw "Error when calling sigprocmask()";
    }

    return true;
}

void platform_webmmpp_main_loop(const std::function<void()> &new_session_callback) {
    sigset_t sigset = create_sigset();
    bool running = true;
    while (running) {
        int signo;
        int res = sigwait(&sigset, &signo);
        if (res) {
            throw "Error when calling sigwait()";
        }
        switch (signo) {
        case SIGTERM:
            std::cerr << "SIGTERM received!" << std::endl;
            running = false;
            break;
        case SIGUSR1:
            std::cerr << "SIGUSR1 received!" << std::endl;
            new_session_callback();
            break;
        }
    }
}

bool platform_open_browser(std::string browser_url) {
    system(("open " + browser_url).c_str());
    return true;
}

boost::filesystem::path platform_get_resources_base() {
    return boost::filesystem::path(PROJ_DIR) / "resources";
}

// Here we depend a lot on implementation details of C++ threads
void platform_set_current_thread_name(const std::string &name) {
    pthread_setname_np(name.c_str());
}

void platform_set_current_thread_low_priority() {
    pthread_t handle = pthread_self();
    int policy;
    sched_param sched;
    pthread_getschedparam(handle, &policy, &sched);
    policy = SCHED_OTHER;
    sched.sched_priority = 0; //sched_get_priority_min();
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

#include <future>
#include <iostream>

#include <windows.h>
#include <psapi.h>

void platform_set_max_ram(uint64_t bytes){
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

std::promise< void > exit_promise;
std::future< void > exit_future;
bool promise_set = false;
BOOL ctrl_handler(DWORD type) {
    if (type == CTRL_C_EVENT) {
        if (!promise_set) {
            std::cerr << "Ctrl-C received!" << std::endl;
            exit_promise.set_value();
            promise_set = true;
        }
        return true;
    }
    return false;
}

bool platform_webmmpp_init(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    exit_future = exit_promise.get_future();
    SetConsoleCtrlHandler((PHANDLER_ROUTINE) ctrl_handler, TRUE);

    return true;
}

void platform_webmmpp_main_loop(const std::function<void ()> &new_session_callback) {
    (void) new_session_callback;
    assert(exit_future.valid());
    exit_future.wait();
}

#pragma comment(lib, "shell32.lib")
bool platform_open_browser(std::string browser_url) {
    ShellExecuteA(nullptr, nullptr, browser_url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return true;
}

boost::filesystem::path platform_get_resources_base() {
    return boost::filesystem::path(PROJ_DIR) / "resources";
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

// Wonderful clean API taken from https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
// Usage: SetThreadName ((DWORD)-1, "MainThread");
#include <windows.h>
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
 } THREADNAME_INFO;
#pragma pack(pop)
void SetThreadName(DWORD dwThreadID, const char* threadName) {
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try{
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER){
    }
#pragma warning(pop)
}

// It does not seem to be meaningful on Windows machines
void platform_set_current_thread_name(const std::string &name) {
    SetThreadName((DWORD) -1, name.c_str());
}

void platform_set_current_thread_low_priority() {
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
}

#else
#error Current platform is not supported. Please add support in plaftorm.cpp.
#endif

#ifdef STACKTRACE_USE_BACKWARD

PlatformStackTrace platform_get_stack_trace() {
    backward::StackTrace ret;
    ret.load_here();
    return ret;
}

void platform_dump_stack_trace(std::ostream &st, const backward::StackTrace &stacktrace) {
    backward::Printer p;
    p.snippet = false;
    p.color_mode = backward::ColorMode::always;
    p.address = true;
    p.object = false;
    p.print(stacktrace, st);
    st.flush();
}

#endif

#ifdef STACKTRACE_USE_BOOST

PlatformStackTrace platform_get_stack_trace() {
    return boost::stacktrace::stacktrace();
}

void platform_dump_stack_trace(std::ostream &str, const PlatformStackTrace &trace) {
    str << "Stack trace (more recent call first)" << std::endl;
    str << trace;
}

#endif
