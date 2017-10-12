
#include "utils.h"

using namespace std;

#if defined(__GNUG__) && defined(EXCEPTIONS_SELF_DEBUG)
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <stdlib.h>

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

Tic tic() {
    Tic t;
    t.begin = std::chrono::steady_clock::now();
    return t;
}

void toc(const Tic &t, int reps) {
    auto end = std::chrono::steady_clock::now();
    auto usecs = std::chrono::duration_cast< std::chrono::microseconds >(end - t.begin).count();
    cout << "It took " << usecs << " microseconds to repeat " << reps << " times, which is " << (usecs / reps) << " microsecond per execution." << endl;
}

unordered_map< string, function< int(int, char*[]) > > &get_main_functions() {
    static auto *ret = new unordered_map< string, function< int(int, char*[]) > >();
    return *ret;
}

void register_main_function(const string &name, const function< int(int, char*[]) > &main_function) {
    get_main_functions().insert(make_pair(name, main_function));
}

streamsize HasherSink::write(const char *s, streamsize n) {
    this->hasher.Update(reinterpret_cast< const ::byte* >(s), n);
    return n;
}

string HasherSink::get_digest() {
    ::byte digest[decltype(hasher)::DIGESTSIZE];
    this->hasher.Final(digest);
    return std::string(reinterpret_cast< const char* >(digest), sizeof(decltype(digest)));
}
