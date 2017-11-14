
#include <iostream>

#include "utils/utils.h"
#include "test/test_env.h"

using namespace std;

void print_trace(const ProofTree &pt, const Library &lib, const Assertion &ass) {
    size_t essentials_num = 0;
    for (const auto &child : pt.children) {
        if (child.essential) {
            print_trace(child, lib, ass);
            essentials_num++;
        }
    }
    auto it = find(ass.get_ess_hyps().begin(), ass.get_ess_hyps().end(), pt.label);
    if (it == ass.get_ess_hyps().end()) {
        cout << lib.resolve_label(pt.label) << " " << essentials_num << " ";
    } else {
        cout << "_hyp" << (it - ass.get_ess_hyps().begin()) << " 0 ";
    }
}

int dissector_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    auto &tb = data.tb;

    const Assertion &ass = tb.get_assertion(tb.get_label("fta"));
    UncompressedProof unc_proof = ass.get_proof_executor(tb, false)->uncompress();
    auto pe = unc_proof.get_executor(tb, ass, true);
    pe->execute();
    const ProofTree &pt = pe->get_proof_tree();
    print_trace(pt, tb, ass);
    cout << endl;

    return 0;
}
static_block {
    register_main_function("mmpp_dissector", dissector_main);
}
