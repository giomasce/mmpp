#pragma once

#include <functional>
#include <set>
#include <vector>
#include <cstdint>
#include <numeric>

#include "utils/utils.h"

typedef uint32_t SymTok;
typedef uint32_t LabTok;
typedef uint32_t CodeTok;

static_assert(std::is_integral< SymTok >::value);
static_assert(std::is_unsigned< SymTok >::value);
static_assert(std::is_integral< LabTok >::value);
static_assert(std::is_unsigned< LabTok >::value);
static_assert(std::is_integral< CodeTok >::value);
static_assert(std::is_unsigned< CodeTok >::value);

typedef std::vector< SymTok > Sentence;
typedef std::vector< LabTok > Procedure;

const CodeTok INVALID_CODE = std::numeric_limits< CodeTok >::max();

class MMPPParsingError : public MMPPException {
    using MMPPException::MMPPException;
};

void collect_variables(const Sentence &sent, const std::function< bool(SymTok) > &is_var, std::set< SymTok > &vars);

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
