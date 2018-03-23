#pragma once

#include <string>
#include <thread>
#include <cstdint>
#include <functional>

#include <boost/filesystem.hpp>

void platform_set_max_ram(uint64_t bytes);
bool platform_webmmpp_init(int argc, char *argv[]);
void platform_webmmpp_main_loop(const std::function< void() > &new_session_callback);
bool platform_open_browser(std::string browser_url);
boost::filesystem::path platform_get_resources_base();
uint64_t platform_get_peak_used_ram();
uint64_t platform_get_current_used_ram();
void platform_set_current_thread_name(const std::string &name);
void platform_set_current_thread_low_priority();
