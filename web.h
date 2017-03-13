#ifndef WEBENDPOINT_H
#define WEBENDPOINT_H

#include <json/json.h>

#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "httpd.h"
#include "library.h"

const std::string RESOURCES_BASE = "./resources";

int httpd_main(int argc, char *argv[]);

class SendError {
public:
    SendError(unsigned int status_code);
    unsigned int get_status_code();
    std::string get_descr();

private:
    unsigned int status_code;
    const std::unordered_map< unsigned int, std::string > errors = {
        { 404, "Not found" },
    };
};

int safe_stoi(std::string s);

class Workset {
public:
    Workset();
    Json::Value to_json() const;
};

class Session {
public:
    Session();
    Json::Value answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end, std::string method);
private:
    std::pair< size_t, std::shared_ptr< Workset > > create_workset();
    std::shared_ptr< Workset > get_workset(size_t id);

    std::shared_mutex worksets_mutex;
    std::vector< std::shared_ptr< Workset > > worksets;
};

class WebEndpoint : public HTTPTarget {
public:
    WebEndpoint();
    std::string answer(HTTPCallback &cb, std::string url, std::string method, std::string version);
    std::string create_session_and_ticket();
private:
    std::shared_ptr<Session> get_session(std::string session_id);

    std::shared_mutex sessions_mutex;
    std::unordered_map< std::string, std::string > session_tickets;
    std::unordered_map< std::string, std::shared_ptr< Session > > sessions;
};

#endif // WEBENDPOINT_H
