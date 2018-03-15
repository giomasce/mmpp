
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <boost/filesystem.hpp>

#include "test/test_verification.h"

#include "platform.h"
#include "utils/utils.h"
#include "mm/reader.h"
#include "mm/proof.h"

const boost::filesystem::path test_basename = platform_get_resources_base() / "metamath-test";
const std::string tests_filenames = R"tests(
fail anatomy-bad1.mm "Simple incorrect 'anatomy' test "
fail anatomy-bad2.mm "Simple incorrect 'anatomy' test "
fail anatomy-bad3.mm "Simple incorrect 'anatomy' test "
pass big-unifier.mm
fail big-unifier-bad1.mm
fail big-unifier-bad2.mm
fail big-unifier-bad3.mm
pass demo0.mm
fail demo0-bad1.mm
pass demo0-includer.mm "Test simple file inclusion"
pass emptyline.mm 'A file with one empty line'
pass hol.mm
pass iset.mm
pass miu.mm
pass nf.mm
pass peano-fixed.mm
pass ql.mm
pass set.2010-08-29.mm
pass set.mm
fail set-dist.mm
pass lof.mm
pass lofmathbox.mm
pass lofset.mm
pass set(lof).mm
)tests";

static std::vector< std::pair < std::string, bool > > get_tests() {
    std::istringstream iss(tests_filenames);
    std::vector< std::pair< std::string, bool > > tests;
    std::string line;
    while (std::getline(iss, line)) {
        std::string success, filename;
        std::istringstream iss2(line);
        iss2 >> success >> filename;
        if (filename == "") {
            continue;
        }
        tests.push_back(std::make_pair(filename, success == "pass"));
    }
    return tests;
}

bool test_verification_one(std::string filename, bool advanced_tests) {
    bool success = true;
    try {
        std::cout << "Memory usage when starting: " << size_to_string(platform_get_current_used_ram()) << std::endl;
        FileTokenizer ft(test_basename / filename);
        Reader p(ft, true, true);
        std::cout << "Reading library and executing all proofs..." << std::endl;
        p.run();
        LibraryImpl lib = p.get_library();
        std::cout << "Library has " << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << std::endl;

        /*LabTok failing = 287;
        std::cout << "Checking proof of " << lib.resolve_label(failing) << std::endl;
        lib.get_assertion(failing).get_proof_executor(lib)->execute();*/

        if (advanced_tests) {
            std::cout << "Compressing all proofs and executing again..." << std::endl;
            for (auto &ass : lib.get_assertions()) {
                if (ass.is_valid() && ass.is_theorem()) {
                    CompressedProof compressed = ass.get_proof_operator(lib)->compress();
                    compressed.get_executor< Sentence >(lib, ass)->execute();
                }
            }

            std::cout << "Decompressing all proofs and executing again..." << std::endl;
            for (auto &ass : lib.get_assertions()) {
                if (ass.is_valid() && ass.is_theorem()) {
                    UncompressedProof uncompressed = ass.get_proof_operator(lib)->uncompress();
                    uncompressed.get_executor< Sentence >(lib, ass)->execute();
                }
            }

            std::cout << "Compressing and decompressing all proofs and executing again..." << std::endl;
            for (auto &ass : lib.get_assertions()) {
                if (ass.is_valid() && ass.is_theorem()) {
                    CompressedProof compressed = ass.get_proof_operator(lib)->compress();
                    UncompressedProof uncompressed = compressed.get_operator(lib, ass)->uncompress();
                    uncompressed.get_executor< Sentence >(lib, ass)->execute();
                }
            }

            std::cout << "Decompressing and compressing all proofs and executing again..." << std::endl;
            for (auto &ass : lib.get_assertions()) {
                if (ass.is_valid() && ass.is_theorem()) {
                    UncompressedProof uncompressed = ass.get_proof_operator(lib)->uncompress();
                    CompressedProof compressed = uncompressed.get_operator(lib, ass)->compress();
                    compressed.get_executor< Sentence >(lib, ass)->execute();
                }
            }
        } else {
            std::cout << "Skipping compression and decompression test" << std::endl;
        }

        std::cout << "Finished. Memory usage: " << size_to_string(platform_get_current_used_ram()) << std::endl;
    } catch (const MMPPException &e) {
        std::cout << "An exception with message '" << e.get_reason() << "' was thrown!" << std::endl;
        e.print_stacktrace(std::cout);
        success = false;
    } catch (const ProofException< Sentence > &e) {
        std::cout << "An exception with message '" << e.get_reason() << "' was thrown!" << std::endl;
        success = false;
    }

    return success;
}

void test_all_verifications() {
    auto tests = get_tests();
    // Comment or uncomment the following line to disable or enable tests with metamath-test
    //tests = {};
    int problems = 0;
    for (auto test_pair : tests) {
        std::string filename = test_pair.first;
        bool expect_success = test_pair.second;
        std::cout << "Testing file " << filename << " from " << test_basename << ", which is expected to " << (expect_success ? "pass" : "fail" ) << "..." << std::endl;

        // Useful for debugging
        /*if (filename == "demo0.mm") {
            mmpp_abort = true;
        } else {
            continue;
            mmpp_abort = false;
        }*/
        /*if (filename == "nf.mm") {
            break;
        }*/

        bool success = test_verification_one(filename, expect_success);
        if (success) {
            if (expect_success) {
                std::cout << "Good, it worked!" << std::endl;
            } else {
                std::cout << "Bad, it should have failed!" << std::endl;
                problems++;
            }
        } else {
            if (expect_success) {
                std::cout << "Bad, this was NOT expected!" << std::endl;
                problems++;
            } else {
                std::cout << "Good, this was expected!" << std::endl;
            }
        }

        std::cout << "-------" << std::endl << std::endl;
    }
    std::cout << "Found " << problems << " problems" << std::endl;
}
