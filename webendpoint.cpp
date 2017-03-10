
#include <json/json.h>

#include "webendpoint.h"

using namespace std;

WebEndpoint::WebEndpoint(const Library &lib) : lib(lib)
{
}

string WebEndpoint::answer()
{
    Json::Value res;
    for (SymTok tok = 1; tok < this->lib.get_symbols_num(); tok++) {
        res["symbols"][tok][0] = this->lib.resolve_symbol(tok);
        res["symbols"][tok][1] = this->lib.get_addendum().althtmldefs[tok];
    }
    Json::FastWriter writer;
    return writer.write(res);
}
