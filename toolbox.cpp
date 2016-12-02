#include "toolbox.h"
#include "statics.h"

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
