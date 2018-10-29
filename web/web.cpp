
#include <iostream>
#include <random>

#include <boost/tokenizer.hpp>
#include <boost/filesystem/fstream.hpp>

#include <giolib/static_block.h>
#include <giolib/main.h>

#if defined(USE_MICROHTTPD)
#include "httpd_microhttpd.h"
#endif

#include "mm/reader.h"
#include "memory.h"
#include "utils/utils.h"
#include "web.h"
#include "platform.h"

const bool SERVE_STATIC_FILES = true;
const bool PUBLICLY_SERVE_STATIC_FILES = true;

std::mt19937 rand_mt;

void init_random() {
    std::random_device rand_dev;
    rand_mt.seed(rand_dev());
}

std::string generate_id() {
    int len = 100;
    std::vector< char > id(len);
    std::uniform_int_distribution< int > dist('a', 'z');
    for (int i = 0; i < len; i++) {
        id[i] = static_cast< char >(dist(rand_mt));
    }
    return std::string(id.begin(), id.end());
}

std::unique_ptr< HTTPD > make_server(int port, WebEndpoint &endpoint, bool open_server) {
#if defined(USE_MICROHTTPD)
    return std::make_unique< HTTPD_microhttpd >(port, endpoint, !open_server);
#else
    // If no HTTP implementation is provided, we return nullptr
    (void) port;
    (void) endpoint;
    (void) open_server;
    return nullptr;
#endif
}

enum class ServerType {
    USER,
    OPEN,
    DOCKER,
};

int webmmpp_main_common(int argc, char *argv[], ServerType type) {
    if (!platform_webmmpp_init(argc, argv)) {
        return 1;
    }

    init_random();

    int port = 8888;
    WebEndpoint endpoint(port, type == ServerType::OPEN);
    std::unique_ptr< HTTPD > httpd = make_server(port, endpoint, type == ServerType::OPEN || type == ServerType::DOCKER);
    if (httpd == nullptr) {
        std::cerr << "Could not build an HTTP server, exiting" << std::endl;
        return 1;
    }

    if (type == ServerType::OPEN) {
        // The session is already available, but it is constant; we need to populate it with some content
        std::shared_ptr< Session > session = endpoint.get_guest_session();
        std::shared_ptr< Workset > workset;
        std::tie(std::ignore, workset) = session->create_workset();
        workset->set_name("Default workset");
        workset->load_library(platform_get_resources_base() / "set.mm", platform_get_resources_base() / "set.mm.cache", "|-");
    }

    httpd->start();

    if (type == ServerType::DOCKER || type == ServerType::USER) {
        // Generate a session and pass it to the browser
        std::string ticket_id = endpoint.create_session_and_ticket();
        if (type == ServerType::USER) {
            std::string browser_url = "http://127.0.0.1:" + std::to_string(port) + "/ticket/" + ticket_id;
            platform_open_browser(browser_url);
            std::cout << "A browser session was spawned; if you cannot see it, go to " << browser_url << std::endl;
        } else {
            std::string browser_url = "http://DOCKER_ADDRESS:" + std::to_string(port) + "/ticket/" + ticket_id;
            std::cout << "Open a browser and go to " << browser_url << std::endl;
        }
    }

    auto new_session_callback = [&endpoint,port]() {
        std::string ticket_id = endpoint.create_session_and_ticket();
        std::string browser_url = "http://127.0.0.1:" + std::to_string(port) + "/ticket/" + ticket_id;
        std::cout << "New session created; please visit " << browser_url << std::endl;
    };

    platform_webmmpp_main_loop(new_session_callback);
    std::cerr << "Stopping webserver..." << std::endl;
    httpd->stop();
    httpd->join();
    std::cerr << "Webserver stopped, will now exit" << std::endl;

    return 0;
}

int webmmpp_main(int argc, char*argv[]) {
    return webmmpp_main_common(argc, argv, ServerType::USER);
}
gio_static_block {
    gio::register_main_function("webmmpp", webmmpp_main);
}

