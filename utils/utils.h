#ifndef UTILS_H
#define UTILS_H

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

#include <boost/filesystem.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/concepts.hpp>

#include <cryptopp/sha.h>

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

inline static bool is_ascii(char c) {
    return c > 32 && c < 127;
}

inline static bool is_dollar(char c) {
    return c == '$';
}

inline static bool is_mm_valid(char c) {
    return is_ascii(c) && !is_dollar(c);
}

inline static bool is_mm_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f';
}

inline static bool is_label_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
}

inline static bool is_valid_label(std::string s) {
    for (auto c : s) {
        if (!is_label_char(c)) {
            return false;
        }
    }
    return true;
}

inline static bool is_valid_symbol(std::string s) {
    for (auto c : s) {
        if (!is_mm_valid(c)) {
            return false;
        }
    }
    return true;
}

// Taken from http://stackoverflow.com/a/217605/807307
// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<char, bool>(is_mm_whitespace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<char, bool>(is_mm_whitespace))).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (copying)
static inline std::string ltrimmed(std::string s) {
    ltrim(s);
    return s;
}

// trim from end (copying)
static inline std::string rtrimmed(std::string s) {
    rtrim(s);
    return s;
}

// trim from both ends (copying)
static inline std::string trimmed(std::string s) {
    trim(s);
    return s;
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

class HasherSink : public boost::iostreams::sink {
public:
    std::streamsize write(const char* s, std::streamsize n);
    std::string get_digest();

private:
    CryptoPP::SHA256 hasher;
};

template< typename T >
std::string hash_object(const T &obj) {
    HasherSink hasher;
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

#endif // UTILS_H
