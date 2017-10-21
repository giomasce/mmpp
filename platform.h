#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>

#include <boost/filesystem.hpp>

void set_max_ram(uint64_t bytes);
void set_max_ram32(uint64_t bytes);
bool platform_init(int argc, char *argv[]);
bool platform_should_stop();
bool platform_open_browser(std::string browser_url);
boost::filesystem::path platform_get_resources_base();
size_t platform_get_peak_rss();
size_t platform_get_current_rss();

#endif // PLATFORM_H
