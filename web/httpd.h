#ifndef HTTPD_H
#define HTTPD_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

const size_t HTTP_BUFFER_SIZE = 1024;

class HTTPD
{
public:
    virtual ~HTTPD();
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void join() = 0;
    virtual bool is_running() = 0;
};

typedef std::function< std::string() > HTTPAnswerer;

class HTTPCallback {
public:
    virtual ~HTTPCallback();
    virtual void set_status_code(unsigned int status_code) = 0;
    virtual void add_header(std::string header, std::string content) = 0;
    virtual void set_answerer(HTTPAnswerer &&answerer) = 0;
    virtual void set_answer(std::string &&answer) = 0;
    virtual void set_size(uint64_t size) = 0;

    virtual const std::string &get_url() = 0;
    virtual const std::string &get_method() = 0;
    virtual const std::string &get_version() = 0;
    virtual const std::unordered_map< std::string, std::string > &get_request_headers() = 0;
    virtual const std::unordered_map< std::string, std::string > &get_cookies() = 0;
};

class HTTPTarget {
public:
    virtual ~HTTPTarget();
    virtual void answer(HTTPCallback &cb) = 0;
};

#endif // HTTPD_H
