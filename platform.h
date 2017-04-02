#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>

bool platform_init(int argc, char *argv[]);
bool platform_should_stop();
bool platform_open_browser(std::string browser_url);
std::string platform_get_resources_base();

#endif // PLATFORM_H
