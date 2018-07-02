#pragma once

#include <string>
#include <thread>
#include <cstdint>
#include <functional>

#include <boost/filesystem.hpp>

/* Windows is not supported by backward, so we have to use boost.
 */
#if (defined(_WIN32))
#define STACKTRACE_USE_BOOST
#endif

/* Under macOS neither boost nor backward give very good results (neither is
 * able to reconstruct line numbers). We default to backtrace because
 * it has a somewhat nicer output.
 */
#if (defined(__APPLE__) && defined(__MACH__))
#define STACKTRACE_USE_BACKWARD

// If you want you can switch to boost
//#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
//#define STACKTRACE_USE_BOOST
#endif

/* Under Linux backward has more features than boost; moreover, since the header
 * is in the repository, it works on Ubuntu 16.04 too, which ships an older version
 * of boost. Therefore we default to backtrace.
 */
#if (defined(__linux) || defined(__linux__))
// By default we use backtrace wth libbfd (compile with -lbfd)
#define BACKWARD_HAS_BFD 1
#define STACKTRACE_USE_BACKWARD

// If you want you can switch to boost (compile with -lbacktrace)
//#define BOOST_STACKTRACE_USE_BACKTRACE
//#define STACKTRACE_USE_BOOST
#endif

#ifdef STACKTRACE_USE_BOOST
#include <boost/stacktrace.hpp>
typedef boost::stacktrace::stacktrace PlatformStackTrace;
#endif

#ifdef STACKTRACE_USE_BACKWARD
#include "libs/backward.h"
typedef backward::StackTrace PlatformStackTrace;
#endif

void platform_set_max_ram(uint64_t bytes);
bool platform_webmmpp_init(int argc, char *argv[]);
void platform_webmmpp_main_loop(const std::function< void() > &new_session_callback);
bool platform_open_browser(std::string browser_url);
boost::filesystem::path platform_get_resources_base();
uint64_t platform_get_peak_used_ram();
uint64_t platform_get_current_used_ram();
void platform_set_current_thread_name(const std::string &name);
void platform_set_current_thread_low_priority();
PlatformStackTrace platform_get_stack_trace();
void platform_dump_stack_trace(std::ostream &str, const PlatformStackTrace &trace);
std::string platform_type_of_current_exception();
