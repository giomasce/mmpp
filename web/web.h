#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>

#include <giolib/memory.h>

#include "libs/json.h"

class Session;

#include "web/httpd.h"
#include "workset.h"
#include "utils/utils.h"

class SendError {
public:
    SendError(unsigned int status_code) : status_code(status_code)
    {
    }

    unsigned int get_status_code()
    {
        return this->status_code;
    }

    std::string get_descr()
    {
        return this->errors.at(this->get_status_code());
    }

private:
    unsigned int status_code;
    const std::unordered_map< unsigned int, std::string > errors = {
        { 403, "Forbidden" },
        { 404, "Not found" },
        { 405, "Method Not Allowed" },
        { 422, "Unprocessable Entity" },
        { 500, "Internal Server Error"},
    };
};

class WaitForPost {
public:
    WaitForPost(const std::function< nlohmann::json(const std::unordered_map< std::string, PostItem >&) > &callback) : callback(callback) {
    }

    const std::function< nlohmann::json(const std::unordered_map< std::string, PostItem >&) > &get_callback() const {
        return this->callback;
    }

private:
    std::function< nlohmann::json(const std::unordered_map< std::string, PostItem >&) > callback;
};

int safe_stoi(const std::string &s);

template< typename Container >
const typename Container::mapped_type &safe_at(const Container &cont, const typename Container::key_type &key) {
    try {
        return cont.at(key);
    } catch (std::out_of_range&) {
        throw SendError(404);
    }
}

template< typename Container >
typename Container::mapped_type safe_at(Container &cont, const typename Container::key_type &key) {
    try {
        return cont.at(key);
    } catch (std::out_of_range&) {
        throw SendError(404);
    }
}

class Session : public gio::virtual_enable_create< Session >{
public:
    nlohmann::json answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end);
    bool is_constant();
    std::pair< size_t, std::shared_ptr< Workset > > create_workset();
    bool destroy_workset(std::shared_ptr< Workset > workset);
    std::shared_ptr< Workset > get_workset(size_t id);
    nlohmann::json json_list_worksets();

protected:
    Session(bool constant = false);

private:
    std::mutex worksets_mutex;
    std::map< size_t, std::shared_ptr< Workset > > worksets;

    bool constant;
    size_t new_id;
};

class WebEndpoint : public HTTPTarget {
public:
    WebEndpoint(int port, bool enable_guest_session);
    void answer(HTTPCallback &cb);
    std::shared_ptr< Session > get_guest_session();
    std::string create_session_and_ticket();

private:
    std::shared_ptr<Session> get_session(std::string session_id);

    std::mutex sessions_mutex;
    std::unordered_map< std::string, std::string > session_tickets;
    std::unordered_map< std::string, std::shared_ptr< Session > > sessions;

    int port;
    std::shared_ptr< Session > guest_session;
};
