#include "httpd.h"

#include <cstring>
#include <sstream>
#include <iostream>

#include <cassert>

#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

HTTPD_microhttpd::HTTPD_microhttpd(int port, HTTPTarget &target) :
    port(port), daemon(NULL), target(target)
{
}

void HTTPD_microhttpd::start()
{
    unique_lock< mutex > lock(this->daemon_mutex);
    this->daemon = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_POLL, this->port,
                                     &this->accept_wrapper, this,
                                     &this->answer_wrapper, this,
                                     MHD_OPTION_NOTIFY_COMPLETED, &this->cleanup_wrapper, NULL,
                                     MHD_OPTION_END);
    if (this->daemon == NULL) {
        throw string("Could not start httpd daemon");
    }
    this->daemon_cv.notify_all();
}

void HTTPD_microhttpd::stop()
{
    unique_lock< mutex > lock(this->daemon_mutex);
    if (this->daemon != NULL) {
        MHD_stop_daemon (daemon);
        this->daemon = NULL;
    }
    this->daemon_cv.notify_all();
}

void HTTPD_microhttpd::join()
{
    unique_lock< mutex > lock(this->daemon_mutex);
    while (true) {
        if (this->daemon == NULL) {
            return;
        }
        this->daemon_cv.wait(lock);
    }
}

HTTPD_microhttpd::~HTTPD_microhttpd()
{
    this->stop();
}

// Accept only connection coming from localhost and IPv4
int HTTPD_microhttpd::accept_wrapper(void *cls, const sockaddr *addr, socklen_t addrlen)
{
    (void) cls;
    (void) addrlen;

    if (addr->sa_family != AF_INET) {
        return MHD_NO;
    }
    auto inetaddr = reinterpret_cast< const sockaddr_in* >(addr);
    struct in_addr localhost;
    int ret = inet_pton(AF_INET, "127.0.0.1", &localhost);
    assert(ret == 1);
    if (memcmp(&inetaddr->sin_addr, &localhost, sizeof(localhost)) == 0) {
        return MHD_YES;
    }
    return MHD_NO;
}

// If it is not null, delete the HTTPCallback object
void HTTPD_microhttpd::cleanup_wrapper(void *cls, MHD_Connection *connection, void **con_cls, MHD_RequestTerminationCode toe)
{
    auto object = reinterpret_cast< HTTPD_microhttpd* >(cls);
    (void) object;
    (void) connection;
    (void) toe;

    if (*con_cls != NULL) {
        auto cb = reinterpret_cast< HTTPCallback_microhttpd* >(*con_cls);
        delete cb;
        *con_cls = NULL;
    }
}

// Pass the request to the target
int HTTPD_microhttpd::answer_wrapper(void *cls, MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{
    auto object = reinterpret_cast< HTTPD_microhttpd* >(cls);
    HTTPCallback_microhttpd *cb;
    if (*con_cls == NULL) {
        cb = new HTTPCallback_microhttpd();
        *con_cls = cb;
    } else {
        cb = reinterpret_cast< HTTPCallback_microhttpd* >(*con_cls);
    }
    string surl(url);
    string smethod(method);
    string sversion(version);
    cerr << "Request: " << sversion << ", " << smethod << ": " << surl << endl;
    string content = object->target.answer(*cb, surl, smethod, sversion);

    struct MHD_Response *response;
    int ret;
    response = MHD_create_response_from_buffer (content.size(), (void*) content.c_str(), MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header (response, "Content-Type", "application/json");
    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);
    return ret;
}

HTTPD::~HTTPD()
{
}

HTTPCallback::~HTTPCallback()
{
}

HTTPCallback_microhttpd::HTTPCallback_microhttpd()
{
}
