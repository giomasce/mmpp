#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>

#include <boost/filesystem.hpp>

bool platform_init(int argc, char *argv[]);
bool platform_should_stop();
bool platform_open_browser(std::string browser_url);
boost::filesystem::path platform_get_resources_base();
size_t platform_get_peak_rss();
size_t platform_get_current_rss();

#endif // PLATFORM_H
