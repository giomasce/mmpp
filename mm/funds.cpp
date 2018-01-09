
#include "funds.h"

using namespace std;

void collect_variables(const Sentence &sent, const std::function<bool (SymTok)> &is_var, std::set<SymTok> &vars) {
    for (const auto tok : sent) {
        if (is_var(tok)) {
            vars.insert(tok);
        }
    }
}
