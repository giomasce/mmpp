#pragma once

#include <functional>
#include <set>
#include <vector>
#include <cstdint>
#include <numeric>
#include <unordered_map>

#include <boost/functional/hash.hpp>

#include "libs/json.h"

#include "utils/utils.h"

// This definition is inspired to BOOST_STRONG_TYPEDEF, but supports more things
#define TOK_TYPEDEF(N, T) \
static_assert(std::is_integral< N >::value, "Must be an integral type"); \
static_assert(std::is_unsigned< N >::value, "Must be unsigned"); \
class T { \
private: \
    N val_; \
public: \
    \
    /* Some static things */ \
    static T maxval() { return T((std::numeric_limits< N >::max)()); } \
    typedef N val_type; \
    \
    /* Basic things, as for BOOST_STRONG_TYPEDEF */  \
    T() : val_() {} \
    explicit T(const N val) : val_(val) {} \
    T &operator=(const N &x) { this->val_ = x; return *this; } \
    T &operator=(const T &x) { this->val_ = x.val_; return *this; } \
    bool operator==(const T &x) const { return this->val_ == x.val_; } \
    bool operator!=(const T &x) const { return this->val_ != x.val_; } \
    bool operator<(const T &x) const { return this->val_ < x.val_; } \
    bool operator<=(const T &x) const { return this->val_ <= x.val_; } \
    bool operator>(const T &x) const { return this->val_ > x.val_; } \
    bool operator>=(const T &x) const { return this->val_ >= x.val_; } \
    \
    /* We permit to get the encapsulated value, but through an explicit call */ \
    N val() const { return this->val_; } \
    \
    /* Support boost serialization */ \
    friend class boost::serialization::access; \
    template< class Archive > \
    void serialize(Archive &ar, const unsigned int version) { \
        (void) version; \
        ar & this->val_; \
    } \
}; \
\
/* Support standard library hashing */ \
namespace std { \
template<> \
struct hash< T > { \
    typedef T argument_type; \
    typedef std::size_t result_type; \
    result_type operator()(const argument_type &x) const noexcept { \
        return hash< N >()(x.val()); \
    } \
}; \
} \
\
/* Support boost hashing */ \
namespace boost { \
template<> \
struct hash< T > { \
    typedef T argument_type; \
    typedef std::size_t result_type; \
    result_type operator()(const argument_type &x) const noexcept { \
        return hash< N >()(x.val()); \
    } \
}; \
} \
\
/* Support for (de)serializing to/from JSON */ \
namespace nlohmann { \
inline void to_json(nlohmann::json &j, const T &x) { \
    j = x.val(); \
} \
inline void from_json(const nlohmann::json &j, T &x) { \
    x = { j.get< N >() }; \
} \
}

TOK_TYPEDEF(uint32_t, SymTok)
TOK_TYPEDEF(uint32_t, LabTok)
TOK_TYPEDEF(uint32_t, CodeTok)

typedef std::vector< SymTok > Sentence;
typedef std::vector< LabTok > Procedure;

// See https://stackoverflow.com/a/27443191
const CodeTok INVALID_CODE = (std::numeric_limits< CodeTok >::max)();

class MMPPParsingError : public MMPPException {
    using MMPPException::MMPPException;
};

void collect_variables(const Sentence &sent, const std::function< bool(SymTok) > &is_var, std::set< SymTok > &vars);
Sentence substitute(const Sentence &orig, const std::unordered_map<SymTok, std::vector<SymTok> > &subst_map, const std::function<bool(SymTok)> &is_var);

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
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](char ch) { return !is_mm_whitespace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](char ch) { return !is_mm_whitespace(ch); }).base(), s.end());
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
