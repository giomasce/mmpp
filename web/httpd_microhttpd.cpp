#include "httpd_microhttpd.h"

#include <cstring>
#include <sstream>
#include <iostream>
#include <deque>

#include <cassert>

#if (!defined(_WIN32))
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "utils/utils.h"

//#define LOG_MICROHTTPD_REQUESTS

static void microhttpd_panic(void *cls, const char *file, unsigned int line, const char *reason) {
    (void) cls;
    std::cout << "Microhttpd panic in " << file << ":" << line << " beacuse of \"" << reason << "\"" << std::endl;
    abort();
}

HTTPD_microhttpd::HTTPD_microhttpd(int port, HTTPTarget &target, bool only_from_localhost) :
    port(port), daemon(nullptr), target(target), restrict_to_localhost(only_from_localhost)
{
    MHD_set_panic_func(microhttpd_panic, nullptr);
}

void HTTPD_microhttpd::start()
{
    std::unique_lock< std::mutex > lock(this->daemon_mutex);
    this->daemon = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_POLL, this->port,
                                     &this->accept_wrapper, this,
                                     &this->answer_wrapper, this,
                                     MHD_OPTION_THREAD_STACK_SIZE, DEFAULT_STACK_SIZE,
                                     MHD_OPTION_NOTIFY_COMPLETED, &this->cleanup_wrapper, nullptr,
                                     MHD_OPTION_END);
    if (this->daemon == nullptr) {
        throw std::string("Could not start httpd daemon");
    }
    this->daemon_cv.notify_all();
}

void HTTPD_microhttpd::stop()
{
    std::unique_lock< std::mutex > lock(this->daemon_mutex);
    if (this->daemon != nullptr) {
        MHD_stop_daemon (daemon);
        this->daemon = nullptr;
    }
    this->daemon_cv.notify_all();
}

void HTTPD_microhttpd::join()
{
    std::unique_lock< std::mutex > lock(this->daemon_mutex);
    while (true) {
        if (this->daemon == nullptr) {
            return;
        }
        this->daemon_cv.wait(lock);
    }
}

bool HTTPD_microhttpd::is_running()
{
    return this->daemon != nullptr;
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
#ifdef NDEBUG
        (void) ret;
#endif
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
    *con_cls = nullptr;
}

class microhttpd_reader {
public:
    microhttpd_reader(HTTPAnswerer &answerer) : pos(0), total_pos(0), answerer(answerer) {
    }

    size_t callback(uint64_t pos, char* buf, size_t max) {
        assert(pos == this->total_pos);
#ifdef NDEBUG
        (void) pos;
#endif
        size_t bytes_avail = this->buffer.size() - this->pos;
        if (bytes_avail == 0) {
            this->buffer = this->answerer.answer();
            if (this->buffer.empty()) {
                return MHD_CONTENT_READER_END_OF_STREAM;
            }
            this->pos = 0;
            bytes_avail = this->buffer.size();
        }
        size_t bytes_num = (std::min)(max, bytes_avail);
        memcpy(buf, this->buffer.c_str() + this->pos, bytes_num);
        this->pos += bytes_num;
        this->total_pos += bytes_num;
        return bytes_num;
    }

private:
    std::string buffer;
    size_t pos;
    size_t total_pos;
    HTTPAnswerer &answerer;

};

