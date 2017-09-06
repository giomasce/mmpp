
#include <regex>

#include "jsonize.h"

using namespace std;
using namespace nlohmann;

string fix_htmlcss_for_web(string s)
{
    string tmp(s);
    tmp = tmp + "\n\n";
    // We need to fix the link to the WOFF
    tmp = regex_replace(tmp, regex("xits-math.woff"), "woff/xits-math.woff");
    // We are not providing the additional stylesheets at the moment, so let us avoid a couple of errors
    tmp = regex_replace(tmp, regex("<LINK [^>]*>"), "");
    return tmp;
}

vector< string > fix_htmldefs_for_web(const vector< string > &htmldefs) {
    vector< string > ret;
    for (const auto &elem : htmldefs) {
        ret.push_back(regex_replace(elem, regex("SRC='"), "SRC='images/symbols/"));
    }
    return ret;
}

json jsonize(const ExtendedLibraryAddendum &addendum)
{
    json ret;
    ret["htmldefs"] = fix_htmldefs_for_web(addendum.get_htmldefs());
    ret["althtmldefs"] = addendum.get_althtmldefs();
    ret["latexdefs"] = addendum.get_latexdefs();
    ret["htmlcss"] = fix_htmlcss_for_web(addendum.get_htmlcss());
    ret["htmlfont"] = addendum.get_htmlfont();
    ret["htmltitle"] = addendum.get_htmltitle();
    ret["htmlhome"] = addendum.get_htmlhome();
    ret["htmlbibliography"] = addendum.get_htmlbibliography();
    ret["exthtmltitle"] = addendum.get_exthtmltitle();
    ret["exthtmlhome"] = addendum.get_exthtmlhome();
    ret["exthtmllabel"] = addendum.get_exthtmllabel();
    ret["exthtmlbibliography"] = addendum.get_exthtmlbibliography();
    ret["htmlvarcolor"] = addendum.get_htmlvarcolor();
    ret["htmldir"] = addendum.get_htmldir();
    ret["althtmldir"] = addendum.get_althtmldir();
    return ret;
}
