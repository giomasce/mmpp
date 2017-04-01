
#include <atomic>
#include <iostream>
#include <csignal>
#include <random>

#include <boost/tokenizer.hpp>

#include "parser.h"
#include "memory.h"
#include "utils.h"
#include "web.h"

using namespace std;

mt19937 rand_mt;

void init_random() {
    random_device rand_dev;
    rand_mt.seed(rand_dev());
}

string generate_id() {
    int len = 100;
    vector< char > id(len);
    uniform_int_distribution< char > dist('a', 'z');
    for (int i = 0; i < len; i++) {
        id[i] = dist(rand_mt);
    }
    return string(id.begin(), id.end());
}

atomic< bool > signalled;
void int_handler(int signal) {
    (void) signal;
    signalled = true;
}

int httpd_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    init_random();

    /*cout << "Reading set.mm..." << endl;
    FileTokenizer ft("../set.mm/set.mm");
    //FileTokenizer ft("../metamath/ql.mm");
    Parser p(ft, false, true);
    p.run();
    LibraryImpl lib = p.get_library();
    //LibraryToolbox tb(lib, true);
    cout << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << endl;
    cout << "Memory usage after loading the library: " << size_to_string(getCurrentRSS()) << endl;*/

    WebEndpoint endpoint;

    int port = 8888;
    HTTPD_microhttpd httpd(port, endpoint);

    struct sigaction act;
    act.sa_handler = int_handler;
    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    httpd.start();

    // Generate a session and pass it to the browser
    string ticket_id = endpoint.create_session_and_ticket();
    string browser_url = "http://127.0.0.1:" + to_string(port) + "/ticket/" + ticket_id;
    system(("xdg-open " + browser_url).c_str());

    while (true) {
        if (signalled) {
            cerr << "Signal received, stopping..." << endl;
            httpd.stop();
            break;
        }
        sleep(1);
    }
    httpd.join();

    return 0;
}

WebEndpoint::WebEndpoint()
{
}

vector< pair< string, string > > content_types = {
    { "css", "text/css" },
    { "gif", "image/gif" },
    { "htm", "text/html" },
    { "html", "text/html" },
    { "ico", "image/x-icon" },
    { "jpeg", "image/jpeg" },
    { "jpg", "image/jpeg" },
    { "js", "application/javascript" },
    { "json", "application/json" },
    { "pdf", "application/pdf" },
    { "png", "image/png" },
    { "svg", "image/svg+xml" },
};
string guess_content_type(string url) {
    for (const auto &pair : content_types) {
        if (url.size() >= pair.first.size() && equal(pair.first.rbegin(), pair.first.rend(), url.rbegin())) {
            return pair.second;
        }
    }
    return "application/octet-stream";
}

string WebEndpoint::answer(HTTPCallback &cb, string url, string method, string version)
{
    (void) version;

    // Receive session ticket
    string cookie_name = "mmpp_session_id";
    string ticket_url = "/ticket/";
    if (method == "GET" && equal(ticket_url.begin(), ticket_url.end(), url.begin())) {
        unique_lock< shared_mutex > lock(this->sessions_mutex);
        string ticket = string(url.begin() + ticket_url.size(), url.end());
        if (this->session_tickets.find(ticket) != this->session_tickets.end()) {
            // The ticket is valid, we set the session and remove the ticket
            cb.add_header("Set-Cookie", cookie_name + "=" + this->session_tickets.at(ticket) + "; path=/; httponly");
            this->session_tickets.erase(ticket);
            cb.add_header("Location", "/");
            cb.set_status_code(302);
            return "";
        } else {
            // The ticket is not valid, we forbid
            cb.set_status_code(403);
            cb.add_header("Content-Type", "text/plain");
            return "403 Forbidden";
        }
    }

    // Check auth cookie and recover session
    shared_ptr< Session > session;
    try {
        session = this->get_session(cb.get_cookies().at(cookie_name));
    } catch (out_of_range e) {
        cb.set_status_code(403);
        cb.add_header("Content-Type", "text/plain");
        return "403 Forbidden";
    }

    // Redirect to the main app
    if (method == "GET" && url == "/") {
        cb.add_header("Location", "/static/index.html");
        cb.set_status_code(302);
        return "";
    }

    // Serve static files (FIXME this code is terrible from security POV)
    string static_url = "/static/";
    if (method == "GET" && equal(static_url.begin(), static_url.end(), url.begin())) {
        // FIXME Sanitize URL
        string filename = RESOURCES_BASE + string(url.begin(), url.end());
        ifstream infile(filename, ios::binary);
        if (infile.rdstate() && ios::failbit) {
            cb.set_status_code(404);
            cb.add_header("Content-Type", "text/plain");
            return "404 Not Found";
        }
        string content;
        try {
            content = string(istreambuf_iterator< char >(infile), istreambuf_iterator< char >());
        } catch(exception &e) {
            (void) e;
            cb.set_status_code(404);
            cb.add_header("Content-Type", "text/plain");
            return "404 Not Found";
        }
        cb.set_status_code(200);
        cb.add_header("Content-Type", guess_content_type(url));
        return content;
    }

    // Expose API version
    string api_version_url = "/api/version";
    if (url == api_version_url) {
        Json::Value res;
        res["application"] = "mmpp";
        res["min_version"] = 1;
        res["max_version"] = 1;
        Json::FastWriter writer;
        cb.add_header("Content-Type", "application/json");
        cb.set_status_code(200);
        return writer.write(res);
    }

    // Serve API requests
    string api_url = "/api/1/";
    if (equal(api_url.begin(), api_url.end(), url.begin())) {
        boost::char_separator< char > sep("/");
        boost::tokenizer< boost::char_separator< char > > tokens(url.begin() + api_url.size(), url.end(), sep);
        const vector< string > path(tokens.begin(), tokens.end());
        try {
            Json::Value res = session->answer_api1(cb, path.begin(), path.end(), method);
            Json::FastWriter writer;
            cb.add_header("Content-Type", "application/json");
            cb.set_status_code(200);
            return writer.write(res);
        } catch (SendError se) {
            cb.add_header("Content-Type", "text/plain");
            cb.set_status_code(se.get_status_code());
            return to_string(se.get_status_code()) + " " + se.get_descr();
        }
    }

    // If nothing has serviced the request yet, then this is a 404
    cb.set_status_code(404);
    cb.add_header("Content-Type", "text/plain");
    return "404 Not Found";
}

