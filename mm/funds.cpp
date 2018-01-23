
#include "funds.h"

using namespace std;

void collect_variables(const Sentence &sent, const std::function<bool (SymTok)> &is_var, std::set<SymTok> &vars) {
    for (const auto tok : sent) {
        if (is_var(tok)) {
            vars.insert(tok);
        }
    }
}

Sentence substitute(const Sentence &orig, const std::unordered_map<SymTok, std::vector<SymTok> > &subst_map, const std::function< bool(SymTok) > &is_var)
{
    vector< SymTok > ret;
    for (auto it = orig.begin(); it != orig.end(); it++) {
        const SymTok &tok = *it;
        if (!is_var(tok)) {
            ret.push_back(tok);
        } else {
            const vector< SymTok > &subst = subst_map.at(tok);
            copy(subst.begin(), subst.end(), back_inserter(ret));
        }
    }
    return ret;
}
