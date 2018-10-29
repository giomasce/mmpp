#pragma once

#include <string>
#include <thread>
#include <cstdint>
#include <functional>

#include <boost/filesystem.hpp>

#include <giolib/platform.h>

#ifdef GIO_PLATFORM_LINUX
#define COROUTINE_USE_COROUTINE2
#endif

#ifdef GIO_PLATFORM_MACOS
#define COROUTINE_USE_COROUTINE
#endif

#ifdef GIO_PLATFORM_WIN32
#define COROUTINE_USE_COROUTINE
#endif

#ifdef COROUTINE_USE_COROUTINE
#define BOOST_COROUTINES_NO_DEPRECATION_WARNING
#include <boost/coroutine/all.hpp>
template< typename T >
using coroutine_pull = typename boost::coroutines::asymmetric_coroutine< T >::pull_type;
template< typename T >
using coroutine_push = typename boost::coroutines::asymmetric_coroutine< T >::push_type;
using coroutine_forced_unwind = boost::coroutines::detail::forced_unwind;
#endif

#ifdef COROUTINE_USE_COROUTINE2
#include <boost/coroutine2/all.hpp>
template< typename T >
using coroutine_pull = typename boost::coroutines2::coroutine< T >::pull_type;
template< typename T >
using coroutine_push = typename boost::coroutines2::coroutine< T >::push_type;
using coroutine_forced_unwind = boost::context::detail::forced_unwind;
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
