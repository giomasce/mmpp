#ifndef HTTPD_MICROHTTPD_H
#define HTTPD_MICROHTTPD_H

#include "web/httpd.h"

#include <mutex>
#include <condition_variable>
#include <atomic>

#include <microhttpd.h>

class HTTPD_microhttpd : public HTTPD {
public:
    HTTPD_microhttpd(int port, HTTPTarget &target, bool restrict_to_localhost);
    void start();
    void stop();
    void join();
    bool is_running();
    ~HTTPD_microhttpd();

private:
    static int accept_wrapper(void *cls, const struct sockaddr *addr, socklen_t addrlen);
    static void cleanup_wrapper(void *cls, struct MHD_Connection *connection,
                               void **con_cls, enum MHD_RequestTerminationCode toe);
    static int answer_wrapper(void *cls, struct MHD_Connection *connection,
               const char *url,
               const char *method, const char *version,
               const char *upload_data,
               size_t *upload_data_size, void **con_cls);

    int port;
    std::mutex daemon_mutex;
    std::condition_variable daemon_cv;
    std::atomic< MHD_Daemon* > daemon;
    HTTPTarget &target;
    bool restrict_to_localhost;
};

class HTTPCallback_microhttpd : public HTTPCallback {
public:
    HTTPCallback_microhttpd(MHD_Connection *connection);
    ~HTTPCallback_microhttpd();
    void set_status_code(unsigned int status_code);
    void add_header(std::string header, std::string content);
    void set_post_iterator(std::unique_ptr< HTTPPostIterator > &&post_iterator);
    void set_answerer(std::unique_ptr< HTTPAnswerer > &&answerer);
    void set_answer(std::string &&answer);
    void set_size(uint64_t size);

    const std::string &get_url();
    const std::string &get_method();
    const std::string &get_version();
    const std::unordered_map< std::string, std::string > &get_request_headers();
    const std::unordered_map< std::string, std::string > &get_cookies();

    unsigned int get_status_code() const;
    const std::vector< std::pair< std::string, std::string > > &get_headers() const;
    HTTPAnswerer &get_answerer();
    HTTPPostIterator &get_post_iterator();
    uint64_t get_size();
    bool get_send_response() const;
    void set_url(const std::string &url);
    void set_method(const std::string &method);
    void set_version(const std::string &version);
    void process_post_data(const char *data, size_t size);
    void close_post_processor();

private:
    static int iterator_wrapper(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
    static int post_data_iterator(void *cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size);
    MHD_Connection *connection;
    MHD_PostProcessor *post_processor;
    unsigned int status_code;
    std::vector< std::pair< std::string, std::string > > headers;
    bool req_headers_extracted, cookies_extracted;
    std::unordered_map< std::string, std::string > req_headers, cookies;
    std::unique_ptr< HTTPAnswerer > answerer;
    std::unique_ptr< HTTPPostIterator > post_iterator;
    uint64_t size;
    std::string url;
    std::string method;
    std::string version;
    bool send_response;
};

#endif // HTTPD_MICROHTTPD_H
