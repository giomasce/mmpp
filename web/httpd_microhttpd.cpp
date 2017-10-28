#include "httpd_microhttpd.h"

#include <cstring>
#include <sstream>
#include <iostream>
#include <deque>

#include <cassert>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils/utils.h"

using namespace std;

HTTPD_microhttpd::HTTPD_microhttpd(int port, HTTPTarget &target, bool only_from_localhost) :
    port(port), daemon(NULL), target(target), restrict_to_localhost(only_from_localhost)
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

bool HTTPD_microhttpd::is_running()
{
    return this->daemon != NULL;
}

HTTPD_microhttpd::~HTTPD_microhttpd()
{
    this->stop();
}

int HTTPD_microhttpd::accept_wrapper(void *cls, const sockaddr *addr, socklen_t addrlen)
{
    auto object = reinterpret_cast< HTTPD_microhttpd* >(cls);
    (void) addrlen;

    // Accept only IPv4
    if (addr->sa_family != AF_INET) {
        return MHD_NO;
    }

    // If configured so, accept only from localhost
    if (object->restrict_to_localhost) {
        auto inetaddr = reinterpret_cast< const sockaddr_in* >(addr);
        struct in_addr localhost;
        int ret = inet_pton(AF_INET, "127.0.0.1", &localhost);
        assert(ret == 1);
        if (memcmp(&inetaddr->sin_addr, &localhost, sizeof(localhost)) == 0) {
            return MHD_YES;
        }
        return MHD_NO;
    } else {
        return MHD_YES;
    }
}

// Delete the HTTPCallback object
void HTTPD_microhttpd::cleanup_wrapper(void *cls, MHD_Connection *connection, void **con_cls, MHD_RequestTerminationCode toe)
{
    auto object = reinterpret_cast< HTTPD_microhttpd* >(cls);
    (void) object;
    (void) connection;
    (void) toe;

    auto cb = reinterpret_cast< HTTPCallback_microhttpd* >(*con_cls);
    delete cb;
    *con_cls = NULL;
}

class microhttpd_reader {
public:
    microhttpd_reader(HTTPAnswerer &answerer) : pos(0), total_pos(0), answerer(answerer) {
    }

    size_t callback(uint64_t pos, char* buf, size_t max) {
        assert(pos == this->total_pos);
        size_t bytes_avail = this->buffer.size() - this->pos;
        if (bytes_avail == 0) {
            this->buffer = this->answerer();
            if (this->buffer.empty()) {
                return MHD_CONTENT_READER_END_OF_STREAM;
            }
            this->pos = 0;
            bytes_avail = this->buffer.size();
        }
        size_t bytes_num = min(max, bytes_avail);
        memcpy(buf, this->buffer.c_str() + this->pos, bytes_num);
        this->pos += bytes_num;
        this->total_pos += bytes_num;
        return bytes_num;
    }

private:
    string buffer;
    size_t pos;
    size_t total_pos;
    HTTPAnswerer &answerer;

};

long int reader_callback(void *cls, uint64_t pos, char* buf, size_t max) {
    microhttpd_reader *reader = reinterpret_cast< microhttpd_reader* >(cls);
    return reader->callback(pos, buf, max);
}

void free_reader_callback_data(void *cls) {
    microhttpd_reader *reader = reinterpret_cast< microhttpd_reader* >(cls);
    delete reader;
}

