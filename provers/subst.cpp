#include "subst.h"

#include "test/test_env.h"
#include "utils/utils.h"

int subst_search_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto &ders = tb.get_derivations();

    std::map< SymTok, std::pair< LabTok, LabTok > > equalities = {
        { tb.get_symbol("wff"), { tb.get_label("wb"), tb.get_label("biid") } },
        { tb.get_symbol("set"), { tb.get_label("wceq"), tb.get_label("eqid") } },
        { tb.get_symbol("class"), { tb.get_label(""), tb.get_label("") } },
    };

    // Find an equality symbol for each type
    /*for (const auto &der : ders) {
        auto type = der.first;
        std::cout << "Searching for a reflexive equality theorem for " << tb.resolve_symbol(type) << std::endl;
        auto ass_list = tb.list_assertions();
        const Assertion *ass;
        while ((ass = ass_list()) != nullptr) {
            if (ass->get_ess_hyps().size() != 0) {
                continue;
            }
            if (ass->get_float_hyps().size() != 1) {
                continue;
            }
            const auto &float_hyp = tb.get_parsed_sents().at(ass->get_float_hyps()[0]);
            assert(float_hyp.children.empty());
            const auto var_lab = float_hyp.label;
            if (tb.get_var_lab_to_type_sym(var_lab) != type) {
                continue;
            }
            const auto &pt = tb.get_parsed_sents().at(ass->get_thesis());
            const auto &sent = tb.get_sentence(ass->get_thesis());
            if (sent[0] != tb.get_turnstile()) {
                continue;
            }
            if (pt.children.size() != 2 || pt.children[0] != float_hyp || pt.children[1] != float_hyp) {
                continue;
            }
            std::cout << "  Found theorem " << tb.resolve_label(ass->get_thesis()) << std::endl;
        }
    }*/

    for (const auto &der : ders) {
        const auto type = der.first;
        for (const auto &der2 : der.second) {
            const auto label = der2.first;
            const auto &rule = der2.second;
            bool has_vars = false;
            bool has_set_vars = false;
            for (const auto sym : rule) {
                if (sym == tb.get_symbol("set")) {
                    has_set_vars = true;
                }
                if (ders.find(sym) != ders.end()) {
                    has_vars = true;
                }
            }
            if (!has_vars) {
                continue;
            }
            std::cout << "Considering derivation " << tb.resolve_label(label) <<" for type " << tb.resolve_symbol(type) << std::endl;
            if (has_set_vars) {
                std::cout << "  HAS SET VARS" << std::endl;
            }
        }
    }

    return 0;
}
static_block {
    register_main_function("subst_search", subst_search_main);
}
