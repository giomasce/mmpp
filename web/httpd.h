#ifndef HTTPD_H
#define HTTPD_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

const size_t HTTP_BUFFER_SIZE = 65536;

class HTTPD
{
public:
    virtual ~HTTPD() {}
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void join() = 0;
    virtual bool is_running() = 0;
};

//typedef std::function< std::string() > HTTPAnswerer;

class HTTPAnswerer {
public:
    virtual ~HTTPAnswerer() {}
    virtual std::string answer() = 0;
};

class HTTPStringAnswerer : public HTTPAnswerer {
public:
    HTTPStringAnswerer() : stored_answer(""), answered(true) {
    }

    HTTPStringAnswerer(std::string &answer) : stored_answer(std::move(answer)), answered(false) {
    }

    std::string answer() {
        if (this->answered) {
            return "";
        } else {
            // We return the stored answer and also discard it to reclaim memory
            std::string ret;
            ret.swap(this->stored_answer);
            this->answered = true;
            return ret;
        }
    }

private:
    std::string stored_answer;
    bool answered;
};

class HTTPFileAnswerer : public HTTPAnswerer {
public:
    HTTPFileAnswerer(std::shared_ptr< std::istream > infile) : infile(infile) {
    }

    std::string answer() {
        if (*this->infile) {
            char buffer[HTTP_BUFFER_SIZE];
            infile->read(buffer, HTTP_BUFFER_SIZE);
            auto bytes_num = infile->gcount();
            return std::string(buffer, bytes_num);
        } else {
            return std::string("");
        }
    }

private:
    std::shared_ptr< std::istream > infile;
};

class HTTPPostIterator {
public:
    virtual ~HTTPPostIterator() {}
    virtual void receive(const std::string &key, const std::string &filename, const std::string &content_type, const std::string &transfer_encoding, const std::string &data, uint64_t offset) = 0;
    virtual void finish() = 0;
};

struct PostItem {
    std::string value;
    std::string filename;
    std::string content_type;
    std::string transfer_encoding;
};

class HTTPSimplePostIterator : public HTTPPostIterator {
public:
    HTTPSimplePostIterator(const std::function< void(const std::unordered_map< std::string, PostItem >&) > &callback, size_t max_post_size = 50*1024*1024) : total_size(0), max_post_size(max_post_size), callback(callback) {
    }

    void receive(const std::string &key, const std::string &filename, const std::string &content_type, const std::string &transfer_encoding, const std::string &data, uint64_t offset) {
        if (this->total_size > this->max_post_size) {
            // FIXME Do something more sensible if we receive too much POST data
            return;
        }
        PostItem &pi = this->items[key];
        // FIXME This breaks if parameters are repeated
        assert(offset == pi.value.size());
        if (offset == 0) {
            pi.filename = filename;
            pi.content_type = content_type;
            pi.transfer_encoding = transfer_encoding;
        }
        pi.value += data;
        this->total_size += data.size();
    }

    void finish() {
        this->callback(this->items);
    }

private:
    size_t total_size;
    size_t max_post_size;
    std::unordered_map< std::string, PostItem > items;
    std::function< void(const std::unordered_map< std::string, PostItem >&) > callback;
};

class HTTPCallback {
public:
    virtual ~HTTPCallback() {}
    virtual void set_status_code(unsigned int status_code) = 0;
    virtual void add_header(std::string header, std::string content) = 0;
    virtual void set_post_iterator(std::unique_ptr< HTTPPostIterator > &&post_iterator) = 0;
    virtual void set_answerer(std::unique_ptr< HTTPAnswerer > &&answerer) = 0;
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
    virtual ~HTTPTarget() {}
    virtual void answer(HTTPCallback &cb) = 0;
};

#endif // HTTPD_H
