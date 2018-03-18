
#include <iostream>

#include "mm/reader.h"
#include "mm/library.h"
#include "utils/utils.h"
#include "mm/toolbox.h"
#include "platform.h"
#include "mm/setmm.h"

void unification_loop() {
    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;

    while (true) {
        std::string line;
        std::getline(std::cin, line);
        if (line == "") {
            break;
        }
        size_t dollar_pos;
        std::vector< std::vector< SymTok > > hypotheses;
        while ((dollar_pos = line.find("$")) != std::string::npos) {
            hypotheses.push_back(tb.read_sentence(line.substr(0, dollar_pos)));
            line = line.substr(dollar_pos+1);
        }
        std::vector< SymTok > sent = tb.read_sentence(line);
        /*auto sent_wff = sent;
        sent_wff[0] = lib.get_symbol("wff");
        auto res = lib.prove_type(sent_wff);
        cout << "Found type proof:";
        for (auto &label : res) {
            cout << " " << lib.resolve_label(label);
        }
        cout << endl;*/
        auto res2 = tb.unify_assertion(hypotheses, sent);
        std::cout << "Found " << res2.size() << " matching assertions:" << std::endl;
        for (auto &match : res2) {
            auto &label = std::get<0>(match);
            const Assertion &ass = lib.get_assertion(label);
            std::cout << " * " << lib.resolve_label(label) << ":";
            for (auto &hyp : ass.get_ess_hyps()) {
                auto &hyp_sent = lib.get_sentence(hyp);
                std::cout << " & " << tb.print_sentence(hyp_sent, SentencePrinter::STYLE_ANSI_COLORS_SET_MM);
            }
            auto &thesis_sent = lib.get_sentence(ass.get_thesis());
            std::cout << " => " << tb.print_sentence(thesis_sent, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << std::endl;
        }
    }
}

int unification_loop_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    unification_loop();
    return 0;
}
static_block {
    register_main_function("unificator", unification_loop_main);
}
