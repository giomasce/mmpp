#include "httpd.h"

#include <cstring>
#include <sstream>
#include <iostream>

using namespace std;

HTTPD_microhttpd::HTTPD_microhttpd(int port, HTTPTarget &endpoint) :
    port(port), daemon(NULL), endpoint(endpoint)
{
}

void HTTPD_microhttpd::start()
{
    this->daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, this->port, NULL, NULL, &this->answer_wrapper, this, MHD_OPTION_END);
    if (this->daemon == NULL) {
        throw string("Could not start daemon");
    }
}

void HTTPD_microhttpd::stop()
{
    if (this->daemon != NULL) {
        MHD_stop_daemon (daemon);
        this->daemon = NULL;
    }
}

HTTPD_microhttpd::~HTTPD_microhttpd()
{
    this->stop();
}

int HTTPD_microhttpd::answer_wrapper(void *cls, MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{
    auto object = reinterpret_cast< HTTPD_microhttpd* >(cls);
    return object->answer(connection, url, method, version, upload_data, upload_data_size, con_cls);
}

int HTTPD_microhttpd::answer(MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{
    string content = this->endpoint.answer();
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
