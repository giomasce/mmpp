
#include <string>
#include <iostream>
#include <vector>

#include "mm/proof.h"
#include "test.h"

#ifdef ENABLE_TEST_CODE

std::vector< std::string > test_enc = { "A", "B", "T", "UA", "UB", "UT", "VA", "VB", "YT", "UUA", "YYT", "UUUA", "Z" };
std::vector< int > test_dec = { 1, 2, 20, 21, 22, 40, 41, 42, 120, 121, 620, 621, 0 };

BOOST_DATA_TEST_CASE(test_compressed_decoder_encoder, boost::unit_test::data::make(test_enc) ^ boost::unit_test::data::make(test_dec), enc, dec) {
    CompressedDecoder cd;
    CompressedEncoder ce;
    BOOST_TEST(ce.push_code(CodeTok(dec)) == enc);
    for (size_t j = 0; j < enc.size()-1; j++) {
        BOOST_TEST(cd.push_char(enc[j]) == INVALID_CODE);
    }
    BOOST_TEST(cd.push_char(enc[enc.size()-1]) == CodeTok(dec));
}

#endif
