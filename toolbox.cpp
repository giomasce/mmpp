#include "toolbox.h"
#include "statics.h"
#include "unification.h"
#include "earley.h"
#include "parser.h"

using namespace std;

ostream &operator<<(ostream &os, const SentencePrinter &sp)
{
    bool first = true;
    if (sp.style == SentencePrinter::STYLE_ALTHTML) {
        os << sp.lib.get_htmlstrings().first;
        os << "<SPAN " << sp.lib.get_htmlstrings().second << ">";
    }
    for (auto &tok : sp.sent) {
        if (first) {
            first = false;
        } else {
            os << string(" ");
        }
        if (sp.style == SentencePrinter::STYLE_PLAIN) {
            os << sp.lib.resolve_symbol(tok);
        } else if (sp.style == SentencePrinter::STYLE_HTML) {
            os << sp.lib.get_htmldefs()[tok];
        } else if (sp.style == SentencePrinter::STYLE_ALTHTML) {
            os << sp.lib.get_althtmldefs()[tok];
        } else if (sp.style == SentencePrinter::STYLE_LATEX) {
            os << sp.lib.get_latexdefs()[tok];
        }
    }
    if (sp.style == SentencePrinter::STYLE_ALTHTML) {
        os << "</SPAN>";
    }
    return os;
}

ostream &operator<<(ostream &os, const ProofPrinter &sp)
{
    bool first = true;
    for (auto &label : sp.proof) {
        if (first) {
            first = false;
        } else {
            os << string(" ");
        }
        os << sp.lib.resolve_label(label);
    }
    return os;
}

LibraryToolbox::LibraryToolbox(const Library &lib) :
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

