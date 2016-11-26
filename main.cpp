
#include <iostream>
#include <fstream>

#include "wff.h"
#include "parser.h"
#include "unification.h"

using namespace std;

int main() {

    if (false) {
        auto ph = pwff(new Var("ph"));
        auto ps = pwff(new Var("ps"));
        auto w = pwff(new Nand(ph, ps));
        auto w2 = pwff(new Xor(w, ph));

        cout << w2->to_string() << endl;
        cout << w2->imp_not_form()->to_string() << endl;

        CompressedDecoder cd;
        string test_enc[] = { "A", "B", "T", "UA", "UB", "UT", "VA", "VB", "YT", "UUA", "YYT", "UUUA", "UUUAZ" };
        int test_dec[] = { 1, 2, 20, 21, 22, 40, 41, 42, 120, 121, 620, 621, 0 };
        for (auto &str : test_enc) {
            cout << "Testing " << str << ":";
            for (auto &c : str) {
                cout << " " << c << "->" << cd.push_char(c);
            }
            cout << endl;
        }
        CompressedEncoder ce;
        for (auto &val : test_dec) {
            cout << "Testing " << val << ": " << ce.push_code(val) << endl;
        }
    }

    if (false) {
        fstream in("/home/giovanni/progetti/metamath/set.mm/set.mm");
        FileTokenizer ft(in);
        Parser p(ft);
        p.run();
        Library lib = p.get_library();
        cout << lib.get_symbol_num() << " symbols and " << lib.get_label_num() << " labels" << endl;
    }

    if (true) {
        fstream in("/home/giovanni/progetti/metamath/demo0.mm");
        FileTokenizer ft(in);
        Parser p(ft);
        p.run();
        Library lib = p.get_library();
        cout << lib.get_symbol_num() << " symbols and " << lib.get_label_num() << " labels" << endl;

        vector< SymTok > sent = parse_sentence("t + 0", lib);
        vector< SymTok > templ = parse_sentence("t + P", lib);
        auto res = unify(sent, templ, lib, false);
        cout << "Matching:         " << print_sentence(sent, lib) << endl << "against template: " << print_sentence(templ, lib) << endl;
        for (auto &match : res) {
            cout << "  *";
            for (auto &var: match) {
                cout << " " << print_sentence({var.first}, lib) << " => " << print_sentence(var.second, lib) << "  ";
            }
            cout << endl;
        }
    }

    return 0;

}
