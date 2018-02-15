
#include <boost/filesystem/fstream.hpp>

#include "toolbox.h"
#include "utils/utils.h"
#include "old/unification.h"
#include "parsing/unif.h"
#include "parsing/earley.h"
#include "reader.h"
#include "mm/proof.h"

using namespace std;

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
    if (sp.sent != NULL) {
        sentp = sp.sent;
    } else if (sp.pt != NULL) {
        sent2 = sp.tb.reconstruct_sentence(*sp.pt);
        sentp = &sent2;
    } else if (sp.pt2 != NULL) {
        sent2 = sp.tb.reconstruct_sentence(pt2_to_pt(*sp.pt2));
        sentp = &sent2;
    } else {
        assert("Should never arrive here" == NULL);
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
        } else if (sp.style == SentencePrinter::STYLE_NUMBERS) {
            os << tok;
        } else if (sp.style == SentencePrinter::STYLE_HTML) {
            os << sp.tb.get_addendum().get_htmldef(tok);
        } else if (sp.style == SentencePrinter::STYLE_ALTHTML) {
            os << sp.tb.get_addendum().get_althtmldef(tok);
        } else if (sp.style == SentencePrinter::STYLE_LATEX) {
            os << sp.tb.get_addendum().get_latexdef(tok);
        } else if (sp.style == SentencePrinter::STYLE_ANSI_COLORS_SET_MM) {
            if (sp.tb.get_standard_is_var_sym()(tok)) {
                string type_str = sp.tb.resolve_symbol(sp.tb.get_var_sym_to_type_sym(tok));
                if (type_str == "set") {
                    os << "\033[91m";
                } else if (type_str == "class") {
                    os << "\033[35m";
                } else if (type_str == "wff") {
                    os << "\033[94m";
                }
            } else {
                //os << "\033[37m";
            }
            os << sp.tb.resolve_symbol(tok);
            os << "\033[39m";
        }
    }
    if (sp.style == SentencePrinter::STYLE_ALTHTML) {
        os << "</SPAN>";
    }
    return os;
}

ostream &operator<<(ostream &os, const ProofPrinter &sp)
{
    auto labels = sp.labels;
    if (sp.uncomp_proof) {
        labels = &sp.uncomp_proof->get_labels();
    }
    if (labels) {
        bool first = true;
        for (const auto &label : *labels) {
            if (sp.only_assertions && !(sp.tb.get_assertion(label).is_valid() && sp.tb.get_sentence(label).at(0) == sp.tb.get_turnstile())) {
                continue;
            }
            if (first) {
                first = false;
            } else {
                os << string(" ");
            }
            os << sp.tb.resolve_label(label);
        }
    } else {
        os << "(";
        for (const auto &ref : sp.comp_proof->get_refs()) {
            os << " " << sp.tb.resolve_label(ref);
        }
        os << " ) ";
        CompressedEncoder enc;
        for (const auto &code : sp.comp_proof->get_codes()) {
            os << enc.push_code(code);
        }
    }
    return os;
}

LibraryToolbox::LibraryToolbox(const ExtendedLibrary &lib, string turnstile, shared_ptr<ToolboxCache> cache) :
    cache(cache),
    lib(lib),
    turnstile(lib.get_symbol(turnstile)), turnstile_alias(lib.get_parsing_addendum().get_syntax().at(this->turnstile)),
    temp_generator(make_unique< TempGenerator >(lib))
    //type_labels(lib.get_final_stack_frame().types), type_labels_set(lib.get_final_stack_frame().types_set)
{
    assert(this->lib.is_immutable());
    this->standard_is_var = [this](LabTok x)->bool {
        /*const auto &types_set = this->get_types_set();
        if (types_set.find(x) == types_set.end()) {
            return false;
        }
        return !this->is_constant(this->get_sentence(x).at(1));*/
        if (x > this->lib.get_labels_num()) {
            return true;
        }
        return this->get_is_var_by_type().at(x);
    };
    this->standard_is_var_sym = [this](SymTok x)->bool {
        if (x > this->lib.get_symbols_num()) {
            return true;
        }
        return !this->lib.is_constant(x);
    };
    this->compute_everything();
}

const ExtendedLibrary &LibraryToolbox::get_library() const {
    return this->lib;
}

const std::unordered_map<SymTok, std::vector<std::pair<LabTok, std::vector<SymTok> > > > &LibraryToolbox::get_derivations() const
{
    return this->derivations;
}

