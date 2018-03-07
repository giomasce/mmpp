#pragma once

#include <string>
#include <thread>
#include <cstdint>

#include <boost/filesystem.hpp>

void set_max_ram(uint64_t bytes);
bool platform_init(int argc, char *argv[]);
bool platform_should_stop();
bool platform_open_browser(std::string browser_url);
boost::filesystem::path platform_get_resources_base();
uint64_t platform_get_peak_used_ram();
uint64_t platform_get_current_used_ram();
void set_thread_name(std::thread &t, const std::string &name);
void set_current_thread_low_priority();
