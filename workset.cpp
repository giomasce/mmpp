#include "workset.h"

#include <regex>

#include "json.h"

#include "parser.h"
#include "platform.h"

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

Workset::Workset()
{
}

json Workset::answer_api1(HTTPCallback &cb, std::vector< std::string >::const_iterator path_begin, std::vector< std::string >::const_iterator path_end, std::string method)
{
    (void) cb;
    (void) method;
    unique_lock< mutex > lock(this->global_mutex);
    if (path_begin == path_end) {
        json ret = json::object();
        return ret;
    }
    if (*path_begin == "load") {
        FileTokenizer ft(platform_get_resources_base() + "/library.mm");
        Parser p(ft, false, true);
        p.run();
        this->library = make_unique< LibraryImpl >(p.get_library());
        json ret = { { "status", "ok" } };
        return ret;
    } else if (*path_begin == "get_context") {
        json ret;
        if (this->library == NULL) {
            ret["status"] = "unloaded";
            return ret;
        }
        ret["status"] = "loaded";
        ret["symbols"] = this->library->get_symbols_cache();
        ret["labels"] = this->library->get_labels_cache();
        const auto &addendum = this->library->get_addendum();
        ret["htmldefs"] = fix_htmldefs_for_web(addendum.htmldefs);
        ret["althtmldefs"] = addendum.althtmldefs;
        ret["latexdef"] = addendum.latexdefs;
        ret["htmlcss"] = fix_htmlcss_for_web(addendum.htmlcss);
        ret["htmlfont"] = addendum.htmlfont;
        ret["htmltitle"] = addendum.htmltitle;
        ret["htmlhome"] = addendum.htmlhome;
        ret["htmlbibliography"] = addendum.htmlbibliography;
        ret["exthtmltitle"] = addendum.exthtmltitle;
        ret["exthtmlhome"] = addendum.exthtmlhome;
        ret["exthtmllabel"] = addendum.exthtmllabel;
        ret["exthtmlbibliography"] = addendum.exthtmlbibliography;
        ret["htmlvarcolor"] = addendum.htmlvarcolor;
        ret["htmldir"] = addendum.htmldir;
        ret["althtmldir"] = addendum.althtmldir;
        return ret;
    } else if (*path_begin == "get_sentence") {
        path_begin++;
        if (path_begin == path_end) {
            throw SendError(404);
        }
        int tok = safe_stoi(*path_begin);
        try {
            const Sentence &sent = this->library->get_sentence(tok);
            json ret;
            ret["sentence"] = sent;
            return ret;
        } catch (out_of_range e) {
            (void) e;
            throw SendError(404);
        }
    }
    throw SendError(404);
}