void LibraryToolbox::compute_ders_by_label()
{
    this->ders_by_label = compute_derivations_by_label(this->get_derivations());
}

/* We reimplement, instead of use, the function ::recostuct_sentence(),
 * in order to support variables introduced by temp_generator.
 */
Sentence LibraryToolbox::reconstruct_sentence(const ParsingTree<SymTok, LabTok> &pt, SymTok first_sym) const
{
    std::vector< SymTok > res;
    if (first_sym != SymTok{}) {
        res.push_back(first_sym);
    }
    this->reconstruct_sentence_internal(pt, back_inserter(res));
    return res;
}

void LibraryToolbox::reconstruct_sentence_internal(const ParsingTree<SymTok, LabTok> &pt, std::back_insert_iterator<std::vector<SymTok> > it) const
{
    const auto &rule = this->get_derivation_rule(pt.label);
    auto pt_it = pt.children.begin();
    for (const auto &sym : rule.second) {
        if (this->derivations.find(sym) == this->derivations.end()) {
            it = sym;
        } else {
            assert(pt_it != pt.children.end());
            this->reconstruct_sentence_internal(*pt_it, it);
            pt_it++;
        }
    }
    assert(pt_it == pt.children.end());
}

void LibraryToolbox::compute_vars()
{
    this->sentence_vars.emplace_back();
    for (size_t i = 1; i < this->get_parsed_sents2().size(); i++) {
        const auto &pt = this->get_parsed_sents2()[i];
        set< LabTok > vars;
        collect_variables2(pt, this->get_standard_is_var(), vars);
        this->sentence_vars.push_back(vars);
    }
    for (const auto &ass : this->lib.get_assertions()) {
        if (!ass.is_valid()) {
            this->assertion_const_vars.emplace_back();
            this->assertion_unconst_vars.emplace_back();
            continue;
        }
        const auto &thesis_vars = this->sentence_vars[ass.get_thesis()];
        set< LabTok > hyps_vars;
        for (const auto hyp_tok : ass.get_ess_hyps()) {
            const auto &hyp_vars = this->sentence_vars[hyp_tok];
            hyps_vars.insert(hyp_vars.begin(), hyp_vars.end());
        }
        this->assertion_const_vars.push_back(thesis_vars);
        auto &unconst_vars = this->assertion_unconst_vars.emplace_back();
        set_difference(hyps_vars.begin(), hyps_vars.end(), thesis_vars.begin(), thesis_vars.end(), inserter(unconst_vars, unconst_vars.begin()));
    }
}

const std::vector< std::set< LabTok > > &LibraryToolbox::get_sentence_vars() const
{
    return this->sentence_vars;
}

const std::vector< std::set< LabTok > > &LibraryToolbox::get_assertion_unconst_vars() const
{
    return this->assertion_unconst_vars;
}

const std::vector< std::set< LabTok > > &LibraryToolbox::get_assertion_const_vars() const
{
    return this->assertion_const_vars;
}

void LibraryToolbox::compute_labels_to_theses()
{
    LabTok imp_label = this->get_imp_label();
    bool imp_found = imp_label != 0;
    for (const Assertion &ass : this->lib.get_assertions()) {
        if (!ass.is_valid() || this->get_sentence(ass.get_thesis()).at(0) != this->get_turnstile()) {
            continue;
        }
        const auto &pt = this->get_parsed_sents().at(ass.get_thesis());
        LabTok root_label = pt.label;
        if (this->get_standard_is_var()(root_label)) {
            root_label = {};
        }
        // FIXME The following is set.mm specific
        if (imp_found && root_label == imp_label) {
            LabTok ant_label = pt.children.at(0).label;
            LabTok con_label = pt.children.at(1).label;
            if (this->get_standard_is_var()(ant_label)) {
                ant_label = 0;
            }
            if (this->get_standard_is_var()(con_label)) {
                con_label = 0;
            }
            this->imp_ant_labels_to_theses[ant_label].push_back(ass.get_thesis());
            this->imp_con_labels_to_theses[con_label].push_back(ass.get_thesis());
        } else {
            this->root_labels_to_theses[root_label].push_back(ass.get_thesis());
        }
    }
}

const std::unordered_map<LabTok, std::vector<LabTok> > &LibraryToolbox::get_root_labels_to_theses() const
{
    return this->root_labels_to_theses;
}