ssize_t reader_callback(void *cls, uint64_t pos, char* buf, size_t max) {
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
    if (*con_cls == nullptr) {
        cb = new HTTPCallback_microhttpd(connection);
        *con_cls = cb;
        cb->set_url(url);
        cb->set_method(method);
        cb->set_version(version);
#ifdef LOG_MICROHTTPD_REQUESTS
        acout() << "Request received: " << method << " " << url << " " << version << endl;
#endif
        object->target.answer(*cb);
        if (!cb->get_send_response()) {
            return MHD_YES;
        }
    } else {
        cb = reinterpret_cast< HTTPCallback_microhttpd* >(*con_cls);
        if (*upload_data_size != 0) {
            cb->process_post_data(upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        } else {
            cb->close_post_processor();
            cb->get_post_iterator().finish();
        }
    }

#ifdef LOG_MICROHTTPD_REQUESTS
    acout() << "Responding " << cb->get_status_code() << " (" << method << " " << url << " " << version << ")" << endl;
#endif

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

HTTPCallback_microhttpd::HTTPCallback_microhttpd(MHD_Connection *connection) :
    connection(connection), post_processor(nullptr), status_code(200), req_headers_extracted(false), cookies_extracted(false),
    answerer(std::make_unique< HTTPStringAnswerer >()), size(-1), send_response(false)
{
}

HTTPCallback_microhttpd::~HTTPCallback_microhttpd()
{
    this->close_post_processor();
}

void HTTPCallback_microhttpd::set_status_code(unsigned int status_code)
{
    this->status_code = status_code;
    this->send_response = true;
}

void HTTPCallback_microhttpd::add_header(std::string header, std::string content)
{
    this->headers.push_back(std::make_pair(header, content));
}

void HTTPCallback_microhttpd::set_post_iterator(std::unique_ptr<HTTPPostIterator> &&post_iterator)
{
    this->post_iterator = std::move(post_iterator);
}

void HTTPCallback_microhttpd::set_answerer(std::unique_ptr< HTTPAnswerer > &&answerer)
{
    this->answerer = std::move(answerer);
}

void HTTPCallback_microhttpd::set_answer(std::string &&answer)
{
    this->answerer = std::make_unique< HTTPStringAnswerer >(answer);
}

void HTTPCallback_microhttpd::set_size(uint64_t size)
{
    this->size = size;
}

const std::string &HTTPCallback_microhttpd::get_url()
{
    return this->url;
}

const std::string &HTTPCallback_microhttpd::get_method()
{
    return this->method;
}

const std::string &HTTPCallback_microhttpd::get_version()
{
    return this->version;
};

const std::unordered_map<std::string, std::string> &HTTPCallback_microhttpd::get_request_headers()
{
    if (!this->req_headers_extracted) {
        this->req_headers_extracted = true;
        MHD_get_connection_values(this->connection, MHD_HEADER_KIND, this->iterator_wrapper, &this->req_headers);
    }
    return this->req_headers;
}

const std::unordered_map<std::string, std::string> &HTTPCallback_microhttpd::get_cookies()
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

const std::vector<std::pair<std::string, std::string> > &HTTPCallback_microhttpd::get_headers() const
{
    return this->headers;
}

uint64_t HTTPCallback_microhttpd::get_size()
{
    return this->size;
}

bool HTTPCallback_microhttpd::get_send_response() const
{
    return this->send_response;
}

void HTTPCallback_microhttpd::set_url(const std::string &url)
{
    this->url = url;
}

void HTTPCallback_microhttpd::set_method(const std::string &method)
{
    this->method = method;
}

void HTTPCallback_microhttpd::set_version(const std::string &version)
{
    this->version = version;
}

void HTTPCallback_microhttpd::process_post_data(const char *data, size_t size)
{
    if (this->post_processor == nullptr) {
        this->post_processor = MHD_create_post_processor(this->connection, HTTP_BUFFER_SIZE, HTTPCallback_microhttpd::post_data_iterator, this);
    }
    MHD_post_process(this->post_processor, data, size);
}

void HTTPCallback_microhttpd::close_post_processor()
{
    if (this->post_processor != nullptr) {
        MHD_destroy_post_processor(this->post_processor);
        this->post_processor = nullptr;
    }
}

HTTPAnswerer &HTTPCallback_microhttpd::get_answerer()
{
    return *this->answerer;
}

HTTPPostIterator &HTTPCallback_microhttpd::get_post_iterator()
{
    return *this->post_iterator;
}

int HTTPCallback_microhttpd::iterator_wrapper(void *cls, MHD_ValueKind kind, const char *key, const char *value)
{
    (void) kind;

    auto map = reinterpret_cast< std::unordered_map< std::string, std::string >* >(cls);
    (*map)[std::string(key)] = std::string(value);
    return MHD_YES;
}

int HTTPCallback_microhttpd::post_data_iterator(void *cls, MHD_ValueKind kind, const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size)
{
    (void) kind;

    auto self = reinterpret_cast< HTTPCallback_microhttpd* >(cls);
    std::string skey(key);
    std::string sfilename;
    if (filename != nullptr) {
        sfilename = filename;
    }
    std::string scontent_type;
    if (content_type != nullptr) {
        scontent_type = content_type;
    }
    std::string stransfer_encoding;
    if (transfer_encoding != nullptr) {
        stransfer_encoding = transfer_encoding;
    }
    std::string sdata(data, size);
    self->get_post_iterator().receive(skey, sfilename, scontent_type, stransfer_encoding, sdata, off);
    return MHD_YES;
}


