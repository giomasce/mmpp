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
#include <mutex>
#include <memory>

#include <boost/filesystem.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/concepts.hpp>

#define EXCEPTIONS_SELF_DEBUG

#if defined(__GNUG__) && defined(EXCEPTIONS_SELF_DEBUG)
std::vector< std::string > dump_stacktrace(size_t depth);
std::vector< std::string > dump_stacktrace();
#else
inline static std::vector< std::string > dump_stacktrace(int depth=0) {
    (void) depth;
    return {};
}
#endif

extern bool mmpp_abort;

class MMPPException {
public:
    MMPPException(const std::string &reason="");
    const std::string &get_reason() const;
    const std::vector< std::string > &get_stacktrace() const;
    void print_stacktrace(std::ostream &st) const;

private:
    std::string reason;
    std::vector< std::string > stacktrace;
};

template< typename Exception, typename... Args >
inline static void assert_or_throw(bool cond, const Args&... args) {
    if (!cond) {
        throw Exception(args...);
    }
}

std::string size_to_string(size_t size);
bool starts_with(std::string a, std::string b);

struct Tic {
    std::chrono::steady_clock::time_point begin;
};

Tic tic();
void toc(const Tic &t, int reps);

std::unordered_map< std::string, std::function< int(int, char*[]) > > &get_main_functions();
void register_main_function(const std::string &name, const std::function< int(int, char*[]) > &main_function);

// static_block implementation from https://stackoverflow.com/a/34321324/807307
#define CONCATENATE(s1, s2) s1##s2
#define EXPAND_THEN_CONCATENATE(s1, s2) CONCATENATE(s1, s2)
#ifdef __COUNTER__
#define UNIQUE_IDENTIFIER(prefix) EXPAND_THEN_CONCATENATE(prefix, __COUNTER__)
#else
#define UNIQUE_IDENTIFIER(prefix) EXPAND_THEN_CONCATENATE(prefix, __LINE__)
#endif /* COUNTER */
#define static_block STATIC_BLOCK_IMPL1(UNIQUE_IDENTIFIER(_static_block_))
#define STATIC_BLOCK_IMPL1(prefix) \
    STATIC_BLOCK_IMPL2(CONCATENATE(prefix,_fn),CONCATENATE(prefix,_var))
#define STATIC_BLOCK_IMPL2(function_name,var_name) \
static void function_name(); \
static int var_name __attribute((unused)) = (function_name(), 0) ; \
static void function_name()

class Hasher {
public:
    virtual void update(const char* s, std::streamsize n) = 0;
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
    boost::iostreams::stream fout(hasher);
    {
        boost::archive::text_oarchive archive(fout);
        archive << obj;
    }
    fout.flush();
    return hasher.get_digest();
}

// Funny trick from https://stackoverflow.com/a/41485014/807307
template<typename S>
struct enable_make : public S
{
    template<typename... T>
    enable_make(T&&... t)
        : S(std::forward<T>(t)...)
    {
    }
};

template< typename T >
struct enable_create : public std::enable_shared_from_this< T > {
    template< typename... Args >
    static std::shared_ptr< T > create(Args&&... args) {
        std::shared_ptr< enable_make< T > > pointer = std::make_shared< enable_make< T > >(std::forward< Args >(args)...);
        static_cast< std::shared_ptr< enable_create< T > > >(pointer)->init();
        return pointer;
    }
protected:
    virtual void init() {}
};

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

template< class T >
class SafeWeakPtr : public std::weak_ptr< T > {
public:
    constexpr SafeWeakPtr() noexcept : std::weak_ptr< T >() {
    }

    template< class U >
    SafeWeakPtr(const std::shared_ptr< U > &r) noexcept : std::weak_ptr< T >(r) {
    }

    std::shared_ptr< T > lock() const noexcept {
        auto strong = this->std::weak_ptr<T>::lock();
        assert(strong);
        return strong;
    }
};

template< class It, class URBG >
It random_choose(It first, It last, URBG &&g) {
    return first + std::uniform_int_distribution< size_t >(0, (last - first) - 1)(g);
}

template< typename T >
typename std::vector< T >::reference enlarge_and_set(std::vector< T > &v, typename std::vector< T >::size_type pos) {
    v.resize(std::max(v.size(), pos+1));
    return v[pos];
}

extern std::ostream cnull;
