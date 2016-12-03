#include "toolbox.h"
#include "statics.h"
#include "unification.h"

using namespace std;

LibraryToolbox::LibraryToolbox(const LibraryInterface &lib) :
    lib(lib)
{
}

std::vector<SymTok> LibraryToolbox::substitute(const std::vector<SymTok> &orig, const std::unordered_map<SymTok, std::vector<SymTok> > &subst_map) const
{
    vector< SymTok > ret;
    for (auto it = orig.begin(); it != orig.end(); it++) {
        const SymTok &tok = *it;
        if (this->lib.is_constant(tok)) {
            ret.push_back(tok);
        } else {
            const vector< SymTok > &subst = subst_map.at(tok);
            copy(subst.begin(), subst.end(), back_inserter(ret));
        }
    }
    return ret;
}

// Computes second o first
std::unordered_map<SymTok, std::vector<SymTok> > LibraryToolbox::compose_subst(const std::unordered_map<SymTok, std::vector<SymTok> > &first, const std::unordered_map<SymTok, std::vector<SymTok> > &second) const
{
    std::unordered_map< SymTok, std::vector< SymTok > > ret;
    for (auto &first_pair : first) {
        auto res = ret.insert(make_pair(first_pair.first, this->substitute(first_pair.second, second)));
        assert(res.second);
    }
    return ret;
}

static vector< size_t > invert_perm(const vector< size_t > &perm) {
    vector< size_t > ret(perm.size());
    for (size_t i = 0; i < perm.size(); i++) {
        ret[perm[i]] = i;
    }
    return ret;
}

std::vector<LabTok> LibraryToolbox::proving_helper(const std::vector<std::vector<SymTok> > &templ_hyps, const std::vector<SymTok> &templ_thesis,
                                                   const std::vector<std::vector<LabTok> > &hyps_proofs, const std::unordered_map<SymTok, std::vector<SymTok> > &subst_map) const
{
    vector< LabTok > ret;
    auto res = this->lib.unify_assertion(templ_hyps, templ_thesis, true);
    assert_or_throw(!res.empty(), "Could not find the template assertion");
    const Assertion &ass = this->lib.get_assertion(get<0>(*res.begin()));
    assert(ass.is_valid());
    const vector< size_t > &perm = get<1>(*res.begin());
    const vector< size_t > perm_inv = invert_perm(perm);
    const unordered_map< SymTok, vector< SymTok > > &ass_map = get<2>(*res.begin());
    const unordered_map< SymTok, vector< SymTok > > full_map = this->compose_subst(ass_map, subst_map);

    // Compute floating hypotheses
    for (size_t i = 0; i < ass.get_num_floating(); i++) {
        auto proof = this->lib.prove_type(this->substitute(this->lib.get_sentence(ass.get_mand_hyps()[i]), full_map));
        copy(proof.begin(), proof.end(), back_inserter(ret));
    }

    // Compute essential hypotheses
    for (size_t i = 0; i < ass.get_mand_hyps().size() - ass.get_num_floating(); i++) {
        copy(hyps_proofs[perm_inv[i]].begin(), hyps_proofs[perm_inv[i]].end(), back_inserter(ret));
    }

    // Finally add this assertion's label
    ret.push_back(ass.get_thesis());

    return ret;
}

bool LibraryToolbox::proving_helper2(const std::vector<std::vector<SymTok> > &templ_hyps, const std::vector<SymTok> &templ_thesis, const std::vector<std::function<bool(const LibraryInterface &, ProofEngine &)> > &hyps_provers, const std::unordered_map<SymTok, std::vector<SymTok> > &subst_map, ProofEngine &engine) const
{
    engine.checkpoint();
    auto res = this->lib.unify_assertion(templ_hyps, templ_thesis, true);
    assert_or_throw(!res.empty(), "Could not find the template assertion");
    const Assertion &ass = this->lib.get_assertion(get<0>(*res.begin()));
    assert(ass.is_valid());
    const vector< size_t > &perm = get<1>(*res.begin());
    const vector< size_t > perm_inv = invert_perm(perm);
    const unordered_map< SymTok, vector< SymTok > > &ass_map = get<2>(*res.begin());
    const unordered_map< SymTok, vector< SymTok > > full_map = this->compose_subst(ass_map, subst_map);

    // Compute floating hypotheses
    // TODO Handle rollback for floating hypotheses
    for (size_t i = 0; i < ass.get_num_floating(); i++) {
        auto proof = this->lib.prove_type(this->substitute(this->lib.get_sentence(ass.get_mand_hyps()[i]), full_map));
        for (auto &label : proof) {
            engine.process_label(label);
        }
    }

    // Compute essential hypotheses
    for (size_t i = 0; i < ass.get_mand_hyps().size() - ass.get_num_floating(); i++) {
        bool res = hyps_provers[perm_inv[i]](lib, engine);
        if (!res) {
            engine.rollback();
            return false;
        }
    }

    // Finally add this assertion's label
    engine.process_label(ass.get_thesis());

    engine.commit();
    return true;
}

