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

    LabTok imp_lab = tb.get_label("wi");
    LabTok ph_lab = tb.get_label("wph");

    std::map< SymTok, std::pair< LabTok, LabTok > > equalities = {
        { tb.get_symbol("wff"), { tb.get_label("wb"), tb.get_label("biid") } },
        { tb.get_symbol("class"), { tb.get_label("wceq"), tb.get_label("eqid") } },
        //{ tb.get_symbol("set"), { tb.get_label(""), tb.get_label("") } },
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
            for (const auto sym : rule) {
                if (ders.find(sym) != ders.end()) {
                    has_vars = true;
                }
            }
            if (!has_vars) {
                continue;
            }
            std::cout << "Considering derivation " << tb.resolve_label(label) <<" for type " << tb.resolve_symbol(type) << ", with variables:";
            for (const auto sym : rule) {
                if (ders.find(sym) != ders.end()) {
                    std::cout << " " << tb.resolve_symbol(sym);
                }
            }
            std::cout << std::endl;

            for (unsigned i = 0; i < rule.size(); i++) {
                auto it = equalities.find(rule[i]);
                if (it != equalities.end()) {
                    bool found = false;
                    bool found_strong = false;
                    std::cout << " * Search for a substitution rule for " << tb.resolve_symbol(it->first) << " in position " << i << std::endl;
                    tb.new_temp_var_frame();
                    ParsingTree< SymTok, LabTok > pt_hyp;
                    pt_hyp.label = it->second.first;
                    pt_hyp.type = tb.get_turnstile_alias();
                    ParsingTree< SymTok, LabTok > pt_left;
                    ParsingTree< SymTok, LabTok > pt_right;
                    pt_left.label = label;
                    pt_left.type = type;
                    pt_right.label = label;
                    pt_right.type = type;
                    for (unsigned j = 0; j < rule.size(); j++) {
                        if (ders.find(rule[j]) != ders.end()) {
                            ParsingTree< SymTok, LabTok > pt_var1;
                            ParsingTree< SymTok, LabTok > pt_var2;
                            auto var1 = tb.new_temp_var(rule[j]);
                            pt_var1.label = var1.first;
                            pt_var1.type = rule[j];
                            if (i == j) {
                                auto var2 = tb.new_temp_var(rule[j]);
                                pt_var2.label = var2.first;
                                pt_var2.type = rule[j];
                                pt_hyp.children.push_back(pt_var1);
                                pt_hyp.children.push_back(pt_var2);
                            } else {
                                pt_var2 = pt_var1;
                            }
                            pt_left.children.push_back(pt_var1);
                            pt_right.children.push_back(pt_var2);
                        }
                    }
                    ParsingTree< SymTok, LabTok > pt_thesis;
                    pt_thesis.label = equalities[type].first;
                    pt_thesis.type = tb.get_turnstile_alias();
                    pt_thesis.children.push_back(pt_left);
                    pt_thesis.children.push_back(pt_right);

                    assert(pt_thesis.validate(tb.get_validation_rule()));
                    assert(pt_hyp.validate(tb.get_validation_rule()));
                    std::cout << "   Inference form: searching for " << tb.print_sentence(pt_thesis) << " with hypothesis " << tb.print_sentence(pt_hyp) << std::endl;
                    auto res = tb.unify_assertion({std::make_pair(tb.get_turnstile(), pt_hyp)}, std::make_pair(tb.get_turnstile(), pt_thesis));
                    if (!res.empty()) {
                        std::cout << "     Found match " << tb.resolve_label(std::get<0>(res[0])) << std::endl;
                        found = true;
                    } else {
                        std::cout << "     Found NO match..." << std::endl;
                    }

                    ParsingTree< SymTok, LabTok > pt_thm;
                    pt_thm.type = tb.get_turnstile_alias();
                    pt_thm.label = imp_lab;
                    pt_thm.children.push_back(pt_hyp);
                    pt_thm.children.push_back(pt_thesis);
                    assert(pt_thm.validate(tb.get_validation_rule()));
                    std::cout << "   Inference form: searching for " << tb.print_sentence(pt_thm) << std::endl;
                    res = tb.unify_assertion({}, std::make_pair(tb.get_turnstile(), pt_thm));
                    if (!res.empty()) {
                        std::cout << "     Found match " << tb.resolve_label(std::get<0>(res[0])) << std::endl;
                        found = true;
                        found_strong = true;
                    } else {
                        std::cout << "     Found NO match..." << std::endl;
                    }

                    ParsingTree< SymTok, LabTok > pt_ph;
                    pt_ph.type = tb.get_turnstile_alias();
                    pt_ph.label = ph_lab;
                    ParsingTree< SymTok, LabTok > pt_hypd;
                    pt_hypd.type = tb.get_turnstile_alias();
                    pt_hypd.label = imp_lab;
                    pt_hypd.children.push_back(pt_ph);
                    pt_hypd.children.push_back(pt_hyp);
                    ParsingTree< SymTok, LabTok > pt_thesisd;
                    pt_thesisd.type = tb.get_turnstile_alias();
                    pt_thesisd.label = imp_lab;
                    pt_thesisd.children.push_back(pt_ph);
                    pt_thesisd.children.push_back(pt_thesis);
                    assert(pt_hypd.validate(tb.get_validation_rule()));
                    assert(pt_thesisd.validate(tb.get_validation_rule()));
                    std::cout << "   Deduction form: searching for " << tb.print_sentence(pt_thesisd) << " with hypothesis " << tb.print_sentence(pt_hypd) << std::endl;
                    res = tb.unify_assertion({std::make_pair(tb.get_turnstile(), pt_hypd)}, std::make_pair(tb.get_turnstile(), pt_thesisd));
                    if (!res.empty()) {
                        std::cout << "     Found match " << tb.resolve_label(std::get<0>(res[0])) << std::endl;
                        found = true;
                        found_strong = true;
                    } else {
                        std::cout << "     Found NO match..." << std::endl;
                    }

                    if (!found) {
                        std::cout << "   NO RULE FOUND" << std::endl;
                    } else if (!found_strong) {
                        std::cout << "   ONLY WEAK RULES WERE FOUND" << std::endl;
                    } else {
                        std::cout << "   Found strong rules!" << std::endl;
                    }

                    tb.release_temp_var_frame();
                }
            }
        }
    }

    return 0;
}
static_block {
    register_main_function("subst_search", subst_search_main);
}
