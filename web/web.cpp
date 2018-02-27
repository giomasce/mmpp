
#include <iostream>
#include <random>

#include <boost/tokenizer.hpp>
#include <boost/filesystem/fstream.hpp>

#include "mm/reader.h"
#include "memory.h"
#include "utils/utils.h"
#include "web.h"
#include "platform.h"
#include "httpd_microhttpd.h"

using namespace std;
using namespace nlohmann;

const bool SERVE_STATIC_FILES = true;
const bool PUBLICLY_SERVE_STATIC_FILES = true;

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

unique_ptr< HTTPD > make_server(int port, WebEndpoint &endpoint, bool open_server) {
#if defined(USE_MICROHTTPD)
    return make_unique< HTTPD_microhttpd >(port, endpoint, !open_server);
#else
    // If no HTTP implementation is provided, we return NULL
    (void) port;
    (void) endpoint;
    (void) open_server;
    return NULL;
#endif
}

int webmmpp_main_common(int argc, char *argv[], bool open_server) {
    if (!platform_init(argc, argv)) {
        return 1;
    }

    init_random();

    int port = 8888;
    WebEndpoint endpoint(port, open_server);
    unique_ptr< HTTPD > httpd = make_server(port, endpoint, open_server);
    if (httpd == NULL) {
        cerr << "Could not build an HTTP server, exiting" << endl;
        return 1;
    }

    if (open_server) {
        // The session is already available, but it is constant; we need to populate it with some content
        shared_ptr< Session > session = endpoint.get_guest_session();
        shared_ptr< Workset > workset;
        tie(ignore, workset) = session->create_workset();
        workset->set_name("Default workset");
        workset->load_library(platform_get_resources_base() / "set.mm", platform_get_resources_base() / "set.mm.cache", "|-");
    }

    httpd->start();

    if (!open_server) {
        // Generate a session and pass it to the browser
        string ticket_id = endpoint.create_session_and_ticket();
        string browser_url = "http://127.0.0.1:" + to_string(port) + "/ticket/" + ticket_id;
        platform_open_browser(browser_url);
        cout << "A browser session was spawned; if you cannot see it, go to " << browser_url << endl;
    }

    while (true) {
        if (platform_should_stop()) {
            cerr << "Signal received, stopping..." << endl;
            httpd->stop();
            break;
        }
        sleep(1);
    }
    httpd->join();

    return 0;
}

int webmmpp_main(int argc, char*argv[]) {
    return webmmpp_main_common(argc, argv, false);
}
static_block {
    register_main_function("webmmpp", webmmpp_main);
}

int webmmpp_open_main(int argc, char*argv[]) {
    return webmmpp_main_common(argc, argv, true);
}
static_block {
    register_main_function("webmmpp_open", webmmpp_open_main);
}

WebEndpoint::WebEndpoint(int port, bool enable_guest_session) :
    port(port), guest_session(enable_guest_session ? make_shared< Session >(true) : NULL)
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

