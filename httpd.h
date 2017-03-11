#ifndef HTTPD_H
#define HTTPD_H

#include <string>
#include <mutex>
#include <condition_variable>

#include <microhttpd.h>

class HTTPD
{
public:
    virtual ~HTTPD();
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void join() = 0;
};

class HTTPCallback {
public:
    virtual ~HTTPCallback();
};

class HTTPTarget {
public:
    virtual std::string answer(HTTPCallback &cb, std::string url, std::string method, std::string version) = 0;
};

class HTTPD_microhttpd : public HTTPD {
public:
    HTTPD_microhttpd(int port, HTTPTarget &target);
    void start();
    void stop();
    void join();
    ~HTTPD_microhttpd();
protected:
    static int accept_wrapper(void *cls, const struct sockaddr *addr, socklen_t addrlen);
    static void cleanup_wrapper(void *cls, struct MHD_Connection *connection,
                               void **con_cls, enum MHD_RequestTerminationCode toe);
    static int answer_wrapper(void *cls, struct MHD_Connection *connection,
               const char *url,
               const char *method, const char *version,
               const char *upload_data,
               size_t *upload_data_size, void **con_cls);
private:
    int port;
    std::mutex daemon_mutex;
    std::condition_variable daemon_cv;
    MHD_Daemon *daemon;
    HTTPTarget &target;
};

class HTTPCallback_microhttpd : public HTTPCallback {
public:
    HTTPCallback_microhttpd();
};

#endif // HTTPD_H