const std::unordered_map<LabTok, std::vector<LabTok> > &LibraryToolbox::get_imp_ant_labels_to_theses() const
{
    return this->imp_ant_labels_to_theses;
}

const std::unordered_map<LabTok, std::vector<LabTok> > &LibraryToolbox::get_imp_con_labels_to_theses() const
{
    return this->imp_con_labels_to_theses;
}

// FIXME Deduplicate with refresh_parsing_tree()
std::pair<std::vector<ParsingTree<SymTok, LabTok> >, ParsingTree<SymTok, LabTok> > LibraryToolbox::refresh_assertion(const Assertion &ass)
{
    // Collect all variables
    std::set< LabTok > var_labs;
    const auto &is_var = this->get_standard_is_var();
    const ParsingTree< SymTok, LabTok > &thesis_pt = this->get_parsed_sents().at(ass.get_thesis());
    collect_variables(thesis_pt, is_var, var_labs);
    for (const auto hyp : ass.get_ess_hyps()) {
        const ParsingTree< SymTok, LabTok > &hyp_pt = this->get_parsed_sents().at(hyp);
        collect_variables(hyp_pt, is_var, var_labs);
    }

    // Build a substitution map
    SubstMap< SymTok, LabTok > subst = this->build_refreshing_subst_map(var_labs);

    // Substitute and return
    ParsingTree< SymTok, LabTok > thesis_new_pt = ::substitute(thesis_pt, is_var, subst);
    vector< ParsingTree< SymTok, LabTok > > hyps_new_pts;
    for (const auto hyp : ass.get_ess_hyps()) {
        const ParsingTree< SymTok, LabTok > &hyp_pt = this->get_parsed_sents().at(hyp);
        hyps_new_pts.push_back(::substitute(hyp_pt, is_var, subst));
    }
    return make_pair(hyps_new_pts, thesis_new_pt);
}

std::pair<std::vector<ParsingTree2<SymTok, LabTok> >, ParsingTree2<SymTok, LabTok> > LibraryToolbox::refresh_assertion2(const Assertion &ass)
{
    // Collect all variables
    std::set< LabTok > var_labs;
    const auto &is_var = this->get_standard_is_var();
    const ParsingTree2< SymTok, LabTok > &thesis_pt = this->get_parsed_sents2().at(ass.get_thesis());
    collect_variables2(thesis_pt, is_var, var_labs);
    for (const auto hyp : ass.get_ess_hyps()) {
        const ParsingTree2< SymTok, LabTok > &hyp_pt = this->get_parsed_sents2().at(hyp);
        collect_variables2(hyp_pt, is_var, var_labs);
    }

    // Build a substitution map
    SimpleSubstMap2< SymTok, LabTok > subst = this->build_refreshing_subst_map2(var_labs);

    // Substitute and return
    ParsingTree2< SymTok, LabTok > thesis_new_pt = ::substitute2_simple(thesis_pt, is_var, subst);
    vector< ParsingTree2< SymTok, LabTok > > hyps_new_pts;
    for (const auto hyp : ass.get_ess_hyps()) {
        const ParsingTree2< SymTok, LabTok > &hyp_pt = this->get_parsed_sents2().at(hyp);
        hyps_new_pts.push_back(::substitute2_simple(hyp_pt, is_var, subst));
    }
    return make_pair(hyps_new_pts, thesis_new_pt);
}

ParsingTree<SymTok, LabTok> LibraryToolbox::refresh_parsing_tree(const ParsingTree<SymTok, LabTok> &pt)
{
    // Collect all variables
    std::set< LabTok > var_labs;
    const auto &is_var = this->get_standard_is_var();
    collect_variables(pt, is_var, var_labs);

    // Build a substitution map
    SubstMap< SymTok, LabTok > subst = this->build_refreshing_subst_map(var_labs);

    // Substitute and return
    return ::substitute(pt, is_var, subst);
}

ParsingTree2<SymTok, LabTok> LibraryToolbox::refresh_parsing_tree2(const ParsingTree2<SymTok, LabTok> &pt)
{
    // Collect all variables
    std::set< LabTok > var_labs;
    const auto &is_var = this->get_standard_is_var();
    collect_variables2(pt, is_var, var_labs);

    // Build a substitution map
    SimpleSubstMap2< SymTok, LabTok > subst = this->build_refreshing_subst_map2(var_labs);

    // Substitute and return
    return ::substitute2_simple(pt, is_var, subst);
}

