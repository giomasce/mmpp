#ifndef HTTPD_H
#define HTTPD_H

#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <unordered_map>

#include <microhttpd.h>

class HTTPD
{
public:
    virtual ~HTTPD();
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void join() = 0;
    virtual bool is_running() = 0;
};

class HTTPCallback {
public:
    virtual ~HTTPCallback();
    virtual void set_status_code(unsigned int status_code) = 0;
    virtual void add_header(std::string header, std::string content) = 0;
    virtual const std::unordered_map< std::string, std::string > &get_request_headers() = 0;
    virtual const std::unordered_map< std::string, std::string > &get_cookies() = 0;
};

class HTTPTarget {
public:
    virtual ~HTTPTarget();
    virtual std::string answer(HTTPCallback &cb, std::string url, std::string method, std::string version) = 0;
};

class HTTPD_microhttpd : public HTTPD {
public:
    HTTPD_microhttpd(int port, HTTPTarget &target, bool only_from_localhost);
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
    bool only_from_localhost;
};

class HTTPCallback_microhttpd : public HTTPCallback {
public:
    HTTPCallback_microhttpd(MHD_Connection *connection);
    void set_status_code(unsigned int status_code);
    void add_header(std::string header, std::string content);
    const std::unordered_map< std::string, std::string > &get_request_headers();
    const std::unordered_map< std::string, std::string > &get_cookies();

    unsigned int get_status_code() const;
    const std::vector< std::pair< std::string, std::string > > &get_headers() const;
private:
    static int iterator_wrapper(void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
    MHD_Connection *connection;
    unsigned int status_code;
    std::vector< std::pair< std::string, std::string > > headers;
    bool req_headers_extracted, cookies_extracted;
    std::unordered_map< std::string, std::string > req_headers, cookies;
};

#endif // HTTPD_H
