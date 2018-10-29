#pragma once

#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <ostream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <functional>
#include <iomanip>
#include <chrono>
#include <unordered_map>
#include <map>
#include <mutex>
#include <memory>
#include <cstdint>

#include <boost/filesystem.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/current_function.hpp>

#include "platform.h"

const size_t DEFAULT_STACK_SIZE = 8*1024*1024;

extern bool mmpp_abort;

std::string size_to_string(uint64_t size);

struct Tic {
    std::chrono::steady_clock::time_point begin;
};

Tic tic();
void toc(const Tic &t, int reps);

class Hasher {
public:
    virtual void update(const char* s, std::size_t n) = 0;
    virtual std::string get_digest() = 0;
};

class HashSink {
public:
    typedef char char_type;
    typedef boost::iostreams::sink_tag category;

    HashSink();
    std::streamsize write(const char *s, std::streamsize n);
    std::string get_digest();

private:
    std::shared_ptr< Hasher > hasher;
};

template< typename T >
std::string hash_object(const T &obj) {
    HashSink hasher;
    // g++ can automatically deduce the template parameter here, but clang++ cannot
    boost::iostreams::stream< HashSink > fout(hasher);
    {
        boost::archive::text_oarchive archive(fout);
        archive << obj;
    }
    fout.flush();
    return hasher.get_digest();
}

// Taken from https://stackoverflow.com/a/45046349/807307 and adapted
static std::mutex mtx_cout;
struct acout
{
        std::unique_lock<std::mutex> lk;
        acout()
            :
              lk(std::unique_lock<std::mutex>(mtx_cout))
        {

        }

        template<typename T>
        acout& operator<<(const T& _t)
        {
            std::cout << _t;
            return *this;
        }

        acout& operator<<(std::ostream& (*fp)(std::ostream&))
        {
            std::cout << fp;
            return *this;
        }
};

class Reportable {
public:
    virtual void report(double current) = 0;
    virtual void set_total(double total) = 0;
    virtual ~Reportable();
};

class TextProgressBar : public Reportable {
public:
    TextProgressBar(size_t length=100, double total=0.0);
    void set_total(double total);
    void report(double current, bool force);
    void report(double current);
    void finished();

private:
    std::chrono::steady_clock::time_point last_report;
    size_t last_len;
    double total;
    size_t length;
};

class Finally {
public:
    Finally(std::function< void() > &&finally) : finally(std::move(finally)) {}
    ~Finally() {
        this->finally();
    }

private:
    std::function< void() > finally;
};

extern std::ostream cnull;

template< typename T >
using Generator = coroutine_pull< T >;

void default_exception_handler(std::exception_ptr ptr);

template<typename T>
struct istream_begin_end {
    istream_begin_end(std::istream &s) : s(s) {}

    decltype(auto) begin() const {
        return std::istream_iterator<T>(s);
    }

    decltype(auto) end() const {
        return std::istream_iterator<T>();
    }

    std::istream &s;
};