void WebEndpoint::answer(HTTPCallback &cb)
{
    // Receive session ticket
    string cookie_name = "mmpp_session_id";
    string ticket_url = "/ticket/";
    const string &url = cb.get_url();
    const string &method = cb.get_method();
    if (method == "GET" && starts_with(url, ticket_url)) {
        unique_lock< mutex > lock(this->sessions_mutex);
        string ticket = string(url.begin() + ticket_url.size(), url.end());
        if (this->session_tickets.find(ticket) != this->session_tickets.end()) {
            // The ticket is valid, we set the session and remove the ticket
            cb.add_header("Set-Cookie", cookie_name + "=" + this->session_tickets.at(ticket) + "; path=/; httponly");
            this->session_tickets.erase(ticket);
            cb.add_header("Location", "/");
            cb.set_status_code(302);
            cb.set_answer("");
            return;
        } else {
            // The ticket is not valid, we forbid
            cb.set_status_code(403);
            cb.add_header("Content-Type", "text/plain");
            cb.set_answer("403 Forbidden");
            return;
        }
    }

    // Check auth cookie and recover session; at this point we allow a NULL session if we are publicly serving static files
    string session_cookie;
    try {
        session_cookie = cb.get_cookies().at(cookie_name);
    } catch (out_of_range) {
        session_cookie = "";
    }
    shared_ptr< Session > session = this->get_session(session_cookie);
    if (!PUBLICLY_SERVE_STATIC_FILES && session == NULL) {
        cb.set_status_code(403);
        cb.add_header("Content-Type", "text/plain");
        cb.set_answer("403 Forbidden");
        return;
    }

    // Serve static files
    // TODO This is not the greatest static file server ever...
    if (SERVE_STATIC_FILES) {
        string static_url = "/static/";
        boost::filesystem::path static_dir = boost::filesystem::canonical(platform_get_resources_base() / "static");
        if (method == "GET" && starts_with(url, static_url)) {
            boost::filesystem::path filename = platform_get_resources_base() / string(url.begin(), url.end());
            // First we have to check if the file exists, because canonical() requires it
            if (!boost::filesystem::exists(filename)) {
                cb.set_status_code(404);
                cb.add_header("Content-Type", "text/plain");
                cb.set_answer("404 Not Found");
                return;
            }
            // Then we canonicalize the path and check that it still is under the right directory by repeatedly passing to the parent directory
            boost::filesystem::path canonical_filename = boost::filesystem::canonical(filename);
            bool is_in_static = false;
            for (boost::filesystem::path i = canonical_filename; !i.empty(); i = i.parent_path()) {
                if (i == static_dir) {
                    is_in_static = true;
                    break;
                }
            }
            if (!is_in_static) {
                // We respond 404 and not 403, because the client could query the existence of files outside the static dir
                cb.set_status_code(404);
                cb.add_header("Content-Type", "text/plain");
                cb.set_answer("404 Not Found");
                return;
            }
            // Ok, we are cleared to send the file
            auto infile = make_shared< boost::filesystem::ifstream >(filename, ios::binary);
            if (!(*infile)) {
                cb.set_status_code(404);
                cb.add_header("Content-Type", "text/plain");
                cb.set_answer("404 Not Found");
                return;
            }
            cb.set_status_code(200);
            cb.add_header("Content-Type", guess_content_type(url));
            /*string content;
            try {
                content = string(istreambuf_iterator< char >(infile), istreambuf_iterator< char >());
            } catch(exception) {
                cb.set_status_code(404);
                cb.add_header("Content-Type", "text/plain");
                cb.set_answer("404 Not Found");
                return;
            }
            cb.set_answer(move(content));*/
            cb.set_answerer(make_unique< HTTPFileAnswerer >(infile));
            return;
        }
    }

    // Check again the session: if you do not have a valid one, you cannot go past here
    if (session == NULL) {
        cb.set_status_code(403);
        cb.add_header("Content-Type", "text/plain");
        cb.set_answer("403 Forbidden");
        return;
    }

    // Redirect to the main app
    if (method == "GET" && url == "/") {
        cb.add_header("Location", "/static/index.html");
        cb.set_status_code(302);
        cb.set_answer("");
        return;
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
        cb.set_answer(res.dump());
        return;
    }

    // Backdoor for easily creating tickets (only enable on test builds)
    if (false) {
        string api_create_ticket = "/api/create_ticket";
        if (url == api_create_ticket) {
            cb.add_header("Content-Type", "text/plain");
            cb.set_status_code(200);
            string ticket_id = this->create_session_and_ticket();
            string browser_url = "http://127.0.0.1:" + to_string(this->port) + "/ticket/" + ticket_id;
            cb.set_answer(move(browser_url));
            return;
        }
    }

    // Serve API requests
    string api_url = "/api/1/";
    if (starts_with(url, api_url)) {
        boost::char_separator< char > sep("/");
        boost::tokenizer< boost::char_separator< char > > tokens(url.begin() + api_url.size(), url.end(), sep);
        const vector< string > path(tokens.begin(), tokens.end());
        try {
            json res = session->answer_api1(cb, path.begin(), path.end());
            cb.add_header("Content-Type", "application/json");
            cb.set_status_code(200);
            cb.set_answer(res.dump());
            return;
        } catch (SendError se) {
            cb.add_header("Content-Type", "text/plain");
            cb.set_status_code(se.get_status_code());
            cb.set_answer(to_string(se.get_status_code()) + " " + se.get_descr());
            return;
        } catch (WaitForPost wfp) {
            auto callback = [wfp,&cb] (const auto &post_data) {
                try {
                    json res = wfp.get_callback()(post_data);
                    cb.add_header("Content-Type", "application/json");
                    cb.set_status_code(200);
                    cb.set_answer(res.dump());
                } catch (SendError se) {
                    cb.add_header("Content-Type", "text/plain");
                    cb.set_status_code(se.get_status_code());
                    cb.set_answer(to_string(se.get_status_code()) + " " + se.get_descr());
                }
            };
            std::function< void(const std::unordered_map< std::string, PostItem >&) > cb2 = callback;
            cb.set_post_iterator(make_unique< HTTPSimplePostIterator >(cb2));
            return;
        }
    }

    // If nothing has serviced the request yet, then this is a 404
    cb.set_status_code(404);
    cb.add_header("Content-Type", "text/plain");
    cb.set_answer("404 Not Found");
}