bool LibraryToolbox::proving_helper3(const std::vector<std::vector<SymTok> > &templ_hyps, const std::vector<SymTok> &templ_thesis, const std::unordered_map<SymTok, Prover> &types_provers, const std::vector<Prover> &hyps_provers, ProofEngine &engine) const
{
    engine.checkpoint();
    auto res = this->lib.unify_assertion(templ_hyps, templ_thesis, true);
    assert_or_throw(!res.empty(), "Could not find the template assertion");
    const Assertion &ass = this->lib.get_assertion(get<0>(*res.begin()));
    assert(ass.is_valid());
    const vector< size_t > &perm = get<1>(*res.begin());
    const vector< size_t > perm_inv = invert_perm(perm);
    const unordered_map< SymTok, vector< SymTok > > &ass_map = get<2>(*res.begin());
    //const unordered_map< SymTok, vector< SymTok > > full_map = this->compose_subst(ass_map, subst_map);

    // Compute floating hypotheses
    for (size_t i = 0; i < ass.get_num_floating(); i++) {
        bool res = this->type_proving_helper(this->substitute(this->lib.get_sentence(ass.get_mand_hyps()[i]), ass_map), engine, types_provers);
        if (!res) {
            engine.rollback();
            return false;
        }
    }

    // Compute essential hypotheses
    for (size_t i = 0; i < ass.get_mand_hyps().size() - ass.get_num_floating(); i++) {
        bool res = hyps_provers[perm_inv[i]](lib, engine);
        if (!res) {
            engine.rollback();
            return false;
        }
    }

    // Finally add this assertion's label
    engine.process_label(ass.get_thesis());

    engine.commit();
    return true;
}

bool LibraryToolbox::type_proving_helper(const std::vector<SymTok> &type_sent, ProofEngine &engine, const std::unordered_map<SymTok, Prover> &var_provers) const
{
    // Iterate over all propositions (maybe just axioms would be enough) with zero essential hypotheses, try to match and recur on all matches;
    // hopefully nearly all branches die early and there is just one real long-standing branch;
    // when the length is 2 try to match with floating hypotheses.
    // The current implementation is probably less efficient and more copy-ish than it could be.
    assert(type_sent.size() >= 2);
    auto &type_const = type_sent.at(0);
    if (type_sent.size() == 2) {
        for (auto &test_type : this->lib.get_types()) {
            if (this->lib.get_sentence(test_type) == type_sent) {
                auto &type_var = type_sent.at(1);
                auto it = var_provers.find(type_var);
                if (it == var_provers.end()) {
                    engine.process_label(test_type);
                    return true;
                } else {
                    auto &prover = var_provers.at(type_var);
                    return prover(this->lib, engine);
                }
            }
        }
    }
    // If a there are no assertions for a certain type (which is possible, see for example "set" in set.mm), then processing stops here
    if (this->lib.get_assertions_by_type().find(type_const) == this->lib.get_assertions_by_type().end()) {
        return false;
    }
    for (auto &templ : this->lib.get_assertions_by_type().at(type_const)) {
        const Assertion &templ_ass = this->lib.get_assertion(templ);
        if (templ_ass.get_num_floating() != templ_ass.get_mand_hyps().size()) {
            continue;
        }
        const auto &templ_sent = this->lib.get_sentence(templ);
        // We have to sort hypotheses by order af appearance for pushing them correctly on the stack; here we assume that the numeric order of labels coincides with the order of appearance
        vector< pair< LabTok, SymTok > > hyp_labels;
        for (auto &tok : templ_sent) {
            if (!this->lib.is_constant(tok)) {
                hyp_labels.push_back(make_pair(this->lib.get_types_by_var()[tok], tok));
            }
        }
        sort(hyp_labels.begin(), hyp_labels.end());
        auto unifications = unify(type_sent, templ_sent, this->lib);
        for (auto &unification : unifications) {
            bool failed = false;
            engine.checkpoint();
            for (auto &hyp_pair : hyp_labels) {
                const SymTok &var = hyp_pair.second;
                const vector< SymTok > &subst = unification.at(var);
                SymTok type = this->lib.get_sentence(this->lib.get_types_by_var().at(var)).at(0);
                vector< SymTok > new_type_sent = { type };
                // TODO This is not very efficient
                copy(subst.begin(), subst.end(), back_inserter(new_type_sent));
                bool res = this->type_proving_helper(new_type_sent, engine, var_provers);
                if (!res) {
                    failed = true;
                    engine.rollback();
                    break;
                }
            }
            if (!failed) {
                engine.process_label(templ);
                engine.commit();
                return true;
            }
        }
    }
    return false;
}

std::function<bool (const LibraryInterface &, ProofEngine &)> LibraryToolbox::build_prover2(const std::vector<std::vector<SymTok> > &templ_hyps, const std::vector<SymTok> &templ_thesis, const std::vector<std::function<bool (const LibraryInterface &, ProofEngine &)> > &hyps_provers, const std::unordered_map<SymTok, std::vector<SymTok> > &subst_map)
{
    return [=](const LibraryInterface & lib, ProofEngine &engine){
        LibraryToolbox tb(lib);
        return tb.proving_helper2(templ_hyps, templ_thesis, hyps_provers, subst_map, engine);
    };
}

std::function<bool (const LibraryInterface &, ProofEngine &)> LibraryToolbox::build_prover3(const std::vector<std::vector<SymTok> > &templ_hyps, const std::vector<SymTok> &templ_thesis, const std::unordered_map<SymTok, Prover> &types_provers, const std::vector<Prover> &hyps_provers)
{
    return [=](const LibraryInterface & lib, ProofEngine &engine){
        LibraryToolbox tb(lib);
        return tb.proving_helper3(templ_hyps, templ_thesis, types_provers, hyps_provers, engine);
    };
}

std::function<bool (const LibraryInterface &, ProofEngine &)> LibraryToolbox::build_type_prover(const std::vector<SymTok> &type_sent, const std::unordered_map<SymTok, Prover> &var_provers)
{
    return [=](const LibraryInterface &lib, ProofEngine &engine){
        LibraryToolbox tb(lib);
        return tb.type_proving_helper(type_sent, engine, var_provers);
    };
}
