#pragma once

#include <iostream>

// See the comment in test_main.cpp
#if (!(defined(_WIN32)))
#define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>
#include <boost/test/data/monomorphic/generators/xrange.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

// Some useful printers
namespace std {
template< typename T, typename U >
std::ostream &operator<<(std::ostream &str, const std::pair< T, U > &x) {
    return str << "(" << x.first << ", " << x.second << ")";
}
}

#include "mm/funds.h"

inline std::ostream &operator<<(std::ostream &str, SymTok tok) {
    return str << tok.val();
}

inline std::ostream &operator<<(std::ostream &str, LabTok tok) {
    return str << tok.val();
}

inline std::ostream &operator<<(std::ostream &str, CodeTok tok) {
    return str << tok.val();
}
