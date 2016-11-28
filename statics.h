#ifndef STATICS_H
#define STATICS_H

#include <set>
#include <string>

class MMPPException {
public:
    MMPPException(std::string reason);
    const std::string &get_reason() {
        return this->reason;
    }

private:
    std::string reason;
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

#endif // STATICS_H
