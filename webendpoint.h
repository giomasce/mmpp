#ifndef WEBENDPOINT_H
#define WEBENDPOINT_H

#include "httpd.h"
#include "library.h"

class WebEndpoint : public HTTPTarget {
public:
    WebEndpoint(const Library &lib);
    std::string answer(HTTPCallback &cb, std::string url, std::string method, std::string version);
private:
    const Library &lib;
};

#endif // WEBENDPOINT_H
