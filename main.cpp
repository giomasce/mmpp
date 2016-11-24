
#include <iostream>
#include <fstream>

#include "wff.h"
#include "parser.h"

using namespace std;

int main() {

    auto ph = pwff(new Var("ph"));
    auto ps = pwff(new Var("ps"));
    auto w = pwff(new Nand(ph, ps));
    auto w2 = pwff(new Xor(w, ph));

    cout << w2->to_string() << endl;
    cout << w2->imp_not_form()->to_string() << endl;

    CompressedDecoder cd;
    string test[] = { "A", "B", "T", "UA", "UB", "UT", "VA", "VB", "YT", "UUA", "YYT", "UUUA", "UUUAZ" };
    for (auto &str : test) {
        cout << "Testing " << str << ":";
        for (auto &c : str) {
            cout << " " << c << "->" << cd.push_char(c);
        }
        cout << endl;
    }

    {
        fstream in("/home/giovanni/progetti/metamath/set.mm/set.mm");
        FileTokenizer ft(in);
        Parser p(ft);
        p.run();
        Library lib = p.get_library();
        cout << lib.get_symbol_num() << " symbols and " << lib.get_label_num() << " labels" << endl;
    }

    {
        fstream in("/home/giovanni/progetti/metamath/demo0.mm");
        FileTokenizer ft(in);
        Parser p(ft);
        p.run();
        Library lib = p.get_library();
        cout << lib.get_symbol_num() << " symbols and " << lib.get_label_num() << " labels" << endl;
    }

    return 0;

}