SubstMap<SymTok, LabTok> LibraryToolbox::build_refreshing_subst_map(const std::set<LabTok> &vars)
{
    SubstMap< SymTok, LabTok > subst;
    for (const auto var : vars) {
        SymTok type_sym = this->get_sentence(var).at(0);
        SymTok new_sym;
        LabTok new_lab;
        tie(new_lab, new_sym) = this->new_temp_var(type_sym);
        ParsingTree< SymTok, LabTok > new_pt;
        new_pt.label = new_lab;
        new_pt.type = type_sym;
        subst[var] = new_pt;
    }
    return subst;
}

SimpleSubstMap2<SymTok, LabTok> LibraryToolbox::build_refreshing_subst_map2(const std::set<LabTok> &vars)
{
    SimpleSubstMap2< SymTok, LabTok > subst;
    for (const auto var : vars) {
        SymTok type_sym = this->get_sentence(var).at(0);
        LabTok new_lab;
        tie(new_lab, ignore) = this->new_temp_var(type_sym);
        subst[var] = new_lab;
    }
    return subst;
}

SubstMap2<SymTok, LabTok> LibraryToolbox::build_refreshing_full_subst_map2(const std::set<LabTok> &vars)
{
    return subst_to_subst2(this->build_refreshing_subst_map(vars));
}

SymTok LibraryToolbox::get_symbol(string s) const
{
    auto res = this->lib.get_symbol(s);
    if (res != 0) {
        return res;
    }
    return this->temp_generator->get_symbol(s);
}

LabTok LibraryToolbox::get_label(string s) const
{
    auto res = this->lib.get_label(s);
    if (res != 0) {
        return res;
    }
    return this->temp_generator->get_label(s);
}

string LibraryToolbox::resolve_symbol(SymTok tok) const
{
    auto res = this->lib.resolve_symbol(tok);
    if (res != "") {
        return res;
    }
    return this->temp_generator->resolve_symbol(tok);
}

string LibraryToolbox::resolve_label(LabTok tok) const
{
    auto res = this->lib.resolve_label(tok);
    if (res != "") {
        return res;
    }
    return this->temp_generator->resolve_label(tok);
}

size_t LibraryToolbox::get_symbols_num() const
{
    return this->lib.get_symbols_num() + this->temp_generator->get_symbols_num();
}

size_t LibraryToolbox::get_labels_num() const
{
    return this->lib.get_labels_num() + this->temp_generator->get_labels_num();
}

bool LibraryToolbox::is_constant(SymTok c) const
{
    // Things in temp_generator can only be variable
    return this->lib.is_constant(c);
}

const Sentence &LibraryToolbox::get_sentence(LabTok label) const
{
    const Sentence *sent = this->lib.get_sentence_ptr(label);
    if (sent != NULL) {
        return *sent;
    }
    return this->temp_generator->get_sentence(label);
}

