
#include <string>
#include <iostream>
#include <vector>

#include "test/test_minor.h"

#include "provers/wff.h"
#include "mm/reader.h"
#include "utils/utils.h"
#include "platform.h"

void test_small_stuff() {
    std::cout << "Testing random small stuff..." << std::endl;

    CompressedDecoder cd;
    std::string test_enc[] = { "A", "B", "T", "UA", "UB", "UT", "VA", "VB", "YT", "UUA", "YYT", "UUUA", "UUUAZ" };
    int test_dec[] = { 1, 2, 20, 21, 22, 40, 41, 42, 120, 121, 620, 621, 0 };
    for (auto &str : test_enc) {
        std::cout << "Testing " << str << ":";
        for (auto &c : str) {
            std::cout << " " << c << "->" << cd.push_char(c).val();
        }
        std::cout << std::endl;
    }
    CompressedEncoder ce;
    for (auto &val : test_dec) {
        std::cout << "Testing " << val << ": " << ce.push_code(CodeTok(val)) << std::endl;
    }

    std::cout << "Finished. Memory usage: " << size_to_string(platform_get_current_used_ram()) << std::endl << std::endl;
}
