#ifndef HTTPD_H
#define HTTPD_H

#include <string>

#include <microhttpd.h>

class HTTPD
{
public:
    virtual ~HTTPD();
    virtual void start() = 0;
    virtual void stop() = 0;
};

class HTTPTarget {
public:
    virtual std::string answer() = 0;
};

class HTTPD_microhttpd : public HTTPD {
public:
    HTTPD_microhttpd(int port, HTTPTarget &endpoint);
    void start();
    void stop();
    ~HTTPD_microhttpd();
protected:
    static int answer_wrapper(void *cls, struct MHD_Connection *connection,
               const char *url,
               const char *method, const char *version,
               const char *upload_data,
               size_t *upload_data_size, void **con_cls);
    int answer(struct MHD_Connection *connection,
               const char *url,
               const char *method, const char *version,
               const char *upload_data,
               size_t *upload_data_size, void **con_cls);
private:
    int port;
    MHD_Daemon *daemon;
    HTTPTarget &endpoint;
};

#endif // HTTPD_H
