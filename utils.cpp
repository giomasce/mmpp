
#include "utils.h"

using namespace std;

#ifdef __GNUG__

// Taken from https://stupefydeveloper.blogspot.it/2008/10/cc-call-stack.html and partially adapted
std::vector<string> dump_stacktrace(size_t depth) {
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

std::vector<string> dump_stacktrace() {
    for (size_t depth = 10; ; depth *= 2) {
        auto ret = dump_stacktrace(depth);
        if (ret.size() < depth) {
            return ret;
        }
    }
}

#endif

// Partly taken from http://programanddesign.com/cpp/human-readable-file-size-in-c/
string size_to_string(size_t size) {
    std::ostringstream stream;
    int i = 0;
    const char* units[] = {"B", "kiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};
    while (size > 1024) {
        size /= 1024;
        i++;
    }
    stream << std::fixed << std::setprecision(2) << size << " " << units[i];
    return stream.str();
}

MMPPException::MMPPException(string reason) : reason(reason), stacktrace(dump_stacktrace()) {
    if (mmpp_abort) {
        std::cerr << "Exception with message: " << reason << std::endl;
        this->print_stacktrace(std::cerr);
        abort();
    }
}

const string &MMPPException::get_reason() const {
    return this->reason;
}

const std::vector<string> &MMPPException::get_stacktrace() const {
    return this->stacktrace;
}

void MMPPException::print_stacktrace(ostream &st) const {
    using namespace std;
    st << "Stack trace:" << endl;
    for (auto &frame : this->stacktrace) {
        st << "  * " << frame << endl;
    }
    st << "End of stack trace" << endl;
    st.flush();
}

bool starts_with(string a, string b) {
    if (b.size() > a.size()) {
        return false;
    }
    return equal(b.begin(), b.end(), a.begin());
}
