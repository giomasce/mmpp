
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <boost/filesystem.hpp>

#include "test/test_verification.h"

#include "platform.h"
#include "utils/utils.h"
#include "reader.h"

using namespace std;

const boost::filesystem::path test_basename = platform_get_resources_base() / "metamath-test";
const string tests_filenames = R"tests(
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

static vector< pair < string, bool > > get_tests() {
    istringstream iss(tests_filenames);
    vector< pair< string, bool > > tests;
    string line;
    while (getline(iss, line)) {
        string success, filename;
        istringstream iss2(line);
        iss2 >> success >> filename;
        if (filename == "") {
            continue;
        }
        tests.push_back(make_pair(filename, success == "pass"));
    }
    return tests;
}

bool test_verification_one(string filename, bool advanced_tests) {
    bool success = true;
    try {
        cout << "Memory usage when starting: " << size_to_string(platform_get_current_rss()) << endl;
        FileTokenizer ft(test_basename / filename);
        Reader p(ft, true, true);
        cout << "Reading library and executing all proofs..." << endl;
        p.run();
        LibraryImpl lib = p.get_library();
        cout << "Library has " << lib.get_symbols_num() << " symbols and " << lib.get_labels_num() << " labels" << endl;

        /*LabTok failing = 287;
        cout << "Checking proof of " << lib.resolve_label(failing) << endl;
        lib.get_assertion(failing).get_proof_executor(lib)->execute();*/

        if (advanced_tests) {
            cout << "Compressing all proofs and executing again..." << endl;
            for (auto &ass : lib.get_assertions()) {
                if (ass.is_valid() && ass.is_theorem()) {
                    CompressedProof compressed = ass.get_proof_executor(lib)->compress();
                    compressed.get_executor(lib, ass)->execute();
                }
            }

            cout << "Decompressing all proofs and executing again..." << endl;
            for (auto &ass : lib.get_assertions()) {
                if (ass.is_valid() && ass.is_theorem()) {
                    UncompressedProof uncompressed = ass.get_proof_executor(lib)->uncompress();
                    uncompressed.get_executor(lib, ass)->execute();
                }
            }

            cout << "Compressing and decompressing all proofs and executing again..." << endl;
            for (auto &ass : lib.get_assertions()) {
                if (ass.is_valid() && ass.is_theorem()) {
                    CompressedProof compressed = ass.get_proof_executor(lib)->compress();
                    UncompressedProof uncompressed = compressed.get_executor(lib, ass)->uncompress();
                    uncompressed.get_executor(lib, ass)->execute();
                }
            }

            cout << "Decompressing and compressing all proofs and executing again..." << endl;
            for (auto &ass : lib.get_assertions()) {
                if (ass.is_valid() && ass.is_theorem()) {
                    UncompressedProof uncompressed = ass.get_proof_executor(lib)->uncompress();
                    CompressedProof compressed = uncompressed.get_executor(lib, ass)->compress();
                    compressed.get_executor(lib, ass)->execute();
                }
            }
        } else {
            cout << "Skipping compression and decompression test" << endl;
        }

        cout << "Finished. Memory usage: " << size_to_string(platform_get_current_rss()) << endl;
    } catch (const MMPPException &e) {
        cout << "An exception with message '" << e.get_reason() << "' was thrown!" << endl;
        e.print_stacktrace(cout);
        success = false;
    } catch (const ProofException &e) {
        cout << "An exception with message '" << e.get_reason() << "' was thrown!" << endl;
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
        string filename = test_pair.first;
        bool expect_success = test_pair.second;
        cout << "Testing file " << filename << " from " << test_basename << ", which is expected to " << (expect_success ? "pass" : "fail" ) << "..." << endl;

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
                cout << "Good, it worked!" << endl;
            } else {
                cout << "Bad, it should have failed!" << endl;
                problems++;
            }
        } else {
            if (expect_success) {
                cout << "Bad, this was NOT expected!" << endl;
                problems++;
            } else {
                cout << "Good, this was expected!" << endl;
            }
        }

        cout << "-------" << endl << endl;
    }
    cout << "Found " << problems << " problems" << endl;
}
