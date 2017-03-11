
#include <json/json.h>

#include <atomic>
#include <iostream>
#include <csignal>
#include <random>

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

string WebEndpoint::answer(HTTPCallback &cb, string url, string method, string version)
{
    // Receive session ticket
    string cookie_name = "mmpp_session_id";
    string ticket_url = "/ticket/";
    if (method == "GET" && equal(ticket_url.begin(), ticket_url.end(), url.begin())) {
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
    if (cb.get_cookies().find(cookie_name) == cb.get_cookies().end()
            || this->sessions.find(cb.get_cookies().at(cookie_name)) == this->sessions.end()) {
        cb.set_status_code(403);
        cb.add_header("Content-Type", "text/plain");
        return "403 Forbidden";
    }
    Session &session = this->sessions.at(cb.get_cookies().at(cookie_name));

    cb.add_header("Content-Type", "application/json");
    Json::Value res;
    /*for (SymTok tok = 1; tok < this->lib.get_symbols_num(); tok++) {
        res["symbols"][tok][0] = this->lib.resolve_symbol(tok);
        //res["symbols"][tok][1] = this->lib.get_addendum().althtmldefs[tok];
    }*/
    Json::FastWriter writer;
    return writer.write(res);
}

string WebEndpoint::create_session_and_ticket()
{
    string session_id = generate_id();
    string ticket_id = generate_id();
    this->session_tickets[ticket_id] = session_id;
    this->sessions[session_id] = Session();
    return ticket_id;
}

Session::Session()
{
}
