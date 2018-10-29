
#include "utils.h"

#include <boost/crc.hpp>
#include <boost/type_index.hpp>

// Partly taken from http://programanddesign.com/cpp/human-readable-file-size-in-c/
std::string size_to_string(uint64_t size) {
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

MMPPException::MMPPException(const std::string &reason) : reason(reason), stacktrace(platform_get_stack_trace()) {
    if (mmpp_abort) {
        std::cerr << "Exception with message: " << reason << std::endl;
        this->print_stacktrace(std::cerr);
        abort();
    }
}

const std::string &MMPPException::get_reason() const {
    return this->reason;
}

const PlatformStackTrace &MMPPException::get_stacktrace() const {
    return this->stacktrace;
}

void MMPPException::print_stacktrace(std::ostream &st) const {
    platform_dump_stack_trace(st, this->get_stacktrace());
}

Tic tic() {
    Tic t;
    t.begin = std::chrono::steady_clock::now();
    return t;
}

void toc(const Tic &t, int reps) {
    auto end = std::chrono::steady_clock::now();
    auto usecs = std::chrono::duration_cast< std::chrono::microseconds >(end - t.begin).count();
    std::cout << "It took " << usecs << " microseconds to repeat " << reps << " times, which is " << (usecs / reps) << " microsecond per execution." << std::endl;
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
    std::cout << std::fixed << std::setprecision(0);
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
    std::cout << "\033[2K\r";
    std::cout << "|";
    size_t i = 0;
    for (; i < cur_len; i++) {
        std::cout << "#";
    }
    for (; i < this->length; i++) {
        std::cout << "-";
    }
    std::cout << "|";
    std::cout << " " << current << " / " << this->total;
    std::cout.flush();
}

void TextProgressBar::report(double current)
{
    this->report(current, false);
}

void TextProgressBar::finished()
{
    this->report(this->total, true);
    std::cout << std::defaultfloat << std::endl;
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

HashSink::HashSink() : hasher(std::make_shared< HasherCRC32 >())
{
}

std::streamsize HashSink::write(const char *s, std::streamsize n)
{
    this->hasher->update(s, n);
    return n;
}

std::string HashSink::get_digest()
{
    return this->hasher->get_digest();
}

void default_exception_handler(std::exception_ptr ptr)
{
    try {
        if (ptr) {
            std::rethrow_exception(ptr);
        }
    } catch (const MMPPException &e) {
        std::cerr << "Exception of type MMPPException: " << e.get_reason() << std::endl;
        platform_dump_stack_trace(std::cerr, e.get_stacktrace());
    } catch (const char* &e) {
        std::cerr << "Exception of type char*: " << e << std::endl;
    } catch (const std::string &e) {
        std::cerr << "Exception of type std::string: " << e << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Exception of dynamic type " << boost::typeindex::type_id_runtime(e).pretty_name() << ": " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Exception of unmanaged type " << platform_type_of_current_exception() << std::endl;
    }
}
