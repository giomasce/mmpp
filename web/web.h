#ifndef WEBENDPOINT_H
#define WEBENDPOINT_H

#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "libs/json.h"

#include "web/httpd.h"
#include "workset.h"

class SendError {
public:
    SendError(unsigned int status_code);
    unsigned int get_status_code();
    std::string get_descr();

private:
    unsigned int status_code;
    const std::unordered_map< unsigned int, std::string > errors = {
        { 404, "Not found" },
        { 403, "Forbidden" },
    };
};

int safe_stoi(std::string s);

class Session {
public:
    Session(bool constant = false);
    nlohmann::json answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end, std::string method);
    bool is_constant();
    std::pair< size_t, std::shared_ptr< Workset > > create_workset();
    std::shared_ptr< Workset > get_workset(size_t id);
    nlohmann::json json_list_worksets();

private:
    std::shared_mutex worksets_mutex;
    std::vector< std::shared_ptr< Workset > > worksets;

    bool constant;
};

class WebEndpoint : public HTTPTarget {
public:
    WebEndpoint(int port, bool enable_guest_session);
    void answer(HTTPCallback &cb);
    std::shared_ptr< Session > get_guest_session();
    std::string create_session_and_ticket();

private:
    std::shared_ptr<Session> get_session(std::string session_id);

    std::shared_mutex sessions_mutex;
    std::unordered_map< std::string, std::string > session_tickets;
    std::unordered_map< std::string, std::shared_ptr< Session > > sessions;

    int port;
    std::shared_ptr< Session > guest_session;
};

#endif // WEBENDPOINT_H
