
#include <json/json.h>

#include <atomic>
#include <iostream>
#include <csignal>

#include "parser.h"
#include "memory.h"
#include "utils.h"
#include "web.h"

using namespace std;

atomic< bool > signalled;
void int_handler(int signal) {
    (void) signal;
    signalled = true;
}

int httpd_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    cout << "Reading set.mm..." << endl;
    FileTokenizer ft("../set.mm/set.mm");
    //FileTokenizer ft("../metamath/ql.mm");
    Parser p(ft, false, true);
    p.run();
    LibraryImpl lib = p.get_library();
    //LibraryToolbox tb(lib, true);
    cout << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << endl;
    cout << "Memory usage after loading the library: " << size_to_string(getCurrentRSS()) << endl;

    WebEndpoint endpoint(lib);

    HTTPD_microhttpd httpd(8888, endpoint);

    struct sigaction act;
    act.sa_handler = int_handler;
    act.sa_flags = 0;
    sigfillset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    httpd.start();
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

WebEndpoint::WebEndpoint(const Library &lib) : lib(lib)
{
}

string WebEndpoint::answer(HTTPCallback &cb, string url, string method, string version)
{
    Json::Value res;
    for (SymTok tok = 1; tok < this->lib.get_symbols_num(); tok++) {
        res["symbols"][tok][0] = this->lib.resolve_symbol(tok);
        //res["symbols"][tok][1] = this->lib.get_addendum().althtmldefs[tok];
    }
    Json::FastWriter writer;
    return writer.write(res);
}
