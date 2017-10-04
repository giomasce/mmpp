
#include <boost/filesystem/fstream.hpp>

#include "toolbox.h"
#include "utils.h"
#include "unification.h"
#include "unif.h"
#include "earley.h"
#include "reader.h"

using namespace std;

#define TOOLBOX_SELF_TEST

ostream &operator<<(ostream &os, const SentencePrinter &sp)
{
    bool first = true;
    if (sp.style == SentencePrinter::STYLE_ALTHTML) {
        // TODO - HTML fixing could be done just once
        os << fix_htmlcss_for_qt(sp.tb.get_addendum().get_htmlcss());
        os << "<SPAN " << sp.tb.get_addendum().get_htmlfont() << ">";
    }
    Sentence sent2;
    const Sentence *sentp = NULL;
    if (sp.is_sent) {
        sentp = &sp.sent;
    } else {
        sent2 = sp.tb.reconstruct_sentence(sp.pt);
        sentp = &sent2;
    }
    const Sentence &sent = *sentp;
    for (auto &tok : sent) {
        if (first) {
            first = false;
        } else {
            os << string(" ");
        }
        if (sp.style == SentencePrinter::STYLE_PLAIN) {
            os << sp.tb.resolve_symbol(tok);
        } else if (sp.style == SentencePrinter::STYLE_HTML) {
            os << sp.tb.get_addendum().get_htmldef(tok);
        } else if (sp.style == SentencePrinter::STYLE_ALTHTML) {
            os << sp.tb.get_addendum().get_althtmldef(tok);
        } else if (sp.style == SentencePrinter::STYLE_LATEX) {
            os << sp.tb.get_addendum().get_latexdef(tok);
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
        os << sp.tb.resolve_label(label);
    }
    return os;
}

LibraryToolbox::LibraryToolbox(const ExtendedLibrary &lib, string turnstile, bool compute, shared_ptr<ToolboxCache> cache) :
    lib(lib), turnstile(lib.get_symbol(turnstile)), turnstile_alias(lib.get_parsing_addendum().get_syntax().at(this->turnstile)),
    types(lib.get_final_stack_frame().types), types_set(lib.get_final_stack_frame().types_set),
    cache(cache), temp_syms(lib.get_symbols_num()+1), temp_labs(lib.get_labels_num()+1)
{
    assert(this->lib.is_immutable());
    if (compute) {
        this->compute_everything();
    }
}

LibraryToolbox::~LibraryToolbox()
{
    delete this->parser;
}

void LibraryToolbox::set_cache(std::shared_ptr<ToolboxCache> cache)
{
    this->cache = cache;
}

std::vector<SymTok> LibraryToolbox::substitute(const std::vector<SymTok> &orig, const std::unordered_map<SymTok, std::vector<SymTok> > &subst_map) const
{
    vector< SymTok > ret;
    for (auto it = orig.begin(); it != orig.end(); it++) {
        const SymTok &tok = *it;
        if (this->is_constant(tok)) {
            ret.push_back(tok);
        } else {
            const vector< SymTok > &subst = subst_map.at(tok);
            copy(subst.begin(), subst.end(), back_inserter(ret));
        }
    }
    return ret;
}

// Computes second o first (map composition)
std::unordered_map<SymTok, std::vector<SymTok> > LibraryToolbox::compose_subst(const std::unordered_map<SymTok, std::vector<SymTok> > &first, const std::unordered_map<SymTok, std::vector<SymTok> > &second) const
{
    throw "Buggy implementation, do not use";
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

bool LibraryToolbox::classical_type_proving_helper(const std::vector<SymTok> &type_sent, ProofEngine &engine, const std::unordered_map<SymTok, Prover> &var_provers) const
{
    // Iterate over all propositions (maybe just axioms would be enough) with zero essential hypotheses, try to match and recur on all matches;
    // hopefully nearly all branches die early and there is just one real long-standing branch;
    // when the length is 2 try to match with floating hypotheses.
    // The current implementation is probably less efficient and more copy-ish than it could be.
    assert(type_sent.size() >= 2);
    auto &type_const = type_sent.at(0);
    if (type_sent.size() == 2) {
        for (auto &test_type : this->get_types()) {
            if (this->get_sentence(test_type) == type_sent) {
                auto &type_var = type_sent.at(1);
                auto it = var_provers.find(type_var);
                if (it == var_provers.end()) {
                    engine.process_label(test_type);
                    return true;
                } else {
                    auto &prover = var_provers.at(type_var);
                    return prover(engine);
                }
            }
        }
    }
    // If a there are no assertions for a certain type (which is possible, see for example "set" in set.mm), then processing stops here
    if (this->get_assertions_by_type().find(type_const) == this->get_assertions_by_type().end()) {
        return false;
    }
    for (auto &templ : this->get_assertions_by_type().at(type_const)) {
        const Assertion &templ_ass = this->get_assertion(templ);
        if (templ_ass.get_ess_hyps().size() != 0) {
            continue;
        }
        const auto &templ_sent = this->get_sentence(templ);
        // We have to sort hypotheses by order af appearance for pushing them correctly on the stack; here we assume that the numeric order of labels coincides with the order of appearance
        vector< pair< LabTok, SymTok > > hyp_labels;
        for (auto &tok : templ_sent) {
            if (!this->is_constant(tok)) {
                hyp_labels.push_back(make_pair(this->get_types_by_var()[tok], tok));
            }
        }
        sort(hyp_labels.begin(), hyp_labels.end());
        auto unifications = unify(type_sent, templ_sent, *this);
        for (auto &unification : unifications) {
            bool failed = false;
            engine.checkpoint();
            for (auto &hyp_pair : hyp_labels) {
                const SymTok &var = hyp_pair.second;
                const vector< SymTok > &subst = unification.at(var);
                SymTok type = this->get_sentence(this->get_types_by_var().at(var)).at(0);
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

static void earley_type_unwind_tree(const ParsingTree< SymTok, LabTok > &tree, ProofEngine &engine, const Library &lib, const std::unordered_map<SymTok, Prover> &var_provers) {
    // We need to sort children according to their order as floating hypotheses of this assertion
    // If this is not an assertion, then there are no children
    const Assertion &ass = lib.get_assertion(tree.label);
    if (ass.is_valid()) {
        unordered_map< SymTok, const ParsingTree< SymTok, LabTok >* > children;
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
            earley_type_unwind_tree(*children.at(tok), engine, lib, var_provers);
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
    auto derivations = this->get_derivations();

    EarleyParser parser(derivations);
    ParsingTree tree = parser.parse(type_sent.begin()+1, type_sent.end(), type);
    if (tree.label == 0) {
        return false;
    } else {
        earley_type_unwind_tree(tree, engine, *this, var_provers);
        return true;
    }
}

const std::unordered_map<SymTok, std::vector<std::pair<LabTok, std::vector<SymTok> > > > &LibraryToolbox::get_derivations()
{
    if (!this->derivations_computed) {
        this->compute_derivations();
    }
    return this->derivations;
}

const std::unordered_map<SymTok, std::vector<std::pair<LabTok, std::vector<SymTok> > > > &LibraryToolbox::get_derivations() const
{
    if (!this->derivations_computed) {
        throw MMPPException("computation required on const object");
    }
    return this->derivations;
}

void LibraryToolbox::compute_ders_by_label()
{
    this->ders_by_label = compute_derivations_by_label(this->get_derivations());
    this->ders_by_label_computed = true;
}

const std::unordered_map<LabTok, std::pair<SymTok, std::vector<SymTok> > > &LibraryToolbox::get_ders_by_label()
{
    if (!this->ders_by_label_computed) {
        this->compute_ders_by_label();
    }
    return this->ders_by_label;
}

const std::unordered_map<LabTok, std::pair<SymTok, std::vector<SymTok> > > &LibraryToolbox::get_ders_by_label() const
{
    if (!this->ders_by_label_computed) {
        throw MMPPException("computation required on const object");
    }
    return this->ders_by_label;
}

std::vector<SymTok> LibraryToolbox::reconstruct_sentence(const ParsingTree<SymTok, LabTok> &pt)
{
    return ::reconstruct_sentence(pt, this->get_derivations(), this->get_ders_by_label());
}

std::vector<SymTok> LibraryToolbox::reconstruct_sentence(const ParsingTree<SymTok, LabTok> &pt) const
{
    return ::reconstruct_sentence(pt, this->get_derivations(), this->get_ders_by_label());
}

Prover LibraryToolbox::build_prover(const std::vector<Sentence> &templ_hyps, const Sentence &templ_thesis, const std::unordered_map<string, Prover> &types_provers, const std::vector<Prover> &hyps_provers) const
{
    auto res = this->unify_assertion(templ_hyps, templ_thesis, true);
    if (res.empty()) {
        cerr << string("Could not find the template assertion: ") + this->print_sentence(templ_thesis).to_string() << endl;
    }
    assert_or_throw(!res.empty(), string("Could not find the template assertion: ") + this->print_sentence(templ_thesis).to_string());
    const auto &res1 = res[0];
    return [=](ProofEngine &engine){
        RegisteredProverInstanceData inst_data;
        inst_data.valid = true;
        inst_data.label = get<0>(res1);
        const vector< size_t > &perm = get<1>(res1);
        inst_data.perm_inv = invert_perm(perm);
        inst_data.ass_map = get<2>(res1);
        return this->proving_helper(inst_data, types_provers, hyps_provers, engine);
    };
}

bool LibraryToolbox::proving_helper(const RegisteredProverInstanceData &inst_data, const std::unordered_map<string, Prover> &types_provers, const std::vector<Prover> &hyps_provers, ProofEngine &engine) const {
    const Assertion &ass = this->get_assertion(inst_data.label);
    assert(ass.is_valid());

    std::unordered_map<SymTok, Prover> types_provers_sym;
    for (auto &type_pair : types_provers) {
        auto res = types_provers_sym.insert(make_pair(this->get_symbol(type_pair.first), type_pair.second));
        assert(res.second);
    }

    engine.checkpoint();

    // Compute floating hypotheses
    for (auto &hyp : ass.get_float_hyps()) {
        bool res = this->classical_type_proving_helper(this->substitute(this->get_sentence(hyp), inst_data.ass_map), engine, types_provers_sym);
        if (!res) {
            cerr << "Applying " << inst_data.label_str << " a floating hypothesis failed..." << endl;
            engine.rollback();
            return false;
        }
    }

    // Compute essential hypotheses
    for (size_t i = 0; i < ass.get_ess_hyps().size(); i++) {
        bool res = hyps_provers[inst_data.perm_inv[i]](engine);
        if (!res) {
            cerr << "Applying " << inst_data.label_str << " an essential hypothesis failed..." << endl;
            engine.rollback();
            return false;
        }
    }

    // Finally add this assertion's label
    try {
        engine.process_label(ass.get_thesis());
    } catch (const ProofException &e) {
        cerr << "Applying " << inst_data.label_str << " the proof executor signalled an error..." << endl;
        cerr << "The reason was " << e.get_reason() << endl;
        cerr << "On stack there was: " << this->print_sentence(e.get_error().on_stack) << endl;
        cerr << "Has to match with: " << this->print_sentence(e.get_error().to_subst) << endl;
        cerr << "Substitution map:" << endl;
        for (const auto &it : e.get_error().subst_map) {
            cerr << this->resolve_symbol(it.first) << ": " << this->print_sentence(it.second) << endl;
        }
        engine.rollback();
        return false;
    }

    engine.commit();
    return true;
}

std::pair<LabTok, SymTok> LibraryToolbox::new_temp_var(SymTok type_sym)
{
    // Create names and variables
    assert(this->is_constant(type_sym));
    size_t idx = ++this->temp_idx[type_sym];
    string sym_name = this->resolve_symbol(type_sym) + to_string(idx);
    assert(this->get_symbol(sym_name) == 0);
    string lab_name = "temp" + sym_name;
    assert(this->get_label(lab_name) == 0);
    SymTok sym = this->temp_syms.create(sym_name);
    LabTok lab = this->temp_labs.create(lab_name);
    this->temp_types[lab] = { type_sym, sym };

    // Add the variables to a few structures
    this->types.push_back(lab);
    this->types_set.insert(lab);
    this->derivations.at(type_sym).push_back(pair< LabTok, vector< SymTok > >(lab, { sym }));
    this->ders_by_label[lab] = pair< SymTok, vector< SymTok > >(type_sym, { sym });

    return std::make_pair(lab, sym);
}

std::pair<std::vector<ParsingTree<SymTok, LabTok> >, ParsingTree<SymTok, LabTok> > LibraryToolbox::refresh_assertion(const Assertion &ass)
{
    // Collect all variables
    std::set< LabTok > var_labs;
    auto is_var = this->get_standard_is_var();
    const ParsingTree< SymTok, LabTok > &thesis_pt = this->get_parsed_sents().at(ass.get_thesis());
    collect_variables(thesis_pt, is_var, var_labs);
    for (const auto hyp : ass.get_ess_hyps()) {
        const ParsingTree< SymTok, LabTok > &hyp_pt = this->get_parsed_sents().at(hyp);
        collect_variables(hyp_pt, is_var, var_labs);
    }

    // Build a substitution map
    SubstMap< SymTok, LabTok > subst;
    for (const auto var_lab : var_labs) {
        SymTok type_sym = this->get_sentence(var_lab).at(0);
        SymTok new_sym;
        LabTok new_lab;
        tie(new_lab, new_sym) = this->new_temp_var(type_sym);
        ParsingTree< SymTok, LabTok > new_pt;
        new_pt.label = new_lab;
        new_pt.type = type_sym;
        subst[var_lab] = new_pt;
    }

    // Substitute and return
    ParsingTree< SymTok, LabTok > thesis_new_pt = ::substitute(thesis_pt, is_var, subst);
    vector< ParsingTree< SymTok, LabTok > > hyps_new_pts;
    for (const auto hyp : ass.get_ess_hyps()) {
        const ParsingTree< SymTok, LabTok > &hyp_pt = this->get_parsed_sents().at(hyp);
        hyps_new_pts.push_back(::substitute(hyp_pt, is_var, subst));
    }
    return make_pair(hyps_new_pts, thesis_new_pt);
}

SymTok LibraryToolbox::get_symbol(string s) const
{
    auto res = this->lib.get_symbol(s);
    if (res != 0) {
        return res;
    }
    return this->temp_syms.get(s);
}

LabTok LibraryToolbox::get_label(string s) const
{
    auto res = this->lib.get_label(s);
    if (res != 0) {
        return res;
    }
    return this->temp_labs.get(s);
}

string LibraryToolbox::resolve_symbol(SymTok tok) const
{
    auto res = this->lib.resolve_symbol(tok);
    if (res != "") {
        return res;
    }
    return this->temp_syms.resolve(tok);
}

string LibraryToolbox::resolve_label(LabTok tok) const
{
    auto res = this->lib.resolve_label(tok);
    if (res != "") {
        return res;
    }
    return this->temp_labs.resolve(tok);
}

size_t LibraryToolbox::get_symbols_num() const
{
    return this->lib.get_symbols_num() + this->temp_syms.size();
}

size_t LibraryToolbox::get_labels_num() const
{
    return this->lib.get_labels_num() + this->temp_labs.size();
}

bool LibraryToolbox::is_constant(SymTok c) const
{
    // Things in temp_* can only ve variable
    return this->lib.is_constant(c);
}

const Sentence &LibraryToolbox::get_sentence(LabTok label) const
{
    try {
        return this->lib.get_sentence(label);
    } catch (out_of_range) {
        return this->temp_types.at(label);
    }
}

const Assertion &LibraryToolbox::get_assertion(LabTok label) const
{
    return this->lib.get_assertion(label);
}

std::function<const Assertion *()> LibraryToolbox::list_assertions() const
{
    return this->lib.list_assertions();
}

const StackFrame &LibraryToolbox::get_final_stack_frame() const
{
    return this->lib.get_final_stack_frame();
}

const LibraryAddendum &LibraryToolbox::get_addendum() const
{
    return this->lib.get_addendum();
}

const ParsingAddendumImpl &LibraryToolbox::get_parsing_addendum() const
{
    return this->lib.get_parsing_addendum();
}

string LibraryToolbox::get_digest() const
{
    return this->lib.get_digest();
}

/*Prover cascade_provers(const Prover &a,  const Prover &b)
{
    return [=](ProofEngine &engine) {
        bool res;
        res = a(engine);
        if (res) {
            return true;
        }
        res = b(engine);
        return res;
    };
}*/

std::vector<SymTok> LibraryToolbox::read_sentence(const string &in) const
{
    auto toks = tokenize(in);
    vector< SymTok > res;
    for (auto &tok : toks) {
        auto tok_num = this->get_symbol(tok);
        assert_or_throw(tok_num != 0);
        res.push_back(tok_num);
    }
    return res;
}

SentencePrinter LibraryToolbox::print_sentence(const std::vector<SymTok> &sent, SentencePrinter::Style style) const
{
    return SentencePrinter({ true, sent, {}, *this, style });
}

SentencePrinter LibraryToolbox::print_sentence(const ParsingTree<SymTok, LabTok> &pt, SentencePrinter::Style style) const
{
    return SentencePrinter({ false, {}, pt, *this, style });
}

ProofPrinter LibraryToolbox::print_proof(const std::vector<LabTok> &proof) const
{
    return ProofPrinter({ proof, *this });
}

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > > LibraryToolbox::unify_assertion_uncached(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first, bool up_to_hyps_perms) const
{
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > ret;

    vector< SymTok > sent;
    for (auto &hyp : hypotheses) {
        copy(hyp.begin(), hyp.end(), back_inserter(sent));
        sent.push_back(0);
    }
    copy(thesis.begin(), thesis.end(), back_inserter(sent));

    auto assertions_gen = this->list_assertions();
    while (true) {
        const Assertion *ass2 = assertions_gen();
        if (ass2 == NULL) {
            break;
        }
        const Assertion &ass = *ass2;
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
                auto &hyp = this->get_sentence(ass.get_ess_hyps()[perm[i]]);
                copy(hyp.begin(), hyp.end(), back_inserter(templ));
                templ.push_back(0);
            }
            auto &th = this->get_sentence(ass.get_thesis());
            copy(th.begin(), th.end(), back_inserter(templ));
            auto unifications = unify(sent, templ, *this);
            if (!unifications.empty()) {
                for (auto &unification : unifications) {
                    // Here we have to check that substitutions are of the corresponding type
                    // TODO - Here we immediately drop the type information, which probably mean that later we have to compute it again
                    bool wrong_unification = false;
                    for (auto &float_hyp : ass.get_float_hyps()) {
                        const Sentence &float_hyp_sent = this->get_sentence(float_hyp);
                        Sentence type_sent;
                        type_sent.push_back(float_hyp_sent.at(0));
                        auto &type_main_sent = unification.at(float_hyp_sent.at(1));
                        copy(type_main_sent.begin(), type_main_sent.end(), back_inserter(type_sent));
                        ProofEngine engine(*this);
                        if (!this->classical_type_proving_helper(type_sent, engine)) {
                            wrong_unification = true;
                            break;
                        }
                    }
                    // If this is the case, then we have found a legitimate unification
                    if (!wrong_unification) {
                        ret.emplace_back(ass.get_thesis(), perm, unification);
                        if (just_first) {
                            return ret;
                        }
                    }
                }
            }
            if (!up_to_hyps_perms) {
                break;
            }
        } while (next_permutation(perm.begin(), perm.end()));
    }

    return ret;
}

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > > LibraryToolbox::unify_assertion_uncached2(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first, bool up_to_hyps_perms) const
{
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > ret;

    // Parse inputs
    vector< ParsingTree< SymTok, LabTok > > pt_hyps;
    for (auto &hyp : hypotheses) {
        pt_hyps.push_back(this->parse_sentence(hyp.begin()+1, hyp.end(), this->turnstile_alias));
    }
    ParsingTree< SymTok, LabTok > pt_thesis = this->parse_sentence(thesis.begin()+1, thesis.end(), this->turnstile_alias);

    auto assertions_gen = this->list_assertions();
    const std::function< bool(LabTok) > is_var = this->get_standard_is_var();
    while (true) {
        const Assertion *ass2 = assertions_gen();
        if (ass2 == NULL) {
            break;
        }
        const Assertion &ass = *ass2;
        if (ass.is_usage_disc()) {
            continue;
        }
        if (ass.get_ess_hyps().size() != hypotheses.size()) {
            continue;
        }
        if (thesis[0] != this->get_sentence(ass.get_thesis())[0]) {
            continue;
        }
        std::unordered_map< LabTok, ParsingTree< SymTok, LabTok > > thesis_subst;
        auto &templ_pt = this->get_parsed_sents().at(ass.get_thesis());
        bool res = unify(templ_pt, pt_thesis, is_var, thesis_subst);
        if (!res) {
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
            std::unordered_map< LabTok, ParsingTree< SymTok, LabTok > > subst = thesis_subst;
            res = true;
            for (size_t i = 0; i < hypotheses.size(); i++) {
                res = (hypotheses[i][0] == this->get_sentence(ass.get_ess_hyps()[perm[i]])[0]);
                if (!res) {
                    break;
                }
                auto &templ_pt = this->get_parsed_sents().at(ass.get_ess_hyps()[perm[i]]);
                res = unify(templ_pt, pt_hyps[i], is_var, subst);
                if (!res) {
                    break;
                }
            }
            if (!res) {
                continue;
            }
            std::unordered_map< SymTok, std::vector< SymTok > > subst2;
            for (auto &s : subst) {
                subst2.insert(make_pair(this->get_sentence(s.first).at(1), this->reconstruct_sentence(s.second)));
            }
            ret.emplace_back(ass.get_thesis(), perm, subst2);
            if (just_first) {
                return ret;
            }
            if (!up_to_hyps_perms) {
                break;
            }
        } while (next_permutation(perm.begin(), perm.end()));
    }

    return ret;
}

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > > LibraryToolbox::unify_assertion_cached(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first)
{
    // Cache is used only when requesting just the first result
    if (!just_first) {
        return this->unify_assertion_uncached(hypotheses, thesis, just_first);
    }
    auto idx = make_tuple(hypotheses, thesis);
    if (this->unification_cache.find(idx) == this->unification_cache.end()) {
        this->unification_cache[idx] = this->unify_assertion_uncached(hypotheses, thesis, just_first);
    }
    return this->unification_cache.at(idx);
}

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > > LibraryToolbox::unify_assertion_cached(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first) const
{
    // Cache is used only when requesting just the first result
    if (!just_first) {
        return this->unify_assertion_uncached(hypotheses, thesis, just_first);
    }
    auto idx = make_tuple(hypotheses, thesis);
    if (this->unification_cache.find(idx) == this->unification_cache.end()) {
        return this->unify_assertion_uncached(hypotheses, thesis, just_first);
    }
    return this->unification_cache.at(idx);
}

const std::function<bool (LabTok)> LibraryToolbox::get_standard_is_var() const {
    return [&](LabTok x)->bool {
        const auto &types_set = this->get_types_set();
        if (types_set.find(x) == types_set.end()) {
            return false;
        }
        return !this->is_constant(this->get_sentence(x).at(1));
    };
}

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, std::vector<SymTok> > > > LibraryToolbox::unify_assertion(const std::vector<std::vector<SymTok> > &hypotheses, const std::vector<SymTok> &thesis, bool just_first, bool up_to_hyps_perms) const
{
    auto ret2 = this->unify_assertion_uncached2(hypotheses, thesis, just_first, up_to_hyps_perms);
#ifdef TOOLBOX_SELF_TEST
    auto ret = this->unify_assertion_uncached(hypotheses, thesis, just_first, up_to_hyps_perms);
    assert(ret == ret2);
#endif
    return ret2;
}

void LibraryToolbox::compute_everything()
{
    //cout << "Computing everything" << endl;
    //auto t = tic();
    this->compute_types_by_var();
    this->compute_assertions_by_type();
    this->compute_derivations();
    this->compute_ders_by_label();
    this->compute_parser_initialization();
    this->compute_sentences_parsing();
    this->compute_registered_provers();
    //toc(t, 1);
}

const std::vector<LabTok> &LibraryToolbox::get_types() const
{
    return this->types;
}

const std::set<LabTok> &LibraryToolbox::get_types_set() const
{
    return this->types_set;
}

void LibraryToolbox::compute_types_by_var()
{
    for (auto &type : this->get_types()) {
        const SymTok &var = this->get_sentence(type).at(1);
        this->types_by_var.resize(max(this->types_by_var.size(), (size_t) var+1));
        this->types_by_var[var] = type;
    }
    this->types_by_var_computed = true;
}

const std::vector<LabTok> &LibraryToolbox::get_types_by_var()
{
    if (!this->types_by_var_computed) {
        this->compute_types_by_var();
    }
    return this->types_by_var;
}

const std::vector<LabTok> &LibraryToolbox::get_types_by_var() const
{
    if (!this->types_by_var_computed) {
        throw MMPPException("computation required on const object");
    }
    return this->types_by_var;
}

void LibraryToolbox::compute_assertions_by_type()
{
    auto assertions_gen = this->list_assertions();
    while (true) {
        const Assertion *ass2 = assertions_gen();
        if (ass2 == NULL) {
            break;
        }
        const Assertion &ass = *ass2;
        const auto &label = ass.get_thesis();
        this->assertions_by_type[this->get_sentence(label).at(0)].push_back(label);
    }
    this->assertions_by_type_computed = true;
}

const std::unordered_map<SymTok, std::vector<LabTok> > &LibraryToolbox::get_assertions_by_type()
{
    if (!this->assertions_by_type_computed) {
        this->compute_assertions_by_type();
    }
    return this->assertions_by_type;
}

const std::unordered_map<SymTok, std::vector<LabTok> > &LibraryToolbox::get_assertions_by_type() const
{
    if (!this->assertions_by_type_computed) {
        throw MMPPException("computation required on const object");
    }
    return this->assertions_by_type;
}

void LibraryToolbox::compute_derivations()
{
    // Build the derivation rules; a derivation is created for each $f statement
    // and for each $a and $p statement without essential hypotheses such that no variable
    // appears more than once and without distinct variables constraints and that does not
    // begin with the turnstile
    for (auto &type_lab : this->get_types()) {
        auto &type_sent = this->get_sentence(type_lab);
        this->derivations[type_sent.at(0)].push_back(make_pair(type_lab, vector<SymTok>({type_sent.at(1)})));
    }
    // FIXME Take it from the configuration
    auto assertions_gen = this->list_assertions();
    while (true) {
        const Assertion *ass2 = assertions_gen();
        if (ass2 == NULL) {
            break;
        }
        const Assertion &ass = *ass2;
        if (ass.get_ess_hyps().size() != 0) {
            continue;
        }
        if (ass.get_mand_dists().size() != 0) {
            continue;
        }
        const auto &sent = this->get_sentence(ass.get_thesis());
        if (sent.at(0) == this->turnstile) {
            continue;
        }
        set< SymTok > symbols;
        bool duplicate = false;
        for (const auto &tok : sent) {
            if (this->is_constant(tok)) {
                continue;
            }
            auto res = symbols.insert(tok);
            if (!res.second) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            continue;
        }
        vector< SymTok > sent2;
        for (size_t i = 1; i < sent.size(); i++) {
            const auto &tok = sent[i];
            // Variables are replaced with their types, which act as variables of the context-free grammar
            sent2.push_back(this->is_constant(tok) ? tok : this->get_sentence(this->get_types_by_var().at(tok)).at(0));
        }
        this->derivations[sent.at(0)].push_back(make_pair(ass.get_thesis(), sent2));
    }
    this->derivations_computed = true;
}

RegisteredProver LibraryToolbox::register_prover(const std::vector<string> &templ_hyps, const string &templ_thesis)
{
    size_t index = LibraryToolbox::registered_provers().size();
    auto &rps = LibraryToolbox::registered_provers();
    rps.push_back({templ_hyps, templ_thesis});
    //cerr << "first: " << &rps << "; size: " << rps.size() << endl;
    return { index, templ_hyps, templ_thesis };
}

Prover LibraryToolbox::build_registered_prover(const RegisteredProver &prover, const std::unordered_map<string, Prover> &types_provers, const std::vector<Prover> &hyps_provers) const
{
    const size_t &index = prover.index;
    if (index >= this->instance_registered_provers.size() || !this->instance_registered_provers[index].valid) {
        throw MMPPException("cannot modify const object");
    }
    const RegisteredProverInstanceData &inst_data = this->instance_registered_provers[index];

    return [=](ProofEngine &engine){
        return this->proving_helper(inst_data, types_provers, hyps_provers, engine);
    };
}

void LibraryToolbox::compute_registered_provers()
{
    for (size_t index = 0; index < LibraryToolbox::registered_provers().size(); index++) {
        this->compute_registered_prover(index);
    }
    //cerr << "Computed " << LibraryToolbox::registered_provers().size() << " registered provers" << endl;
}

void LibraryToolbox::compute_parser_initialization()
{
    delete this->parser;
    this->parser = NULL;
    std::function< std::ostream&(std::ostream&, SymTok) > sym_printer = [&](ostream &os, SymTok sym)->ostream& { return os << this->resolve_symbol(sym); };
    std::function< std::ostream&(std::ostream&, LabTok) > lab_printer = [&](ostream &os, LabTok lab)->ostream& { return os << this->resolve_label(lab); };
    this->parser = new LRParser< SymTok, LabTok >(this->get_derivations(), sym_printer, lab_printer);
    bool loaded = false;
    if (this->cache != NULL) {
        if (this->cache->load()) {
            if (this->get_digest() == this->cache->get_digest()) {
                this->parser->set_cached_data(this->cache->get_lr_parser_data());
                loaded = true;
            }
        }
    }
    if (!loaded) {
        this->parser->initialize();
        if (this->cache != NULL) {
            this->cache->set_digest(this->get_digest());
            this->cache->set_lr_parser_data(this->parser->get_cached_data());
            this->cache->store();
        }
    }
    // Drop the cache so that memory can be recovered
    this->cache = NULL;
    this->parser_initialization_computed = true;
}

const LRParser<SymTok, LabTok> &LibraryToolbox::get_parser()
{
    if (!this->parser_initialization_computed) {
        this->compute_parser_initialization();
    }
    return *this->parser;
}

const LRParser<SymTok, LabTok> &LibraryToolbox::get_parser() const
{
    if (!this->parser_initialization_computed) {
        throw MMPPException("computation required on const object");
    }
    return *this->parser;
}

ParsingTree< SymTok, LabTok > LibraryToolbox::parse_sentence(typename vector<SymTok>::const_iterator sent_begin, typename vector<SymTok>::const_iterator sent_end, SymTok type) const
{
    return this->get_parser().parse(sent_begin, sent_end, type);
}

ParsingTree< SymTok, LabTok > LibraryToolbox::parse_sentence(const std::vector<SymTok> &sent, SymTok type) const
{
    return this->get_parser().parse(sent, type);
}

ParsingTree< SymTok, LabTok > LibraryToolbox::parse_sentence(typename vector<SymTok>::const_iterator sent_begin, typename vector<SymTok>::const_iterator sent_end, SymTok type)
{
    return this->get_parser().parse(sent_begin, sent_end, type);
}

ParsingTree< SymTok, LabTok > LibraryToolbox::parse_sentence(const std::vector<SymTok> &sent, SymTok type)
{
    return this->get_parser().parse(sent, type);
}

void LibraryToolbox::compute_sentences_parsing()
{
    /*if (!this->parser_initialization_computed) {
        this->compute_parser_initialization();
    }*/
    for (size_t i = 1; i < this->get_labels_num()+1; i++) {
        const Sentence &sent = this->get_sentence(i);
        auto pt = this->parse_sentence(sent.begin()+1, sent.end(), this->get_parsing_addendum().get_syntax().at(sent[0]));
        if (pt.label == 0) {
            throw MMPPException("Failed to parse a sentence in the library");
        }
        this->parsed_sents.resize(i+1);
        this->parsed_sents[i] = pt;
    }
    this->sentences_parsing_computed = true;
}

const std::vector<ParsingTree<SymTok, LabTok> > &LibraryToolbox::get_parsed_sents()
{
    if (!this->sentences_parsing_computed) {
        this->compute_sentences_parsing();
    }
    return this->parsed_sents;
}

const std::vector<ParsingTree<SymTok, LabTok> > &LibraryToolbox::get_parsed_sents() const
{
    if (!this->sentences_parsing_computed) {
        throw MMPPException("computation required on const object");
    }
    return this->parsed_sents;
}

void LibraryToolbox::compute_registered_prover(size_t index)
{
    this->instance_registered_provers.resize(LibraryToolbox::registered_provers().size());
    const RegisteredProverData &data = LibraryToolbox::registered_provers()[index];
    RegisteredProverInstanceData &inst_data = this->instance_registered_provers[index];
    if (!inst_data.valid) {
        // Decode input strings to sentences
        std::vector<std::vector<SymTok> > templ_hyps_sent;
        for (auto &hyp : data.templ_hyps) {
            templ_hyps_sent.push_back(this->read_sentence(hyp));
        }
        std::vector<SymTok> templ_thesis_sent = this->read_sentence(data.templ_thesis);

        auto unification = this->unify_assertion(templ_hyps_sent, templ_thesis_sent, true);
        assert_or_throw(!unification.empty(), "Could not find the template assertion");
        inst_data.label = get<0>(*unification.begin());
        inst_data.label_str = this->resolve_label(inst_data.label);
        const Assertion &ass = this->get_assertion(inst_data.label);
        assert(ass.is_valid());
        const vector< size_t > &perm = get<1>(*unification.begin());
        inst_data.perm_inv = invert_perm(perm);
        inst_data.ass_map = get<2>(*unification.begin());
        //const unordered_map< SymTok, vector< SymTok > > full_map = this->compose_subst(ass_map, subst_map);
        inst_data.valid = true;
    }
}

Prover LibraryToolbox::build_classical_type_prover(const std::vector<SymTok> &type_sent, const std::unordered_map<SymTok, Prover> &var_provers) const
{
    return [=](ProofEngine &engine){
        return this->classical_type_proving_helper(type_sent, engine, var_provers);
    };
}

Prover LibraryToolbox::build_earley_type_prover(const std::vector<SymTok> &type_sent, const std::unordered_map<SymTok, Prover> &var_provers) const
{
    return [=](ProofEngine &engine){
        return this->earley_type_proving_helper(type_sent, engine, var_provers);
    };
}

Prover LibraryToolbox::build_type_prover(const std::vector<SymTok> &type_sent, const std::unordered_map<SymTok, Prover> &var_provers) const
{
    return LibraryToolbox::build_classical_type_prover(type_sent, var_provers);
}

string SentencePrinter::to_string() const
{
    ostringstream buf;
    buf << *this;
    return buf.str();
}

string test_prover(Prover prover, const LibraryToolbox &tb) {
    ProofEngine engine(tb);
    bool res = prover(engine);
    if (!res) {
        return "(prover failed)";
    } else {
        if (engine.get_stack().size() == 0) {
            return "(prover did not produce a result)";
        } else {
            string ret = tb.print_sentence(engine.get_stack().back()).to_string();
            if (engine.get_stack().size() > 1) {
                ret += " (prover did produce other stack entries)";
            }
            return ret;
        }
    }
}

ToolboxCache::~ToolboxCache()
{
}

FileToolboxCache::FileToolboxCache(const boost::filesystem::path &filename) : filename(filename) {
}

bool FileToolboxCache::load() {
    boost::filesystem::ifstream lr_fin(this->filename);
    if (lr_fin.fail()) {
        return false;
    }
    boost::archive::text_iarchive archive(lr_fin);
    archive >> this->digest;
    archive >> this->lr_parser_data;
    return true;
}

bool FileToolboxCache::store() {
    boost::filesystem::ofstream lr_fout(this->filename);
    if (lr_fout.fail()) {
        return false;
    }
    boost::archive::text_oarchive archive(lr_fout);
    archive << this->digest;
    archive << this->lr_parser_data;
    return true;
}

string FileToolboxCache::get_digest() {
    return this->digest;
}

void FileToolboxCache::set_digest(string digest) {
    this->digest = digest;
}

LRParser< SymTok, LabTok >::CachedData FileToolboxCache::get_lr_parser_data() {
    return this->lr_parser_data;
}

void FileToolboxCache::set_lr_parser_data(const LRParser< SymTok, LabTok >::CachedData &cached_data) {
    this->lr_parser_data = cached_data;
}