SentenceType LibraryToolbox::get_sentence_type(LabTok label) const
{
    return this->lib.get_sentence_type(label);
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

std::vector<SymTok> LibraryToolbox::read_sentence(const string &in) const
{
    auto toks = tokenize(in);
    vector< SymTok > res;
    for (auto &tok : toks) {
        auto tok_num = this->get_symbol(tok);
        assert_or_throw< MMPPException >(tok_num != 0, "not a symbol");
        res.push_back(tok_num);
    }
    return res;
}

SentencePrinter LibraryToolbox::print_sentence(const std::vector<SymTok> &sent, SentencePrinter::Style style) const
{
    return SentencePrinter({ &sent, {}, {}, *this, style });
}

SentencePrinter LibraryToolbox::print_sentence(const ParsingTree<SymTok, LabTok> &pt, SentencePrinter::Style style) const
{
    return SentencePrinter({ {}, &pt, {}, *this, style });
}

SentencePrinter LibraryToolbox::print_sentence(const ParsingTree2<SymTok, LabTok> &pt, SentencePrinter::Style style) const
{
    return SentencePrinter({ {}, {}, &pt, *this, style });
}

ProofPrinter LibraryToolbox::print_proof(const std::vector<LabTok> &proof, bool only_assertions) const
{
    return ProofPrinter({ &proof, {}, {}, *this, only_assertions });
}

ProofPrinter LibraryToolbox::print_proof(const CompressedProof &proof, bool only_assertions) const
{
    return ProofPrinter({ {}, &proof, {}, *this, only_assertions });
}

ProofPrinter LibraryToolbox::print_proof(const UncompressedProof &proof, bool only_assertions) const
{
    return ProofPrinter({ {}, {}, &proof, *this, only_assertions });
}

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, Sentence> > > LibraryToolbox::unify_assertion_uncached(const std::vector<Sentence> &hypotheses, const Sentence &thesis, bool just_first, bool up_to_hyps_perms) const
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
            auto unifications = unify_old(sent, templ, *this);
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
                        ExtendedProofEngine< Sentence > engine(*this);
                        if (!this->type_proving_helper(type_sent, engine)) {
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

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, Sentence> > > LibraryToolbox::unify_assertion_uncached2(const std::vector<Sentence> &hypotheses, const Sentence &thesis, bool just_first, bool up_to_hyps_perms) const
{
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > ret;

    // Parse inputs
    vector< ParsingTree< SymTok, LabTok > > pt_hyps;
    for (auto &hyp : hypotheses) {
        pt_hyps.push_back(this->parse_sentence(hyp.begin()+1, hyp.end(), this->turnstile_alias));
    }
    ParsingTree< SymTok, LabTok > pt_thesis = this->parse_sentence(thesis.begin()+1, thesis.end(), this->turnstile_alias);

    auto assertions_gen = this->list_assertions();
    const auto &is_var = this->get_standard_is_var();
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
        UnilateralUnificator< SymTok, LabTok > unif(is_var);
        auto &templ_pt = this->get_parsed_sents().at(ass.get_thesis());
        unif.add_parsing_trees(templ_pt, pt_thesis);
        if (!unif.is_unifiable()) {
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
            UnilateralUnificator< SymTok, LabTok > unif2 = unif;
            bool res = true;
            for (size_t i = 0; i < hypotheses.size(); i++) {
                res = (hypotheses[i][0] == this->get_sentence(ass.get_ess_hyps()[perm[i]])[0]);
                if (!res) {
                    break;
                }
                auto &templ_pt = this->get_parsed_sents().at(ass.get_ess_hyps()[perm[i]]);
                unif2.add_parsing_trees(templ_pt, pt_hyps[i]);
                res = unif2.is_unifiable();
                if (!res) {
                    break;
                }
            }
            if (!res) {
                continue;
            }
            SubstMap< SymTok, LabTok > subst;
            tie(res, subst) = unif2.unify();
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

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, Sentence> > > LibraryToolbox::unify_assertion_cached(const std::vector<Sentence> &hypotheses, const Sentence &thesis, bool just_first)
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

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, Sentence> > > LibraryToolbox::unify_assertion_cached(const std::vector<Sentence> &hypotheses, const Sentence &thesis, bool just_first) const
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

const std::function<bool (LabTok)> &LibraryToolbox::get_standard_is_var() const {
    return this->standard_is_var;
}

const std::function<bool (SymTok)> &LibraryToolbox::get_standard_is_var_sym() const
{
    return this->standard_is_var_sym;
}

SymTok LibraryToolbox::get_turnstile() const
{
    return this->turnstile;
}

SymTok LibraryToolbox::get_turnstile_alias() const
{
    return this->turnstile_alias;
}

LabTok LibraryToolbox::get_imp_label() const
{
    return this->get_label("wi");
}

void LibraryToolbox::dump_proof_exception(const ProofException<Sentence> &e, ostream &out) const
{
    out << "Applying " << this->resolve_label(e.get_error().label) << " the proof executor signalled an error..." << endl;
    out << "The reason was " << e.get_reason() << endl;
    out << "On stack there was: " << this->print_sentence(e.get_error().on_stack) << endl;
    out << "Has to match with: " << this->print_sentence(e.get_error().to_subst) << endl;
    out << "Substitution map:" << endl;
    for (const auto &it : e.get_error().subst_map) {
        out << this->resolve_symbol(it.first) << ": " << this->print_sentence(it.second) << endl;
    }
}

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, Sentence> > > LibraryToolbox::unify_assertion(const std::vector<Sentence> &hypotheses, const Sentence &thesis, bool just_first, bool up_to_hyps_perms) const
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
    this->compute_type_correspondance();
    this->compute_is_var_by_type();
    this->compute_assertions_by_type();
    this->compute_derivations();
    this->compute_ders_by_label();
    this->compute_parser_initialization();
    this->compute_sentences_parsing();
    this->compute_labels_to_theses();
    this->compute_registered_provers();
    this->compute_vars();
    //toc(t, 1);
}

/*const std::vector<LabTok> &LibraryToolbox::get_type_labels() const
{
    return this->type_labels;
}

const std::set<LabTok> &LibraryToolbox::get_type_labels_set() const
{
    return this->type_labels_set;
}*/

void LibraryToolbox::compute_type_correspondance()
{
    for (auto &var_lab : this->get_final_stack_frame().types) {
        const auto &sent = this->get_sentence(var_lab);
        assert(sent.size() == 2);
        const SymTok type_sym = sent[0];
        const SymTok var_sym = sent[1];
        enlarge_and_set(this->var_lab_to_sym, var_lab) = var_sym;
        enlarge_and_set(this->var_sym_to_lab, var_sym) = var_lab;
        enlarge_and_set(this->var_lab_to_type_sym, var_lab) = type_sym;
        enlarge_and_set(this->var_sym_to_type_sym, var_sym) = type_sym;
    }
}

LabTok LibraryToolbox::get_var_sym_to_lab(SymTok sym) const
{
    if (sym <= this->lib.get_symbols_num()) {
        return this->var_sym_to_lab.at(sym);
    } else {
        return this->temp_generator->get_var_sym_to_lab(sym);
    }
}

SymTok LibraryToolbox::get_var_lab_to_sym(LabTok lab) const
{
    if (lab <= this->lib.get_labels_num()) {
        return this->var_lab_to_sym.at(lab);
    } else {
        return this->temp_generator->get_var_lab_to_sym(lab);
    }
}

SymTok LibraryToolbox::get_var_sym_to_type_sym(SymTok sym) const
{
    if (sym <= this->lib.get_symbols_num()) {
        return this->var_sym_to_type_sym.at(sym);
    } else {
        return this->temp_generator->get_var_sym_to_type_sym(sym);
    }
}

SymTok LibraryToolbox::get_var_lab_to_type_sym(LabTok lab) const
{
    if (lab <= this->lib.get_labels_num()) {
        return this->var_lab_to_type_sym.at(lab);
    } else {
        return this->temp_generator->get_var_lab_to_type_sym(lab);
    }
}

void LibraryToolbox::compute_is_var_by_type()
{
    const auto &types_set = this->get_final_stack_frame().types_set;
    this->is_var_by_type.resize(this->lib.get_labels_num());
    for (LabTok label = 1; label < this->lib.get_labels_num(); label++) {
        this->is_var_by_type[label] = types_set.find(label) != types_set.end() && !this->is_constant(this->get_sentence(label).at(1));
    }
}

const std::vector<bool> &LibraryToolbox::get_is_var_by_type() const
{
    return this->is_var_by_type;
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
}

const std::unordered_map<SymTok, std::vector<LabTok> > &LibraryToolbox::get_assertions_by_type() const
{
    return this->assertions_by_type;
}

void LibraryToolbox::compute_derivations()
{
    // Build the derivation rules; a derivation is created for each $f statement
    // and for each $a statement without essential hypotheses such that no variable
    // appears more than once and without distinct variables constraints and that does not
    // begin with the turnstile
    for (auto &type_lab : this->get_final_stack_frame().types) {
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
        if (ass.is_theorem()) {
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
            sent2.push_back(this->is_constant(tok) ? tok : this->get_sentence(this->get_var_sym_to_lab(tok)).at(0));
        }
        this->derivations[sent.at(0)].push_back(make_pair(ass.get_thesis(), sent2));
    }
}

const std::pair<SymTok, Sentence> &LibraryToolbox::get_derivation_rule(LabTok lab) const
{
    if (lab <= this->lib.get_labels_num()) {
        return this->ders_by_label.at(lab);
    } else {
        return this->temp_generator->get_derivation_rule(lab);
    }
}

RegisteredProver LibraryToolbox::register_prover(const std::vector<string> &templ_hyps, const string &templ_thesis)
{
    size_t index = LibraryToolbox::registered_provers().size();
    auto &rps = LibraryToolbox::registered_provers();
    rps.push_back({templ_hyps, templ_thesis});
    //cerr << "first: " << &rps << "; size: " << rps.size() << endl;
    return { index, templ_hyps, templ_thesis };
}



void LibraryToolbox::compute_registered_provers()
{
    for (size_t index = 0; index < LibraryToolbox::registered_provers().size(); index++) {
        this->compute_registered_prover(index, false);
    }
    //cerr << "Computed " << LibraryToolbox::registered_provers().size() << " registered provers" << endl;
}

void LibraryToolbox::compute_parser_initialization()
{
    std::function< std::ostream&(std::ostream&, SymTok) > sym_printer = [&](ostream &os, SymTok sym)->ostream& { return os << this->resolve_symbol(sym); };
    std::function< std::ostream&(std::ostream&, LabTok) > lab_printer = [&](ostream &os, LabTok lab)->ostream& { return os << this->resolve_label(lab); };
    const auto &ders = this->get_derivations();
    string ders_digest = hash_object(ders);
    this->parser = make_unique< LRParser< SymTok, LabTok > >(ders, sym_printer, lab_printer);
    bool loaded = false;
    if (this->cache != NULL) {
        if (this->cache->load()) {
            if (ders_digest == this->cache->get_digest()) {
                this->parser->set_cached_data(this->cache->get_lr_parser_data());
                loaded = true;
            }
        }
    }
    if (!loaded) {
        this->parser->initialize();
        if (this->cache != NULL) {
            this->cache->set_digest(ders_digest);
            this->cache->set_lr_parser_data(this->parser->get_cached_data());
            this->cache->store();
        }
    }
    // Drop the cache so that memory can be recovered
    this->cache = NULL;
}

const LRParser<SymTok, LabTok> &LibraryToolbox::get_parser() const
{
    return *this->parser;
}

ParsingTree< SymTok, LabTok > LibraryToolbox::parse_sentence(typename Sentence::const_iterator sent_begin, typename Sentence::const_iterator sent_end, SymTok type) const
{
    return this->get_parser().parse(sent_begin, sent_end, type);
}

ParsingTree< SymTok, LabTok > LibraryToolbox::parse_sentence(const Sentence &sent, SymTok type) const
{
    return this->get_parser().parse(sent, type);
}

ParsingTree<SymTok, LabTok> LibraryToolbox::parse_sentence(const Sentence &sent) const
{
    return this->parse_sentence(sent.begin()+1, sent.end(), sent.at(0));
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
        this->parsed_sents2.resize(i+1);
        this->parsed_sents2[i] = pt_to_pt2(pt);
        this->parsed_iters.resize(i+1);
        ParsingTreeMultiIterator< SymTok, LabTok > it = this->parsed_sents2[i].get_multi_iterator();
        while (true) {
            auto x = it.next();
            this->parsed_iters[i].push_back(x);
            if (x.first == it.Finished) {
                break;
            }
        }
    }
}

const std::vector<ParsingTree<SymTok, LabTok> > &LibraryToolbox::get_parsed_sents() const
{
    return this->parsed_sents;
}

const std::vector<ParsingTree2<SymTok, LabTok> > &LibraryToolbox::get_parsed_sents2() const
{
    return this->parsed_sents2;
}

const std::vector<std::vector<std::pair<ParsingTreeMultiIterator< SymTok, LabTok >::Status, ParsingTreeNode<SymTok, LabTok> > > > &LibraryToolbox::get_parsed_iters() const
{
    return this->parsed_iters;
}

void LibraryToolbox::compute_registered_prover(size_t index, bool exception_on_failure)
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
        if (unification.empty()) {
            if (exception_on_failure) {
                throw MMPPException("Could not find the template assertion");
            } else {
                return;
            }
        }
        inst_data = RegisteredProverInstanceData(unification[0], this->resolve_label(get<0>(unification[0])));
    }
}

std::pair<LabTok, SymTok> LibraryToolbox::new_temp_var(SymTok type_sym) const
{
    return this->temp_generator->new_temp_var(type_sym);
}

LabTok LibraryToolbox::new_temp_label(string name) const
{
    return this->temp_generator->new_temp_label(name);
}

void LibraryToolbox::new_temp_var_frame() const
{
    return this->temp_generator->new_temp_var_frame();
}

void LibraryToolbox::release_temp_var_frame() const
{
    return this->temp_generator->release_temp_var_frame();
}

string SentencePrinter::to_string() const
{
    ostringstream buf;
    buf << *this;
    return buf.str();
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

string ProofPrinter::to_string() const
{
    ostringstream buf;
    buf << *this;
    return buf.str();
}