bool LibraryToolbox::proving_helper3(const std::vector<std::vector<SymTok> > &templ_hyps, const std::vector<SymTok> &templ_thesis, const std::unordered_map<SymTok, Prover> &types_provers, const std::vector<Prover> &hyps_provers, ProofEngine &engine) const
{
    engine.checkpoint();
    auto res = this->unify_assertion(templ_hyps, templ_thesis, true);
    assert_or_throw(!res.empty(), "Could not find the template assertion");
    const Assertion &ass = this->lib.get_assertion(get<0>(*res.begin()));
    assert(ass.is_valid());
    const vector< size_t > &perm = get<1>(*res.begin());
    const vector< size_t > perm_inv = invert_perm(perm);
    const unordered_map< SymTok, vector< SymTok > > &ass_map = get<2>(*res.begin());
    //const unordered_map< SymTok, vector< SymTok > > full_map = this->compose_subst(ass_map, subst_map);

    // Compute floating hypotheses
    for (auto &hyp : ass.get_float_hyps()) {
        bool res = this->classical_type_proving_helper(this->substitute(this->lib.get_sentence(hyp), ass_map), engine, types_provers);
        if (!res) {
            engine.rollback();
            return false;
        }
    }

    // Compute essential hypotheses
    for (size_t i = 0; i < ass.get_ess_hyps().size(); i++) {
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

bool LibraryToolbox::proving_helper4(const std::vector<string> &templ_hyps, const std::string &templ_thesis, const std::unordered_map<string, Prover> &types_provers, const std::vector<Prover> &hyps_provers, ProofEngine &engine) const
{
    std::vector<std::vector<SymTok> > templ_hyps_sent;
    for (auto &hyp : templ_hyps) {
        templ_hyps_sent.push_back(this->parse_sentence(hyp));
    }
    std::vector<SymTok> templ_thesis_sent = this->parse_sentence(templ_thesis);
    std::unordered_map<SymTok, Prover> types_provers_sym;
    for (auto &type_pair : types_provers) {
        auto res = types_provers_sym.insert(make_pair(lib.get_symbol(type_pair.first), type_pair.second));
        assert(res.second);
    }
    return this->proving_helper3(templ_hyps_sent, templ_thesis_sent, types_provers_sym, hyps_provers, engine);
}

bool LibraryToolbox::classical_type_proving_helper(const std::vector<SymTok> &type_sent, ProofEngine &engine, const std::unordered_map<SymTok, Prover> &var_provers) const
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
        if (templ_ass.get_ess_hyps().size() != 0) {
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
                bool res = this->classical_type_proving_helper(new_type_sent, engine, var_provers);
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

static void earley_type_unwind_tree(const EarleyTreeItem &tree, ProofEngine &engine, const Library &lib) {
    // We need to sort children according to their order as floating hypotheses of this assertion
    // If this is not an assertion, then there are no children
    const Assertion &ass = lib.get_assertion(tree.label);
    if (ass.is_valid()) {
        unordered_map< SymTok, const EarleyTreeItem* > children;
        auto it = tree.children.begin();
        for (auto &tok : lib.get_sentence(tree.label)) {
            if (!lib.is_constant(tok)) {
                children[tok] = &(*it);
                it++;
            }
        }
        assert(it == tree.children.end());
        for (auto &hyp : ass.get_float_hyps()) {
            SymTok tok = lib.get_sentence(hyp).at(1);
            earley_type_unwind_tree(*children.at(tok), engine, lib);
        }
    } else {
        assert(tree.children.size() == 0);
    }
    engine.process_label(tree.label);
}

// TODO Use var_provers
bool LibraryToolbox::earley_type_proving_helper(const std::vector<SymTok> &type_sent, ProofEngine &engine, const std::unordered_map<SymTok, Prover> &var_provers) const
{
    SymTok type = type_sent[0];
    vector< SymTok > sent;
    copy(type_sent.begin()+1, type_sent.end(), back_inserter(sent));

    // Build the derivation rules; a derivation is created for each $f statement
    // and for each $a and $p statement without essential hypotheses such that no variable
    // appears more than once and without distinct variables constraints
    std::unordered_map<SymTok, std::vector<std::pair< LabTok, std::vector<SymTok> > > > derivations;
    for (auto &type_lab : this->lib.get_types()) {
        auto &type_sent = this->lib.get_sentence(type_lab);
        derivations[type_sent.at(0)].push_back(make_pair(type_lab, vector<SymTok>({type_sent.at(1)})));
    }
    for (const Assertion &ass : this->lib.get_assertions()) {
        if (!ass.is_valid()) {
            continue;
        }
        if (ass.get_ess_hyps().size() != 0) {
            continue;
        }
        if (ass.get_mand_dists().size() != 0) {
            continue;
        }
        auto &sent = this->lib.get_sentence(ass.get_thesis());
        set< SymTok > symbols;
        bool duplicate = false;
        for (auto &tok : sent) {
            if (this->lib.is_constant(tok)) {
                continue;
            }
            if (symbols.find(tok) != symbols.end()) {
                duplicate = true;
                break;
            }
            symbols.insert(tok);
        }
        if (duplicate) {
            continue;
        }
        vector< SymTok > sent2;
        for (size_t i = 1; i < sent.size(); i++) {
            auto tok = sent[i];
            // Variables are replaced with their types
            sent2.push_back(this->lib.is_constant(tok) ? tok : this->lib.get_sentence(this->lib.get_types_by_var().at(tok)).at(0));
        }
        derivations[sent.at(0)].push_back(make_pair(ass.get_thesis(), sent2));
    }

    EarleyTreeItem tree = earley(sent, type, derivations);
    if (tree.label == 0) {
        return false;
    } else {
        earley_type_unwind_tree(tree, engine, lib);
        return true;
    }
}

Prover LibraryToolbox::build_prover4(const std::vector<string> &templ_hyps, const string &templ_thesis, const std::unordered_map<string, Prover> &types_provers, const std::vector<Prover> &hyps_provers)
{
    return [=](const Library & lib, ProofEngine &engine){
        LibraryToolbox tb(lib);
        return tb.proving_helper4(templ_hyps, templ_thesis, types_provers, hyps_provers, engine);
    };
}

Prover LibraryToolbox::build_type_prover2(const std::string &type_sent, const std::unordered_map<SymTok, Prover> &var_provers)
{
    return [=](const Library &lib, ProofEngine &engine){
        LibraryToolbox tb(lib);
        vector< SymTok > type_sent2 = tb.parse_sentence(type_sent);
        return tb.classical_type_proving_helper(type_sent2, engine, var_provers);
    };
}

Prover LibraryToolbox::cascade_provers(const Prover &a,  const Prover &b)
{
    return [=](const Library & lib, ProofEngine &engine) {
        bool res;
        res = a(lib, engine);
        if (res) {
            return true;
        }
        res = b(lib, engine);
        return res;
    };
}

std::vector<SymTok> LibraryToolbox::parse_sentence(const string &in) const
{
    auto toks = tokenize(in);
    vector< SymTok > res;
    for (auto &tok : toks) {
        auto tok_num = this->lib.get_symbol(tok);
        assert_or_throw(tok_num != 0);
        res.push_back(tok_num);
    }
    return res;
}

SentencePrinter LibraryToolbox::print_sentence(const std::vector<SymTok> &sent, SentencePrinter::Style style) const
{
    return SentencePrinter({ sent, this->lib, style });
}

ProofPrinter LibraryToolbox::print_proof(const std::vector<LabTok> &proof) const
{
    return ProofPrinter({ proof, this->lib });
}

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > > LibraryToolbox::unify_assertion(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first) const
{
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > ret;

    vector< SymTok > sent;
    for (auto &hyp : hypotheses) {
        copy(hyp.begin(), hyp.end(), back_inserter(sent));
        sent.push_back(0);
    }
    copy(thesis.begin(), thesis.end(), back_inserter(sent));

    for (const Assertion &ass : this->lib.get_assertions()) {
        if (!ass.is_valid()) {
            continue;
        }
        if (ass.is_usage_disc()) {
            continue;
        }
        if (ass.get_ess_hyps().size() != hypotheses.size()) {
            continue;
        }
        // We have to generate all the hypotheses' permutations; fortunately usually hypotheses are not many
        // TODO Is there a better algorithm?
        // The i-th specified hypothesis is matched with the perm[i]-th assertion hypothesis
        vector< size_t > perm;
        for (size_t i = 0; i < hypotheses.size(); i++) {
            perm.push_back(i);
        }
        do {
            vector< SymTok > templ;
            for (size_t i = 0; i < hypotheses.size(); i++) {
                auto &hyp = this->lib.get_sentence(ass.get_ess_hyps()[perm[i]]);
                copy(hyp.begin(), hyp.end(), back_inserter(templ));
                templ.push_back(0);
            }
            auto &th = this->lib.get_sentence(ass.get_thesis());
            copy(th.begin(), th.end(), back_inserter(templ));
            auto unifications = unify(sent, templ, this->lib);
            if (!unifications.empty()) {
                for (auto &unification : unifications) {
                    ret.emplace_back(ass.get_thesis(), perm, unification);
                    if (just_first) {
                        return ret;
                    }
                }
            }
        } while (next_permutation(perm.begin(), perm.end()));
    }

    return ret;
}

Prover LibraryToolbox::build_classical_type_prover(const std::vector<SymTok> &type_sent, const std::unordered_map<SymTok, Prover> &var_provers)
{
    return [=](const Library &lib, ProofEngine &engine){
        LibraryToolbox tb(lib);
        return tb.classical_type_proving_helper(type_sent, engine, var_provers);
    };
}

Prover LibraryToolbox::build_earley_type_prover(const std::vector<SymTok> &type_sent, const std::unordered_map<SymTok, Prover> &var_provers)
{
    return [=](const Library &lib, ProofEngine &engine){
        LibraryToolbox tb(lib);
        return tb.earley_type_proving_helper(type_sent, engine, var_provers);
    };
}

Prover LibraryToolbox::build_type_prover(const std::vector<SymTok> &type_sent, const std::unordered_map<SymTok, Prover> &var_provers)
{
    return LibraryToolbox::build_classical_type_prover(type_sent, var_provers);
}
