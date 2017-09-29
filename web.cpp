
#include <iostream>
#include <random>

#include <boost/tokenizer.hpp>
#include <boost/filesystem/fstream.hpp>

#include "reader.h"
#include "memory.h"
#include "utils.h"
#include "web.h"
#include "platform.h"

using namespace std;
using namespace nlohmann;

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

int httpd_main(int argc, char *argv[]) {
    if (!platform_init(argc, argv)) {
        return 1;
    }

    init_random();

    int port = 8888;
    WebEndpoint endpoint(port);
    HTTPD_microhttpd httpd(port, endpoint);

    httpd.start();

    // Generate a session and pass it to the browser
    string ticket_id = endpoint.create_session_and_ticket();
    string browser_url = "http://127.0.0.1:" + to_string(port) + "/ticket/" + ticket_id;
    platform_open_browser(browser_url);
    cout << "A browser session was spawned; if you cannot see it, go to " << browser_url << endl;

    while (true) {
        if (platform_should_stop()) {
            cerr << "Signal received, stopping..." << endl;
            httpd.stop();
            break;
        }
        sleep(1);
    }
    httpd.join();

    return 0;
}

WebEndpoint::WebEndpoint(int port) :
    port(port)
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
    { "woff", "application/font-woff" },
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
    if (method == "GET" && starts_with(url, ticket_url)) {
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
    if (method == "GET" && starts_with(url, static_url)) {
        // FIXME Sanitize URL
        auto filename = platform_get_resources_base() / string(url.begin(), url.end());
        boost::filesystem::ifstream infile(filename, ios::binary);
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
        json res;
        res["application"] = "mmpp";
        res["min_version"] = 1;
        res["max_version"] = 1;
        cb.add_header("Content-Type", "application/json");
        cb.set_status_code(200);
        return res.dump();
    }

    // Backdoor for easily creating tickets (FIXME disable in production)
    if (true) {
        string api_create_ticket = "/api/create_ticket";
        if (url == api_create_ticket) {
            cb.add_header("Content-Type", "text/plain");
            cb.set_status_code(200);
            string ticket_id = this->create_session_and_ticket();
            string browser_url = "http://127.0.0.1:" + to_string(this->port) + "/ticket/" + ticket_id;
            return browser_url;
        }
    }

    // Serve API requests
    string api_url = "/api/1/";
    if (starts_with(url, api_url)) {
        boost::char_separator< char > sep("/");
        boost::tokenizer< boost::char_separator< char > > tokens(url.begin() + api_url.size(), url.end(), sep);
        const vector< string > path(tokens.begin(), tokens.end());
        try {
            json res = session->answer_api1(cb, path.begin(), path.end(), method);
            cb.add_header("Content-Type", "application/json");
            cb.set_status_code(200);
            return res.dump();
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

json Session::answer_api1(HTTPCallback &cb, vector< string >::const_iterator path_begin, vector< string >::const_iterator path_end, string method)
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
            json ret = { { "id", res.first } };
            return ret;
        } else {
            size_t id = safe_stoi(*path_begin);
            path_begin++;
            auto workset = this->get_workset(id);
            return workset->answer_api1(cb, path_begin, path_end, method);
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
