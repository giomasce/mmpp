#ifndef STATICS_H
#define STATICS_H

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

#ifdef __GNUG__
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <stdlib.h>

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
    MMPPException(std::string reason);
    const std::string &get_reason() const;
    const std::vector< std::string > &get_stacktrace() const;
    void print_stacktrace(std::ostream &st) const;

private:
    std::string reason;
    std::vector< std::string > stacktrace;
};

inline static void assert_or_throw(bool cond, std::string reason="") {
    if (!cond) {
        throw MMPPException(reason);
    }
}

inline static bool is_ascii(char c) {
    return c > 32 && c < 127;
}

inline static bool is_dollar(char c) {
    return c == '$';
}

inline static bool is_valid(char c) {
    return is_ascii(c) && !is_dollar(c);
}

inline static bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f';
}

inline static bool is_label_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.';
}

inline static bool is_label(std::string s) {
    for (auto c : s) {
        if (!is_label_char(c)) {
            return false;
        }
    }
    return true;
}

inline static bool is_symbol(std::string s) {
    for (auto c : s) {
        if (!is_valid(c)) {
            return false;
        }
    }
    return true;
}

// Taken from http://stackoverflow.com/a/217605/807307
// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<char, bool>(is_whitespace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<char, bool>(is_whitespace))).base(), s.end());
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

#endif // STATICS_H
