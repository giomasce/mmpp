
#include <unordered_map>
#include <utility>
#include <iostream>

#include "test.h"

#include "unif.h"

using namespace std;

void test_tree_unification() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;

    std::function< bool(LabTok) > is_var = [&lib](LabTok x)->bool { return !lib.is_constant(lib.get_sentence(x).at(1)); };

    auto pt_templ = tb.parse_sentence(tb.read_sentence("( ps -> ph )"), lib.get_symbol("wff"));
    auto pt_sent = tb.parse_sentence(tb.read_sentence("( ph -> ( ps -> ch ) )"), lib.get_symbol("wff"));

    bool res;
    unordered_map< LabTok, ParsingTree< SymTok, LabTok > > subst;
    tie(res, subst) = unify(pt_templ, pt_sent, is_var);

    for (auto &i : subst) {
        cout << lib.resolve_symbol(lib.get_sentence(i.first).at(1)) << ": " << tb.print_sentence(tb.reconstruct_sentence(i.second)) << endl;
    }
}
