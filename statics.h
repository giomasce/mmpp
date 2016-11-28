#ifndef STATICS_H
#define STATICS_H

#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <ostream>
#include <iostream>

#ifdef __GNUG__
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <stdlib.h>

// Taken from https://stupefydeveloper.blogspot.it/2008/10/cc-call-stack.html and partially adapted
inline static std::vector< std::string > dump_stacktrace(size_t depth) {
    using namespace std;
    using namespace abi;

    vector< string > ret;
    vector< void* > trace(depth);
    Dl_info dlinfo;
    int status;
    const char *symname;
    char *demangled;
    int trace_size = backtrace(trace.data(), depth);
    for (int i=0; i<trace_size; ++i)
    {
        if(!dladdr(trace[i], &dlinfo))
            continue;

        symname = dlinfo.dli_sname;

        demangled = __cxa_demangle(symname, NULL, 0, &status);
        if(status == 0 && demangled)
            symname = demangled;

        ostringstream oss;
        oss << "address: 0x" << trace[i] << ", object: " << dlinfo.dli_fname << ", function: " << symname;
        ret.push_back(oss.str());

        if (demangled)
            free(demangled);
    }
    return ret;
}

inline static std::vector< std::string > dump_stacktrace() {
    for (size_t depth = 10; ; depth *= 2) {
        auto ret = dump_stacktrace(depth);
        if (ret.size() < depth) {
            return ret;
        }
    }
}

#else
inline static std::vector< std::string > dump_stacktrace(int depth=0) {
    return {};
}
#endif

extern bool mmpp_abort;

class MMPPException {
public:
    MMPPException(std::string reason) : reason(reason), stacktrace(dump_stacktrace()) {
        if (mmpp_abort) {
            this->print_stacktrace(std::cerr);
            abort();
        }
    }
    const std::string &get_reason() const {
        return this->reason;
    }
    const std::vector< std::string > &get_stacktrace() const {
        return this->stacktrace;
    }
    void print_stacktrace(std::ostream &st) const {
        using namespace std;
        st << "Stack trace:" << endl;
        for (auto &frame : this->stacktrace) {
            st << "  * " << frame << endl;
        }
        st << "End of stack trace" << endl;
        st.flush();
    }

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

#endif // STATICS_H