std::shared_ptr<Session> WebEndpoint::get_guest_session()
{
    return this->guest_session;
}

string WebEndpoint::create_session_and_ticket()
{
    unique_lock< mutex > lock(this->sessions_mutex);
    string session_id = generate_id();
    string ticket_id = generate_id();
    this->session_tickets[ticket_id] = session_id;
    this->sessions[session_id] = make_shared< Session >();
    return ticket_id;
}

shared_ptr< Session > WebEndpoint::get_session(string session_id)
{
    unique_lock< mutex > lock(this->sessions_mutex);
    try {
        return this->sessions.at(session_id);
    } catch(out_of_range) {
        return this->guest_session;
    }
}

Session::Session(bool constant) : constant(constant)
{
}

json Session::answer_api1(HTTPCallback &cb, vector< string >::const_iterator path_begin, vector< string >::const_iterator path_end)
{
    if (path_begin != path_end && *path_begin == "workset") {
        path_begin++;
        assert_or_throw< SendError >(path_begin != path_end, 404);
        if (*path_begin == "create") {
            assert_or_throw< SendError >(!this->is_constant(), 403);
            path_begin++;
            assert_or_throw< SendError >(path_begin == path_end, 404);
            auto res = this->create_workset();
            json ret = { { "id", res.first } };
            return ret;
        } else if (*path_begin == "list") {
            path_begin++;
            assert_or_throw< SendError >(path_begin == path_end, 404);
            json ret = { { "worksets", this->json_list_worksets() } };
            return ret;
        } else {
            size_t id = safe_stoi(*path_begin);
            path_begin++;
            shared_ptr< Workset > workset;
            try {
                workset = this->get_workset(id);
            } catch (out_of_range) {
                throw SendError(404);
            }
            return workset->answer_api1(cb, path_begin, path_end);
        }
    }
    throw SendError(404);
}

bool Session::is_constant() {
    return this->constant;
}

std::pair<size_t, std::shared_ptr<Workset> > Session::create_workset()
{
    unique_lock< mutex > lock(this->worksets_mutex);
    size_t id = this->worksets.size();
    auto workset = Workset::create();
    workset->set_name("Workset " + to_string(id + 1));
    this->worksets.push_back(workset);

    return { id, this->worksets.at(id) };
}

std::shared_ptr<Workset> Session::get_workset(size_t id)
{
    unique_lock< mutex > lock(this->worksets_mutex);
    return this->worksets.at(id);
}

json Session::json_list_worksets()
{
    unique_lock< mutex > lock(this->worksets_mutex);
    json ret;
    for (size_t i = 0; i < this->worksets.size(); i++) {
        json tmp;
        tmp["id"] = i;
        tmp["name"] = this->worksets.at(i)->get_name();
        ret.push_back(tmp);
    }
    return ret;
}

int safe_stoi(const string &s)
{
    try {
        return stoi(s);
    } catch (std::invalid_argument) {
        throw SendError(404);
    } catch (std::out_of_range) {
        throw SendError(404);
    }
}
