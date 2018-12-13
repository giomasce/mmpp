
#include "platform.h"

#ifdef GIO_PLATFORM_LINUX

#include <csignal>
#include <atomic>
#include <cstdlib>
#include <unistd.h>
#include <sys/resource.h>
#include <cstdio>
#include <pthread.h>
#include <sys/syscall.h>
#include <iostream>

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

boost::filesystem::path platform_get_resources_base() {
    return boost::filesystem::path(PROJ_DIR) / "resources";
}

#endif

#ifdef GIO_PLATFORM_MACOS

#include <csignal>
#include <atomic>
#include <cstdlib>
#include <unistd.h>
#include <sys/resource.h>
#include <cstdio>
#include <pthread.h>
#include <mach/mach.h>
#include <iostream>

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

boost::filesystem::path platform_get_resources_base() {
    return boost::filesystem::path(PROJ_DIR) / "resources";
}

#endif

#ifdef GIO_PLATFORM_WIN32

#include <future>
#include <iostream>

#include <windows.h>
#include <psapi.h>

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

boost::filesystem::path platform_get_resources_base() {
    return boost::filesystem::path(PROJ_DIR) / "resources";
}

#endif

#ifdef GIO_PLATFORM_UNKNOWN
#error Current platform is not supported.
#endif