// Pass the request to the target
int HTTPD_microhttpd::answer_wrapper(void *cls, MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{
    (void) upload_data;
    (void) upload_data_size;

    auto object = reinterpret_cast< HTTPD_microhttpd* >(cls);
    HTTPCallback_microhttpd *cb;
    if (*con_cls == NULL) {
        cb = new HTTPCallback_microhttpd(connection);
        *con_cls = cb;
    } else {
        cb = reinterpret_cast< HTTPCallback_microhttpd* >(*con_cls);
    }
    cb->set_url(url);
    cb->set_method(method);
    cb->set_version(version);
    acout() << "Request received: " << method << " " << url << " " << version << endl;
    object->target.answer(*cb);
    acout() << "Responding " << cb->get_status_code() << " (" << method << " " << url << " " << version << ")" << endl;

    struct MHD_Response *response;
    int ret;

    /*HTTPAnswerer &answerer = cb->get_answerer();
    ostringstream content_buf;
    while (true) {
        string new_content = answerer();
        if (new_content.empty()) {
            break;
        } else {
            content_buf << new_content;
        }
    }
    string content = content_buf.str();
    response = MHD_create_response_from_buffer (content.size(), (void*) content.c_str(), MHD_RESPMEM_MUST_COPY);*/

    response = MHD_create_response_from_callback(cb->get_size(), HTTP_BUFFER_SIZE, reader_callback, new microhttpd_reader(cb->get_answerer()), free_reader_callback_data);

    for (auto header : cb->get_headers()) {
        MHD_add_response_header (response, header.first.c_str(), header.second.c_str());
    }
    ret = MHD_queue_response (connection, cb->get_status_code(), response);
    // This just decrements the reference counter
    MHD_destroy_response (response);
    return ret;
}

HTTPD::~HTTPD()
{
}

HTTPCallback::~HTTPCallback()
{
}

HTTPCallback_microhttpd::HTTPCallback_microhttpd(MHD_Connection *connection) :
    connection(connection), status_code(200), req_headers_extracted(false), cookies_extracted(false),
    answerer([]() { return ""; }), size(-1)
{
}

void HTTPCallback_microhttpd::set_status_code(unsigned int status_code)
{
    this->status_code = status_code;
}

void HTTPCallback_microhttpd::add_header(string header, string content)
{
    this->headers.push_back(make_pair(header, content));
}

void HTTPCallback_microhttpd::set_answer(string &&answer)
{
    this->answerer = [answer,printed=false]() mutable ->string {
        if (!printed) {
            printed = true;
            return answer;
        } else {
            return "";
        }
    };
}

void HTTPCallback_microhttpd::set_size(uint64_t size)
{
    this->size = size;
}

const string &HTTPCallback_microhttpd::get_url()
{
    return this->url;
}

const string &HTTPCallback_microhttpd::get_method()
{
    return this->method;
}

const string &HTTPCallback_microhttpd::get_version()
{
    return this->version;
};

void HTTPCallback_microhttpd::set_answerer(HTTPAnswerer &&answerer)
{
    this->answerer = answerer;
}

const std::unordered_map<string, string> &HTTPCallback_microhttpd::get_request_headers()
{
    if (!this->req_headers_extracted) {
        this->req_headers_extracted = true;
        MHD_get_connection_values(this->connection, MHD_HEADER_KIND, this->iterator_wrapper, &this->req_headers);
    }
    return this->req_headers;
}

const std::unordered_map<string, string> &HTTPCallback_microhttpd::get_cookies()
{
    if (!this->cookies_extracted) {
        this->cookies_extracted = true;
        MHD_get_connection_values(this->connection, MHD_COOKIE_KIND, this->iterator_wrapper, &this->cookies);
    }
    return this->cookies;
}

unsigned int HTTPCallback_microhttpd::get_status_code() const
{
    return this->status_code;
}

const std::vector<std::pair<string, string> > &HTTPCallback_microhttpd::get_headers() const
{
    return this->headers;
}

uint64_t HTTPCallback_microhttpd::get_size()
{
    return this->size;
}

void HTTPCallback_microhttpd::set_url(const string &url)
{
    this->url = url;
}

void HTTPCallback_microhttpd::set_method(const string &method)
{
    this->method = method;
}

void HTTPCallback_microhttpd::set_version(const string &version)
{
    this->version = version;
}

HTTPAnswerer &HTTPCallback_microhttpd::get_answerer()
{
    return this->answerer;
}

int HTTPCallback_microhttpd::iterator_wrapper(void *cls, MHD_ValueKind kind, const char *key, const char *value)
{
    (void) kind;

    auto map = reinterpret_cast< std::unordered_map< std::string, std::string >* >(cls);
    (*map)[string(key)] = string(value);
    return MHD_YES;
}

HTTPTarget::~HTTPTarget()
{
}