
#include "utils.h"

#include <boost/crc.hpp>

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
        if(!dladdr(trace[i], &dlinfo)) {
            continue;
        }

        symname = dlinfo.dli_sname;

        demangled = __cxa_demangle(symname, nullptr, 0, &status);
        if (status == 0 && demangled) {
            symname = demangled;
        }

        ostringstream oss;
        oss << "address: 0x" << trace[i] << ", object: " << dlinfo.dli_fname << ", function: " << symname;
        ret.push_back(oss.str());

        if (demangled) {
            free(demangled);
        }
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
string size_to_string(uint64_t size) {
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

MMPPException::MMPPException(const string &reason) : reason(reason), stacktrace(dump_stacktrace()) {
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

map< string, function< int(int, char*[]) > > &get_main_functions() {
    static auto ret = make_unique< map< string, function< int(int, char*[]) > > >();
    return *ret;
}

void register_main_function(const string &name, const function< int(int, char*[]) > &main_function) {
    get_main_functions().insert(make_pair(name, main_function));
}

class HasherCRC32 final : public Hasher {
public:
    void update(const char *s, std::size_t n) {
        this->hasher.process_bytes(s, n);
    }

    std::string get_digest() {
        auto digest = this->hasher.checksum();
        return std::string(reinterpret_cast< const char* >(&digest), sizeof(digest));
    }

private:
    boost::crc_32_type hasher;
};

TextProgressBar::TextProgressBar(size_t length, double total) : last_len(0), total(total), length(length) {
    cout << fixed << setprecision(0);
}

void TextProgressBar::set_total(double total)
{
    this->total = total;
}

void TextProgressBar::report(double current, bool force) {
    /* In theory it is nice to redraw based on timing, but on older
     * CPUs this spawns a lot of system calls and slows things a lot.
     * So we switch to redrawing when the position of the bar
     * changes. */
    /*auto now = std::chrono::steady_clock::now();
    if (!force && current != this->total && now - this->last_report < 0.1s) {
        return;
    }
    this->last_report = now;*/
    // Truncation happens by default
    size_t cur_len;
    if (this->total == 0.0) {
        cur_len = 0;
    } else {
        cur_len = static_cast< size_t >(current / this->total * this->length);
    }
    if (!force && cur_len != this->total && cur_len == this->last_len) {
        return;
    }
    this->last_len = cur_len;
    cout << "\033[2K\r";
    cout << "|";
    size_t i = 0;
    for (; i < cur_len; i++) {
        cout << "#";
    }
    for (; i < this->length; i++) {
        cout << "-";
    }
    cout << "|";
    cout << " " << current << " / " << this->total;
    cout.flush();
}

void TextProgressBar::report(double current)
{
    this->report(current, false);
}

void TextProgressBar::finished()
{
    this->report(this->total, true);
    cout << defaultfloat << endl;
}

Reportable::~Reportable() {
}

// From https://stackoverflow.com/a/11826666/807307
class NullBuffer : public std::streambuf
{
public:
  int overflow(int c) { return c; }
};
NullBuffer cnull_buffer;
std::ostream cnull(&cnull_buffer);

HashSink::HashSink() : hasher(make_shared< HasherCRC32 >())
{
}

streamsize HashSink::write(const char *s, streamsize n)
{
    this->hasher->update(s, n);
    return n;
}

string HashSink::get_digest()
{
    return this->hasher->get_digest();
}
