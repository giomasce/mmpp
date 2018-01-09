
#include <string>
#include <iostream>
#include <vector>

#include "test/test_minor.h"

#include "provers/wff.h"
#include "mm/reader.h"
#include "utils/utils.h"
#include "platform.h"

using namespace std;

void test_small_stuff() {
    cout << "Testing random small stuff..." << endl;
    auto ph = pwff(new Var("ph"));
    auto ps = pwff(new Var("ps"));
    auto w = pwff(new Nand(ph, ps));
    auto w2 = pwff(new Xor(w, pwff(new And(pwff(new True()), ph))));

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

    cout << "Finished. Memory usage: " << size_to_string(platform_get_current_rss()) << endl << endl;
}
