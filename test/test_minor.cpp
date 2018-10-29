
#include <string>
#include <iostream>
#include <vector>

#include <giolib/containers.h>

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

BOOST_AUTO_TEST_CASE(test_is_disjoint) {
    std::vector< std::set< int > > data = {
        { 0, 1, 5, 10, 100 },
        { 2, 6, 50, 120 },
        { 1000, 2000, 3000 },
        { 1, 6, 200, 300 },
        {}
    };
    for (size_t i = 0; i < data.size(); i++) {
        for (size_t j = 0; j < data.size(); j++) {
            bool res = gio::is_disjoint(data[i].begin(), data[i].end(), data[j].begin(), data[j].end());
            if ((i == j && i != 4) || (i == 0 && j == 3) || (i == 3 && j == 0) || (i == 1 && j == 3) || (i == 3 && j == 1)) {
                BOOST_TEST(!res);
            } else {
                BOOST_TEST(res);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_is_included) {
    std::vector< std::set< int > > data = {
        {},
        { 0, 1, 5, 10, 100 },
        { 0, 1, 10 },
        { 0, 1, 7, 10 },
    };
    for (size_t i = 0; i < data.size(); i++) {
        for (size_t j = 0; j < data.size(); j++) {
            bool res = gio::is_included(data[i].begin(), data[i].end(), data[j].begin(), data[j].end());
            if (i == 0 || i == j || (i == 2 && j == 1) || (i == 2 && j == 3)) {
                BOOST_TEST(res);
            } else {
                BOOST_TEST(!res);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_has_no_diagonal) {
    std::set< std::pair< int, int > > x1 = { { 3, 5}, { 2, 10 }, { 0, 1} };
    std::set< std::pair< int, int > > x2 = { { 3, 5}, { 2, 10 }, { 0, 1}, { 3, 3 } };
    std::set< std::pair< int, int > > x3;
    BOOST_TEST(gio::has_no_diagonal(x1.begin(), x1.end()));
    BOOST_TEST(!gio::has_no_diagonal(x2.begin(), x2.end()));
    BOOST_TEST(gio::has_no_diagonal(x3.begin(), x3.end()));
}

#endif
