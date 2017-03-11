#ifndef WEBENDPOINT_H
#define WEBENDPOINT_H

#include "httpd.h"
#include "library.h"

int httpd_main(int argc, char *argv[]);

class Session {
public:
    Session();
};

class WebEndpoint : public HTTPTarget {
public:
    WebEndpoint();
    std::string answer(HTTPCallback &cb, std::string url, std::string method, std::string version);
    std::string create_session_and_ticket();
private:
    std::unordered_map< std::string, std::string > session_tickets;
    std::unordered_map< std::string, Session > sessions;
};

#endif // WEBENDPOINT_H
