#pragma once

/* For the reasons explained in test/test.cpp, all the code using Boost.Test must
 * be guarded by the ENABLE_TEST_CODE macro.
 */

#ifndef DISABLE_TESTS

// See the comment in test_main.cpp
#if (!defined(_WIN32)) || (defined(TEST_BUILD))
#if (!(defined(_WIN32)))
#define BOOST_TEST_DYN_LINK
#endif
#define ENABLE_TEST_CODE
#include <boost/test/unit_test.hpp>
#include <boost/test/data/monomorphic/generators/xrange.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#endif

#endif

#include <iostream>

#include "mm/funds.h"

// Some useful printers
namespace std {
template< typename T, typename U >
std::ostream &operator<<(std::ostream &str, const std::pair< T, U > &x) {
    return str << "(" << x.first << ", " << x.second << ")";
}
}

inline std::ostream &operator<<(std::ostream &str, SymTok tok) {
    return str << tok.val();
}

inline std::ostream &operator<<(std::ostream &str, LabTok tok) {
    return str << tok.val();
}

inline std::ostream &operator<<(std::ostream &str, CodeTok tok) {
    return str << tok.val();
}