string WebEndpoint::create_session_and_ticket()
{
    unique_lock< shared_mutex > lock(this->sessions_mutex);
    string session_id = generate_id();
    string ticket_id = generate_id();
    this->session_tickets[ticket_id] = session_id;
    this->sessions[session_id] = make_shared< Session >();
    return ticket_id;
}

shared_ptr< Session > WebEndpoint::get_session(string session_id)
{
    shared_lock< shared_mutex > lock(this->sessions_mutex);
    try {
        return this->sessions.at(session_id);
    } catch(out_of_range e) {
        (void) e;
        throw SendError(404);
    }
}

Session::Session()
{
}

Json::Value Session::answer_api1(HTTPCallback &cb, vector< string >::const_iterator path_begin, vector< string >::const_iterator path_end, string method)
{
    if (path_begin != path_end && *path_begin == "workset") {
        path_begin++;
        if (path_begin == path_end) {
            throw SendError(404);
        }

        if (*path_begin == "create") {
            path_begin++;
            if (path_begin != path_end) {
                throw SendError(404);
            }
            auto res = this->create_workset();
            Json::Value ret;
            ret["id"] = Json::Value::Int(res.first);
            return ret;
        } else {
            size_t id = safe_stoi(*path_begin);
            path_begin++;
            return this->get_workset(id)->answer_api1(cb, path_begin, path_end, method);
        }
    }
    throw SendError(404);
}

std::pair<size_t, std::shared_ptr<Workset> > Session::create_workset()
{
    unique_lock< shared_mutex > lock(this->worksets_mutex);
    size_t id = this->worksets.size();
    this->worksets.push_back(make_shared< Workset >());
    return { id, this->worksets.at(id) };
}

std::shared_ptr<Workset> Session::get_workset(size_t id)
{
    shared_lock< shared_mutex > lock(this->worksets_mutex);
    try {
        return this->worksets.at(id);
    } catch(out_of_range e) {
        (void) e;
        throw SendError(404);
    }
}

Workset::Workset()
{
}

Json::Value Workset::answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end, std::string method)
{
    if (path_begin == path_end) {
        Json::Value ret = Json::objectValue;
        return ret;
    }
    if (*path_begin == "load") {
        FileTokenizer ft("../set.mm/set.mm");
        Parser p(ft, false, true);
        p.run();
        this->library = make_unique< LibraryImpl >(p.get_library());
        Json::Value ret;
        ret["symbols_num"] = Json::Value::Int(this->library->get_symbols_num());
        ret["labels_num"] = Json::Value::Int(this->library->get_labels_num());
        return ret;
    }
    throw SendError(404);
}

SendError::SendError(unsigned int status_code) : status_code(status_code)
{
}

unsigned int SendError::get_status_code()
{
    return this->status_code;
}

string SendError::get_descr()
{
    return this->errors.at(this->get_status_code());
}

int safe_stoi(string s)
{
    try {
        return stoi(s);
    } catch (invalid_argument e) {
        (void) e;
        throw SendError(404);
    } catch (out_of_range e) {
        (void) e;
        throw SendError(404);
    }
}