int webmmpp_open_main(int argc, char*argv[]) {
    return webmmpp_main_common(argc, argv, ServerType::OPEN);
}
gio_static_block {
    gio::register_main_function("webmmpp_open", webmmpp_open_main);
}

int webmmpp_docker_main(int argc, char*argv[]) {
    return webmmpp_main_common(argc, argv, ServerType::DOCKER);
}
gio_static_block {
    gio::register_main_function("webmmpp_docker", webmmpp_docker_main);
}

WebEndpoint::WebEndpoint(int port, bool enable_guest_session) :
    port(port), guest_session(enable_guest_session ? Session::create(true) : nullptr)
{
}

std::vector< std::pair< std::string, std::string > > content_types = {
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
std::string guess_content_type(std::string url) {
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
    std::string cookie_name = "mmpp_session_id";
    std::string ticket_url = "/ticket/";
    const std::string &url = cb.get_url();
    const std::string &method = cb.get_method();
    if (method == "GET" && starts_with(url, ticket_url)) {
        std::unique_lock< std::mutex > lock(this->sessions_mutex);
        std::string ticket = std::string(url.begin() + ticket_url.size(), url.end());
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

    // Check auth cookie and recover session; at this point we allow a nullptr session if we are publicly serving static files
    std::string session_cookie;
    try {
        session_cookie = cb.get_cookies().at(cookie_name);
    } catch (std::out_of_range&) {
        session_cookie = "";
    }
    std::shared_ptr< Session > session = this->get_session(session_cookie);
    if (!PUBLICLY_SERVE_STATIC_FILES && session == nullptr) {
        cb.set_status_code(403);
        cb.add_header("Content-Type", "text/plain");
        cb.set_answer("403 Forbidden");
        return;
    }

    // Serve static files
    // TODO This is not the greatest static file server ever...
    if (SERVE_STATIC_FILES) {
        std::string static_url = "/static/";
        boost::filesystem::path static_dir = boost::filesystem::canonical(platform_get_resources_base() / "static");
        if (method == "GET" && starts_with(url, static_url)) {
            boost::filesystem::path filename = platform_get_resources_base() / std::string(url.begin(), url.end());
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
            auto infile = std::make_shared< boost::filesystem::ifstream >(filename, std::ios::binary);
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
            cb.set_answerer(std::make_unique< HTTPFileAnswerer >(infile));
            return;
        }
    }

    // Check again the session: if you do not have a valid one, you cannot go past here
    if (session == nullptr) {
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
    std::string api_version_url = "/api/version";
    if (url == api_version_url) {
        nlohmann::json res;
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
        std::string api_create_ticket = "/api/create_ticket";
        if (url == api_create_ticket) {
            cb.add_header("Content-Type", "text/plain");
            cb.set_status_code(200);
            std::string ticket_id = this->create_session_and_ticket();
            std::string browser_url = "http://127.0.0.1:" + std::to_string(this->port) + "/ticket/" + ticket_id;
            cb.set_answer(move(browser_url));
            return;
        }
    }

    // Serve API requests
    std::string api_url = "/api/1/";
    if (starts_with(url, api_url)) {
        boost::char_separator< char > sep("/");
        boost::tokenizer< boost::char_separator< char > > tokens(url.begin() + api_url.size(), url.end(), sep);
        const std::vector< std::string > path(tokens.begin(), tokens.end());
        try {
            nlohmann::json res = session->answer_api1(cb, path.begin(), path.end());
            cb.add_header("Content-Type", "application/json");
            cb.set_status_code(200);
            cb.set_answer(res.dump());
            return;
        } catch (SendError se) {
            cb.add_header("Content-Type", "text/plain");
            cb.set_status_code(se.get_status_code());
            cb.set_answer(std::to_string(se.get_status_code()) + " " + se.get_descr());
            return;
        } catch (WaitForPost wfp) {
            auto callback = [wfp,&cb] (const auto &post_data) {
                try {
                    nlohmann::json res = wfp.get_callback()(post_data);
                    cb.add_header("Content-Type", "application/json");
                    cb.set_status_code(200);
                    cb.set_answer(res.dump());
                } catch (SendError se) {
                    cb.add_header("Content-Type", "text/plain");
                    cb.set_status_code(se.get_status_code());
                    cb.set_answer(std::to_string(se.get_status_code()) + " " + se.get_descr());
                }
            };
            std::function< void(const std::unordered_map< std::string, PostItem >&) > cb2 = callback;
            cb.set_post_iterator(std::make_unique< HTTPSimplePostIterator >(cb2));
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

std::string WebEndpoint::create_session_and_ticket()
{
    std::unique_lock< std::mutex > lock(this->sessions_mutex);
    std::string session_id = generate_id();
    std::string ticket_id = generate_id();
    this->session_tickets[ticket_id] = session_id;
    this->sessions[session_id] = Session::create();
    return ticket_id;
}

std::shared_ptr< Session > WebEndpoint::get_session(std::string session_id)
{
    std::unique_lock< std::mutex > lock(this->sessions_mutex);
    try {
        return this->sessions.at(session_id);
    } catch(std::out_of_range&) {
        return this->guest_session;
    }
}

Session::Session(bool constant) : constant(constant), new_id(0)
{
}

nlohmann::json Session::answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end)
{
    if (path_begin != path_end && *path_begin == "workset") {
        path_begin++;
        gio::assert_or_throw< SendError >(path_begin != path_end, 404);
        if (*path_begin == "create") {
            gio::assert_or_throw< SendError >(!this->is_constant(), 403);
            path_begin++;
            gio::assert_or_throw< SendError >(path_begin == path_end, 404);
            auto res = this->create_workset();
            nlohmann::json ret = { { "id", res.first } };
            return ret;
        } else if (*path_begin == "list") {
            path_begin++;
            gio::assert_or_throw< SendError >(path_begin == path_end, 404);
            nlohmann::json ret = { { "worksets", this->json_list_worksets() } };
            return ret;
        } else {
            size_t id = safe_stoi(*path_begin);
            path_begin++;
            std::shared_ptr< Workset > workset;
            try {
                workset = this->get_workset(id);
            } catch (std::out_of_range&) {
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
    std::unique_lock< std::mutex > lock(this->worksets_mutex);
    size_t id = this->new_id;
    this->new_id++;
    auto workset = Workset::create(this->weak_from_this());
    workset->set_name("Workset " + std::to_string(id + 1));
    this->worksets[id] = workset;
    return { id, this->worksets.at(id) };
}

bool Session::destroy_workset(std::shared_ptr<Workset> workset)
{
    std::unique_lock< std::mutex > lock(this->worksets_mutex);
    auto it = find_if(this->worksets.begin(), this->worksets.end(), [&workset](const auto &x) { return x.second == workset; });
    if (it != this->worksets.end()) {
        this->worksets.erase(it);
        return true;
    } else {
        return false;
    }
}

std::shared_ptr<Workset> Session::get_workset(size_t id)
{
    std::unique_lock< std::mutex > lock(this->worksets_mutex);
    return this->worksets.at(id);
}

nlohmann::json Session::json_list_worksets()
{
    std::unique_lock< std::mutex > lock(this->worksets_mutex);
    nlohmann::json ret;
    for (size_t i = 0; i < this->worksets.size(); i++) {
        nlohmann::json tmp;
        tmp["id"] = i;
        tmp["name"] = this->worksets.at(i)->get_name();
        ret.push_back(tmp);
    }
    return ret;
}

int safe_stoi(const std::string &s)
{
    try {
        return std::stoi(s);
    } catch (std::invalid_argument&) {
        throw SendError(404);
    } catch (std::out_of_range&) {
        throw SendError(404);
    }
}
