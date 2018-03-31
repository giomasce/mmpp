
#include <string>
#include <iostream>
#include <vector>

#include "provers/wff.h"
#include "mm/reader.h"
#include "utils/utils.h"
#include "platform.h"

std::ostream &operator<<(std::ostream &str, CodeTok tok) {
    return str << tok.val();
}

BOOST_AUTO_TEST_CASE(test_compressed_decoder_encoder) {
    CompressedDecoder cd;
    CompressedEncoder ce;
    std::vector< std::string > test_enc = { "A", "B", "T", "UA", "UB", "UT", "VA", "VB", "YT", "UUA", "YYT", "UUUA", "Z" };
    std::vector< int > test_dec = { 1, 2, 20, 21, 22, 40, 41, 42, 120, 121, 620, 621, 0 };
    assert(test_enc.size() == test_dec.size());
    for (size_t i = 0; i < test_enc.size(); i++) {
        BOOST_TEST(ce.push_code(CodeTok(test_dec[i])) == test_enc[i]);
        for (size_t j = 0; j < test_enc[i].size()-1; j++) {
            BOOST_TEST(cd.push_char(test_enc[i][j]) == INVALID_CODE);
        }
        BOOST_TEST(cd.push_char(test_enc[i][test_enc[i].size()-1]) == CodeTok(test_dec[i]));
    }
}
