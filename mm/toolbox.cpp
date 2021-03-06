
#include <boost/filesystem/fstream.hpp>

#include <giolib/containers.h>

#include "toolbox.h"
#include "utils/utils.h"
#include "old/unification.h"
#include "parsing/unif.h"
#include "parsing/earley.h"
#include "reader.h"
#include "mm/proof.h"

std::ostream &operator<<(std::ostream &os, const SentencePrinter &sp)
{
    bool first = true;
    if (sp.style == SentencePrinter::STYLE_ALTHTML) {
        // TODO - HTML fixing could be done just once
        os << fix_htmlcss_for_qt(sp.tb.get_addendum().get_htmlcss());
        os << "<SPAN " << sp.tb.get_addendum().get_htmlfont() << ">";
    }
    Sentence sent2;
    const Sentence *sentp = nullptr;
    if (sp.sent != nullptr) {
        sentp = sp.sent;
    } else if (sp.pt != nullptr) {
        sent2 = sp.tb.reconstruct_sentence(*sp.pt, sp.type_sym);
        sentp = &sent2;
    } else if (sp.pt2 != nullptr) {
        sent2 = sp.tb.reconstruct_sentence(pt2_to_pt(*sp.pt2), sp.type_sym);
        sentp = &sent2;
    } else {
        assert("Should never arrive here" == nullptr);
    }
    const Sentence &sent = *sentp;
    for (auto &tok : sent) {
        if (first) {
            first = false;
        } else {
            os << std::string(" ");
        }
        if (sp.style == SentencePrinter::STYLE_PLAIN) {
            os << sp.tb.resolve_symbol(tok);
        } else if (sp.style == SentencePrinter::STYLE_NUMBERS) {
            os << tok.val();
        } else if (sp.style == SentencePrinter::STYLE_HTML) {
            os << sp.tb.get_addendum().get_htmldef(tok);
        } else if (sp.style == SentencePrinter::STYLE_ALTHTML) {
            os << sp.tb.get_addendum().get_althtmldef(tok);
        } else if (sp.style == SentencePrinter::STYLE_LATEX) {
            os << sp.tb.get_addendum().get_latexdef(tok);
        } else if (sp.style == SentencePrinter::STYLE_ANSI_COLORS_SET_MM) {
            if (sp.tb.get_standard_is_var_sym()(tok)) {
                std::string type_str = sp.tb.resolve_symbol(sp.tb.get_var_sym_to_type_sym(tok));
                if (type_str == "setvar") {
                    os << "\033[91m";
                } else if (type_str == "class") {
                    os << "\033[35m";
                } else if (type_str == "wff") {
                    os << "\033[94m";
                } else {
                    throw std::runtime_error("Unknown type");
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

std::ostream &operator<<(std::ostream &os, const ProofPrinter &sp)
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
                os << std::string(" ");
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

LibraryToolbox::LibraryToolbox(const ExtendedLibrary &lib, std::string turnstile, std::shared_ptr<ToolboxCache> cache) :
    cache(cache),
    lib(lib),
    turnstile(lib.get_symbol(turnstile)), turnstile_alias(lib.get_parsing_addendum().get_syntax().at(this->turnstile)),
    temp_generator(std::make_unique< TempGenerator >(lib))
    //type_labels(lib.get_final_stack_frame().types), type_labels_set(lib.get_final_stack_frame().types_set)
{
    assert(this->lib.is_immutable());
    this->standard_is_var = [this](LabTok x)->bool {
        /*const auto &types_set = this->get_types_set();
        if (types_set.find(x) == types_set.end()) {
            return false;
        }
        return !this->is_constant(this->get_sentence(x).at(1));*/
        if (x.val() > this->lib.get_labels_num()) {
            return true;
        }
        return this->get_is_var_by_type().at(x.val());
    };
    this->standard_is_var_sym = [this](SymTok x)->bool {
        if (x.val() > this->lib.get_symbols_num()) {
            return true;
        }
        return !this->lib.is_constant(x);
    };
    this->validation_rule = [this](LabTok x) {
        auto rule = this->get_derivation_rule(x);
        std::vector< SymTok > fixed_rule;
        for (const auto r : rule.second) {
            if (this->get_derivations().find(r) != this->get_derivations().end()) {
                fixed_rule.push_back(r);
            }
        }
        return std::make_pair(rule.first, fixed_rule);
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
    for (const ParsingTree2< SymTok, LabTok > &pt : this->gen_parsed_sents2()) {
        std::set< LabTok > vars;
        collect_variables2(pt, this->get_standard_is_var(), vars);
        this->sentence_vars.push_back(vars);
    }
    for (const auto &ass : this->lib.get_assertions()) {
        if (!ass.is_valid()) {
            this->assertion_const_vars.emplace_back();
            this->assertion_unconst_vars.emplace_back();
            continue;
        }
        const auto &thesis_vars = this->sentence_vars[ass.get_thesis().val()];
        std::set< LabTok > hyps_vars;
        for (const auto hyp_tok : ass.get_ess_hyps()) {
            const auto &hyp_vars = this->sentence_vars[hyp_tok.val()];
            hyps_vars.insert(hyp_vars.begin(), hyp_vars.end());
        }
        this->assertion_const_vars.push_back(thesis_vars);
        this->assertion_unconst_vars.emplace_back();
        auto &unconst_vars = this->assertion_unconst_vars.back();
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
    bool imp_found = (imp_label != LabTok{});
    for (const Assertion &ass : this->lib.get_assertions()) {
        if (!ass.is_valid() || this->get_sentence(ass.get_thesis()).at(0) != this->get_turnstile()) {
            continue;
        }
        const auto &pt = this->get_parsed_sent(ass.get_thesis());
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
std::pair<std::vector<ParsingTree<SymTok, LabTok> >, ParsingTree<SymTok, LabTok> > LibraryToolbox::refresh_assertion(const Assertion &ass, temp_allocator &ta) const
{
    // Collect all variables
    std::set< LabTok > var_labs;
    const auto &is_var = this->get_standard_is_var();
    const ParsingTree< SymTok, LabTok > &thesis_pt = this->get_parsed_sent(ass.get_thesis());
    collect_variables(thesis_pt, is_var, var_labs);
    for (const auto hyp : ass.get_ess_hyps()) {
        const ParsingTree< SymTok, LabTok > &hyp_pt = this->get_parsed_sent(hyp);
        collect_variables(hyp_pt, is_var, var_labs);
    }

    // Build a substitution map
    SubstMap< SymTok, LabTok > subst = this->build_refreshing_subst_map(var_labs, ta).first;

    // Substitute and return
    ParsingTree< SymTok, LabTok > thesis_new_pt = ::substitute(thesis_pt, is_var, subst);
    std::vector< ParsingTree< SymTok, LabTok > > hyps_new_pts;
    for (const auto hyp : ass.get_ess_hyps()) {
        const ParsingTree< SymTok, LabTok > &hyp_pt = this->get_parsed_sent(hyp);
        hyps_new_pts.push_back(::substitute(hyp_pt, is_var, subst));
    }
    return std::make_pair(hyps_new_pts, thesis_new_pt);
}

std::pair<std::vector<ParsingTree2<SymTok, LabTok> >, ParsingTree2<SymTok, LabTok> > LibraryToolbox::refresh_assertion2(const Assertion &ass, temp_allocator &ta) const
{
    // Collect all variables
    std::set< LabTok > var_labs;
    const auto &is_var = this->get_standard_is_var();
    const ParsingTree2< SymTok, LabTok > &thesis_pt = this->get_parsed_sent2(ass.get_thesis());
    collect_variables2(thesis_pt, is_var, var_labs);
    for (const auto hyp : ass.get_ess_hyps()) {
        const ParsingTree2< SymTok, LabTok > &hyp_pt = this->get_parsed_sent2(hyp);
        collect_variables2(hyp_pt, is_var, var_labs);
    }

    // Build a substitution map
    SimpleSubstMap2< SymTok, LabTok > subst = this->build_refreshing_subst_map2(var_labs, ta);

    // Substitute and return
    ParsingTree2< SymTok, LabTok > thesis_new_pt = ::substitute2_simple(thesis_pt, is_var, subst);
    std::vector< ParsingTree2< SymTok, LabTok > > hyps_new_pts;
    for (const auto hyp : ass.get_ess_hyps()) {
        const ParsingTree2< SymTok, LabTok > &hyp_pt = this->get_parsed_sent2(hyp);
        hyps_new_pts.push_back(::substitute2_simple(hyp_pt, is_var, subst));
    }
    return std::make_pair(hyps_new_pts, thesis_new_pt);
}

ParsingTree<SymTok, LabTok> LibraryToolbox::refresh_parsing_tree(const ParsingTree<SymTok, LabTok> &pt, temp_allocator &ta) const
{
    // Collect all variables
    std::set< LabTok > var_labs;
    const auto &is_var = this->get_standard_is_var();
    collect_variables(pt, is_var, var_labs);

    // Build a substitution map
    SubstMap< SymTok, LabTok > subst = this->build_refreshing_subst_map(var_labs, ta).first;

    // Substitute and return
    return ::substitute(pt, is_var, subst);
}

ParsingTree2<SymTok, LabTok> LibraryToolbox::refresh_parsing_tree2(const ParsingTree2<SymTok, LabTok> &pt, temp_allocator &ta) const
{
    // Collect all variables
    std::set< LabTok > var_labs;
    const auto &is_var = this->get_standard_is_var();
    collect_variables2(pt, is_var, var_labs);

    // Build a substitution map
    SimpleSubstMap2< SymTok, LabTok > subst = this->build_refreshing_subst_map2(var_labs, ta);

    // Substitute and return
    return ::substitute2_simple(pt, is_var, subst);
}

std::pair< SubstMap<SymTok, LabTok>, std::set< LabTok > > LibraryToolbox::build_refreshing_subst_map(const std::set<LabTok> &vars, temp_allocator &ta) const
{
    SubstMap< SymTok, LabTok > subst;
    std::set< LabTok > new_vars;
    for (const auto &var : vars) {
        SymTok type_sym = this->get_sentence(var).at(0);
        LabTok new_lab;
        std::tie(new_lab, std::ignore) = ta.new_temp_var(type_sym);
        new_vars.insert(new_lab);
        ParsingTree< SymTok, LabTok > new_pt;
        new_pt.label = new_lab;
        new_pt.type = type_sym;
        subst[var] = new_pt;
    }
    return std::make_pair(subst, new_vars);
}

SimpleSubstMap2<SymTok, LabTok> LibraryToolbox::build_refreshing_subst_map2(const std::set<LabTok> &vars, temp_allocator &ta) const
{
    SimpleSubstMap2< SymTok, LabTok > subst;
    for (const auto &var : vars) {
        SymTok type_sym = this->get_sentence(var).at(0);
        LabTok new_lab;
        std::tie(new_lab, std::ignore) = ta.new_temp_var(type_sym);
        subst[var] = new_lab;
    }
    return subst;
}

std::pair< SubstMap2<SymTok, LabTok>, std::set< LabTok > > LibraryToolbox::build_refreshing_full_subst_map2(const std::set<LabTok> &vars, temp_allocator &ta) const
{
    auto tmp = this->build_refreshing_subst_map(vars, ta);
    return std::make_pair(subst_to_subst2(tmp.first), tmp.second);
}

SymTok LibraryToolbox::get_symbol(std::string s) const
{
    auto res = this->lib.get_symbol(s);
    if (res != SymTok{}) {
        return res;
    }
    return this->temp_generator->get_symbol(s);
}

LabTok LibraryToolbox::get_label(std::string s) const
{
    auto res = this->lib.get_label(s);
    if (res != LabTok{}) {
        return res;
    }
    return this->temp_generator->get_label(s);
}

std::string LibraryToolbox::resolve_symbol(SymTok tok) const
{
    auto res = this->lib.resolve_symbol(tok);
    if (res != "") {
        return res;
    }
    return this->temp_generator->resolve_symbol(tok);
}

std::string LibraryToolbox::resolve_label(LabTok tok) const
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
    if (sent != nullptr) {
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
    const Assertion *ass = this->lib.get_assertion_ptr(label);
    if (ass != nullptr) {
        return *ass;
    }
    return this->temp_generator->get_assertion(label);
}

/*std::function<const Assertion *()> LibraryToolbox::list_assertions() const
{
    return this->lib.list_assertions();
}*/

Generator<std::reference_wrapper<const Assertion> > LibraryToolbox::gen_assertions() const
{
    return this->lib.gen_assertions();
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

std::vector<SymTok> LibraryToolbox::read_sentence(const std::string &in) const
{
    auto toks = tokenize(in);
    std::vector< SymTok > res;
    for (auto &tok : toks) {
        auto tok_num = this->get_symbol(tok);
        gio::assert_or_throw< std::runtime_error >(tok_num != SymTok{}, "not a symbol");
        res.push_back(tok_num);
    }
    return res;
}

SentencePrinter LibraryToolbox::print_sentence(const std::vector<SymTok> &sent, SentencePrinter::Style style) const
{
    return SentencePrinter({ &sent, {}, {}, {}, *this, style });
}

SentencePrinter LibraryToolbox::print_sentence(const ParsingTree<SymTok, LabTok> &pt, SentencePrinter::Style style) const
{
    return SentencePrinter({ {}, &pt, {}, {}, *this, style });
}

SentencePrinter LibraryToolbox::print_sentence(const ParsingTree2<SymTok, LabTok> &pt, SentencePrinter::Style style) const
{
    return SentencePrinter({ {}, {}, &pt, {}, *this, style });
}

SentencePrinter LibraryToolbox::print_sentence(const std::pair<SymTok, ParsingTree<SymTok, LabTok> > &pt, SentencePrinter::Style style) const
{
    return SentencePrinter({ {}, &pt.second, {}, pt.first, *this, style });
}

SentencePrinter LibraryToolbox::print_sentence(const std::pair<SymTok, ParsingTree2<SymTok, LabTok> > &pt, SentencePrinter::Style style) const
{
    return SentencePrinter({ {}, {}, &pt.second, pt.first, *this, style });
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

#ifdef TOOLBOX_SELF_TEST
static std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, Sentence> > > unify_assertion_internal_old(const LibraryToolbox *self, const std::vector<Sentence> &hypotheses, const Sentence &thesis, bool just_first, bool up_to_hyps_perms)
{
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > ret;

    std::vector< SymTok > sent;
    for (auto &hyp : hypotheses) {
        std::copy(hyp.begin(), hyp.end(), back_inserter(sent));
        sent.push_back({});
    }
    std::copy(thesis.begin(), thesis.end(), back_inserter(sent));

    for (const Assertion &ass : self->gen_assertions()) {
        if (ass.is_usage_disc()) {
            continue;
        }
        if (ass.get_ess_hyps().size() != hypotheses.size()) {
            continue;
        }
        // We have to generate all the hypotheses' permutations; fortunately usually hypotheses are not many
        // TODO Is there a better algorithm?
        // The i-th specified hypothesis is matched with the perm[i]-th assertion hypothesis
        std::vector< size_t > perm;
        for (size_t i = 0; i < hypotheses.size(); i++) {
            perm.push_back(i);
        }
        do {
            std::vector< SymTok > templ;
            for (size_t i = 0; i < hypotheses.size(); i++) {
                auto &hyp = self->get_sentence(ass.get_ess_hyps()[perm[i]]);
                std::copy(hyp.begin(), hyp.end(), back_inserter(templ));
                templ.push_back({});
            }
            auto &th = self->get_sentence(ass.get_thesis());
            copy(th.begin(), th.end(), back_inserter(templ));
            auto unifications = unify_old(sent, templ, *self);
            if (!unifications.empty()) {
                for (auto &unification : unifications) {
                    // Here we have to check that substitutions are of the corresponding type
                    // TODO - Here we immediately drop the type information, which probably mean that later we have to compute it again
                    bool wrong_unification = false;
                    for (auto &float_hyp : ass.get_float_hyps()) {
                        const Sentence &float_hyp_sent = self->get_sentence(float_hyp);
                        Sentence type_sent;
                        type_sent.push_back(float_hyp_sent.at(0));
                        auto &type_main_sent = unification.at(float_hyp_sent.at(1));
                        std::copy(type_main_sent.begin(), type_main_sent.end(), back_inserter(type_sent));
                        CreativeProofEngineImpl< Sentence > engine(*self);
                        if (!self->type_proving_helper(type_sent, engine)) {
                            wrong_unification = true;
                            break;
                        }
                    }
                    // If self is the case, then we have found a legitimate unification
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
        } while (std::next_permutation(perm.begin(), perm.end()));
    }

    return ret;
}
#endif

static std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, Sentence> > > unify_assertion_internal(const LibraryToolbox *self, const std::vector< std::pair< SymTok, ParsingTree<SymTok, LabTok > > > &pt_hyps, const std::pair< SymTok, ParsingTree< SymTok, LabTok > > &pt_thesis,
                                                                                                                             bool just_first, bool up_to_hyps_perms, const std::set< std::pair< SymTok, SymTok > > &antidists) {
    std::vector<std::tuple< LabTok, std::vector< size_t >, std::unordered_map<SymTok, std::vector<SymTok> > > > ret;
    const auto &is_var = self->get_standard_is_var();
    for (const Assertion &ass : self->gen_assertions()) {
        if (ass.is_usage_disc()) {
            continue;
        }
        if (ass.get_ess_hyps().size() != pt_hyps.size()) {
            continue;
        }
        if (pt_thesis.first != self->get_sentence(ass.get_thesis())[0]) {
            continue;
        }
        UnilateralUnificator< SymTok, LabTok > unif(is_var);
        auto &templ_pt = self->get_parsed_sent(ass.get_thesis());
        unif.add_parsing_trees(templ_pt, pt_thesis.second);
        if (!unif.is_unifiable()) {
            continue;
        }
        // We have to generate all the hypotheses' permutations; fortunately usually hypotheses are not many
        // TODO Is there a better algorithm?
        // The i-th specified hypothesis is matched with the perm[i]-th assertion hypothesis
        std::vector< size_t > perm;
        for (size_t i = 0; i < pt_hyps.size(); i++) {
            perm.push_back(i);
        }
        do {
            auto unif2 = unif;
            bool res = true;
            for (size_t i = 0; i < pt_hyps.size(); i++) {
                res = (pt_hyps[i].first == self->get_sentence(ass.get_ess_hyps()[perm[i]])[0]);
                if (!res) {
                    break;
                }
                auto &templ_pt = self->get_parsed_sent(ass.get_ess_hyps()[perm[i]]);
                unif2.add_parsing_trees(templ_pt, pt_hyps[i].second);
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
                subst2.insert(make_pair(self->get_sentence(s.first).at(1), self->reconstruct_sentence(s.second)));
            }
            VectorMap< SymTok, Sentence > subst3(subst2.begin(), subst2.end());
            auto dists = propagate_dists< Sentence >(ass, subst3, *self);
            if (!gio::has_no_diagonal(dists.begin(), dists.end())) {
                continue;
            }
            if (!gio::is_disjoint(dists.begin(), dists.end(), antidists.begin(), antidists.end())) {
                continue;
            }
            ret.emplace_back(ass.get_thesis(), perm, subst2);
            if (just_first) {
                return ret;
            }
            if (!up_to_hyps_perms) {
                break;
            }
        } while (std::next_permutation(perm.begin(), perm.end()));
    }

    return ret;
}

static std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, Sentence> > > unify_assertion_internal(const LibraryToolbox *self, const std::vector<Sentence> &hypotheses, const Sentence &thesis, bool just_first, bool up_to_hyps_perms, const std::set< std::pair< SymTok, SymTok > > &antidists)
{
    // Parse inputs
    std::vector< std::pair< SymTok, ParsingTree< SymTok, LabTok > > > pt_hyps;
    for (auto &hyp : hypotheses) {
        auto pt = self->parse_sentence(hyp.begin()+1, hyp.end(), hyp.front() == self->get_turnstile() ? self->get_turnstile_alias() : hyp.front());
        if (pt.label == LabTok{}) {
            return {};
        }
        pt_hyps.push_back(std::make_pair(hyp[0], pt));
    }
    auto pt_thesis = std::make_pair(thesis[0], self->parse_sentence(thesis.begin()+1, thesis.end(), thesis.front() == self->get_turnstile() ? self->get_turnstile_alias() : thesis.front()));
    if (pt_thesis.second.label == LabTok{}) {
        return {};
    }

    return unify_assertion_internal(self, pt_hyps, pt_thesis, just_first, up_to_hyps_perms, antidists);
}

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, Sentence> > > LibraryToolbox::unify_assertion(const std::vector<Sentence> &hypotheses, const Sentence &thesis, bool just_first, bool up_to_hyps_perms, const std::set<std::pair<SymTok, SymTok> > &antidists) const
{
    auto ret2 = unify_assertion_internal(this, hypotheses, thesis, just_first, up_to_hyps_perms, antidists);
#ifdef TOOLBOX_SELF_TEST
    auto ret = unify_assertion_internal_old(this, hypotheses, thesis, just_first, up_to_hyps_perms);
    assert(ret == ret2);
#endif
    return ret2;
}

std::vector<std::tuple<LabTok, std::vector<size_t>, std::unordered_map<SymTok, Sentence> > > LibraryToolbox::unify_assertion(const std::vector<std::pair<SymTok, ParsingTree<SymTok, LabTok> > > &hypotheses, const std::pair<SymTok, ParsingTree<SymTok, LabTok> > &thesis, bool just_first, bool up_to_hyps_perms, const std::set<std::pair<SymTok, SymTok> > &antidists) const
{
    return unify_assertion_internal(this, hypotheses, thesis, just_first, up_to_hyps_perms, antidists);
}

const std::function<bool (LabTok)> &LibraryToolbox::get_standard_is_var() const {
    return this->standard_is_var;
}

const std::function<bool (SymTok)> &LibraryToolbox::get_standard_is_var_sym() const
{
    return this->standard_is_var_sym;
}

const std::function<std::pair<SymTok, std::vector<SymTok> > (LabTok)> &LibraryToolbox::get_validation_rule() const
{
    return this->validation_rule;
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

Generator<SymTok> LibraryToolbox::gen_symbols() const
{
    return Generator<SymTok>([this](auto &sink) {
            for (SymTok::val_type i = 1; i <= this->get_symbols_num(); i++) {
                sink(SymTok(i));
            }
        }
    );
}

Generator<LabTok> LibraryToolbox::gen_labels() const
{
    return Generator<LabTok>([this](auto &sink) {
            for (LabTok::val_type i = 1; i <= this->get_labels_num(); i++) {
                sink(LabTok(i));
            }
        }
    );
}

void LibraryToolbox::dump_proof_exception(const ProofException<Sentence> &e, std::ostream &out) const
{
    out << "Applying " << this->resolve_label(e.get_error().label) << " the proof executor signalled an error..." << std::endl;
    out << "The reason was " << e.get_reason() << std::endl;
    out << "On stack there was: " << this->print_sentence(e.get_error().on_stack) << std::endl;
    out << "Has to match with: " << this->print_sentence(e.get_error().to_subst) << std::endl;
    out << "Substitution map:" << std::endl;
    for (const auto &it : e.get_error().subst_map) {
        out << this->resolve_symbol(it.first) << ": " << this->print_sentence(it.second) << std::endl;
    }
}

void LibraryToolbox::dump_proof_exception(const ProofException<ParsingTree2<SymTok, LabTok> > &e, std::ostream &out) const
{
    out << "Applying " << this->resolve_label(e.get_error().label) << " the proof executor signalled an error..." << std::endl;
    out << "The reason was " << e.get_reason() << std::endl;
    out << "On stack there was: " << this->print_sentence(e.get_error().on_stack) << std::endl;
    out << "Has to match with: " << this->print_sentence(e.get_error().to_subst) << std::endl;
    out << "Substitution map:" << std::endl;
    for (const auto &it : e.get_error().subst_map) {
        out << this->resolve_symbol(this->get_var_lab_to_sym(it.first)) << ": " << this->print_sentence(it.second) << std::endl;
    }
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
        gio::enlarge_and_set(this->var_lab_to_sym, var_lab.val()) = var_sym;
        gio::enlarge_and_set(this->var_sym_to_lab, var_sym.val()) = var_lab;
        gio::enlarge_and_set(this->var_lab_to_type_sym, var_lab.val()) = type_sym;
        gio::enlarge_and_set(this->var_sym_to_type_sym, var_sym.val()) = type_sym;
    }
}

LabTok LibraryToolbox::get_var_sym_to_lab(SymTok sym) const
{
    if (sym.val() <= this->lib.get_symbols_num()) {
        return this->var_sym_to_lab.at(sym.val());
    } else {
        return this->temp_generator->get_var_sym_to_lab(sym);
    }
}

SymTok LibraryToolbox::get_var_lab_to_sym(LabTok lab) const
{
    if (lab.val() <= this->lib.get_labels_num()) {
        return this->var_lab_to_sym.at(lab.val());
    } else {
        return this->temp_generator->get_var_lab_to_sym(lab);
    }
}

SymTok LibraryToolbox::get_var_sym_to_type_sym(SymTok sym) const
{
    if (sym.val() <= this->lib.get_symbols_num()) {
        return this->var_sym_to_type_sym.at(sym.val());
    } else {
        return this->temp_generator->get_var_sym_to_type_sym(sym);
    }
}

SymTok LibraryToolbox::get_var_lab_to_type_sym(LabTok lab) const
{
    if (lab.val() <= this->lib.get_labels_num()) {
        return this->var_lab_to_type_sym.at(lab.val());
    } else {
        return this->temp_generator->get_var_lab_to_type_sym(lab);
    }
}

void LibraryToolbox::compute_is_var_by_type()
{
    const auto &types_set = this->get_final_stack_frame().types_set;
    this->is_var_by_type.resize(this->lib.get_labels_num());
    for (LabTok label : this->gen_labels()) {
        this->is_var_by_type[label.val()] = (types_set.find(label) != types_set.end() && !this->is_constant(this->get_sentence(label).at(1)));
    }
}

const std::vector<bool> &LibraryToolbox::get_is_var_by_type() const
{
    return this->is_var_by_type;
}

void LibraryToolbox::compute_assertions_by_type()
{
    for (const Assertion &ass : this->gen_assertions()) {
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
        this->derivations[type_sent.at(0)].push_back(std::make_pair(type_lab, std::vector<SymTok>({type_sent.at(1)})));
    }
    // FIXME Take it from the configuration
    for (const Assertion &ass : this->gen_assertions()) {
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
        std::set< SymTok > symbols;
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
        std::vector< SymTok > sent2;
        for (size_t i = 1; i < sent.size(); i++) {
            const auto &tok = sent[i];
            // Variables are replaced with their types, which act as variables of the context-free grammar
            sent2.push_back(this->is_constant(tok) ? tok : this->get_sentence(this->get_var_sym_to_lab(tok)).at(0));
        }
        this->derivations[sent.at(0)].push_back(std::make_pair(ass.get_thesis(), sent2));
    }
}

const std::pair<SymTok, Sentence> &LibraryToolbox::get_derivation_rule(LabTok lab) const
{
    if (lab.val() <= this->lib.get_labels_num()) {
        return this->ders_by_label.at(lab);
    } else {
        return this->temp_generator->get_derivation_rule(lab);
    }
}

RegisteredProver LibraryToolbox::register_prover(const std::vector<std::string> &templ_hyps, const std::string &templ_thesis)
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
    std::function< std::ostream&(std::ostream&, SymTok) > sym_printer = [&](std::ostream &os, SymTok sym)->std::ostream& { return os << this->resolve_symbol(sym); };
    std::function< std::ostream&(std::ostream&, LabTok) > lab_printer = [&](std::ostream &os, LabTok lab)->std::ostream& { return os << this->resolve_label(lab); };
    const auto &ders = this->get_derivations();
    std::string ders_digest = hash_object(ders);
    this->parser = std::make_unique< LRParser< SymTok, LabTok > >(ders, sym_printer, lab_printer);
    bool loaded = false;
    if (this->cache != nullptr) {
        if (this->cache->load()) {
            if (ders_digest == this->cache->get_digest()) {
                this->parser->set_cached_data(this->cache->get_lr_parser_data());
                loaded = true;
            }
        }
    }
    if (!loaded) {
        std::cerr << "No or invalid parser cache found; re-initializing parser..." << std::endl;
        this->parser->initialize();
        if (this->cache != nullptr) {
            this->cache->set_digest(ders_digest);
            this->cache->set_lr_parser_data(this->parser->get_cached_data());
            this->cache->store();
        }
    }
    // Drop the cache so that memory can be recovered
    this->cache = nullptr;
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
    return this->parse_sentence(sent.begin()+1, sent.end(), this->lib.get_parsing_addendum().get_syntax().at(sent.at(0)));
}

void LibraryToolbox::compute_sentences_parsing()
{
    /*if (!this->parser_initialization_computed) {
        this->compute_parser_initialization();
    }*/
    for (LabTok label : this->gen_labels()) {
        const Sentence &sent = this->get_sentence(label);
        auto pt = this->parse_sentence(sent.begin()+1, sent.end(), this->get_parsing_addendum().get_syntax().at(sent[0]));
        if (pt.label == LabTok{}) {
            throw std::runtime_error("Failed to parse a sentence in the library");
        }
        this->parsed_sents.resize(label.val()+1);
        this->parsed_sents[label.val()] = pt;
        this->parsed_sents2.resize(label.val()+1);
        this->parsed_sents2[label.val()] = pt_to_pt2(pt);
        this->parsed_iters.resize(label.val()+1);
        ParsingTreeMultiIterator< SymTok, LabTok > it = this->parsed_sents2[label.val()].get_multi_iterator();
        while (true) {
            auto x = it.next();
            this->parsed_iters[label.val()].push_back(x);
            if (x.first == it.Finished) {
                break;
            }
        }
    }
}

LabTok LibraryToolbox::get_registered_prover_label(const RegisteredProver &prover) const
{
    const size_t &index = prover.index;
    if (index >= this->instance_registered_provers.size() || !this->instance_registered_provers[index].valid) {
        throw std::runtime_error("cannot modify const object");
    }
    const RegisteredProverInstanceData &inst_data = this->instance_registered_provers[index];
    return inst_data.label;
}

const ParsingTree<SymTok, LabTok> &LibraryToolbox::get_parsed_sent(LabTok label) const
{
    if (label.val() < this->parsed_sents.size()) {
        return this->parsed_sents.at(label.val());
    } else {
        return this->temp_generator->get_parsed_sent(label);
    }
}

const ParsingTree2<SymTok, LabTok> &LibraryToolbox::get_parsed_sent2(LabTok label) const
{
    if (label.val() < this->parsed_sents2.size()) {
        return this->parsed_sents2.at(label.val());
    } else {
        return this->temp_generator->get_parsed_sent2(label);
    }
}

const std::vector<std::pair<ParsingTreeMultiIterator< SymTok, LabTok >::Status, ParsingTreeNode<SymTok, LabTok> > > &LibraryToolbox::get_parsed_iter(LabTok label) const
{
    return this->parsed_iters.at(label.val());
}

Generator<std::reference_wrapper<const ParsingTree<SymTok, LabTok> > > LibraryToolbox::gen_parsed_sents() const
{
    return Generator<std::reference_wrapper< const ParsingTree<SymTok, LabTok> > >([this](auto &sink) {
            for (LabTok::val_type i = 1; i < this->parsed_sents.size(); i++) {
                sink(std::cref(this->parsed_sents[i]));
            }
        }
    );
}

Generator<std::reference_wrapper<const ParsingTree2<SymTok, LabTok> > > LibraryToolbox::gen_parsed_sents2() const
{
    return Generator<std::reference_wrapper< const ParsingTree2<SymTok, LabTok> > >([this](auto &sink) {
            for (LabTok::val_type i = 1; i < this->parsed_sents2.size(); i++) {
                sink(std::cref(this->parsed_sents2[i]));
            }
        }
    );
}

Generator<std::pair<LabTok, std::reference_wrapper<const ParsingTree<SymTok, LabTok> > > > LibraryToolbox::enum_parsed_sents() const
{
    return Generator<std::pair<LabTok, std::reference_wrapper< const ParsingTree<SymTok, LabTok> > > >([this](auto &sink) {
            for (LabTok::val_type i = 1; i < this->parsed_sents.size(); i++) {
                sink(std::make_pair(LabTok(i), std::cref(this->parsed_sents[i])));
            }
        }
    );
}

Generator<std::pair<LabTok, std::reference_wrapper<const ParsingTree2<SymTok, LabTok> > > > LibraryToolbox::enum_parsed_sents2() const
{
    return Generator<std::pair<LabTok, std::reference_wrapper< const ParsingTree2<SymTok, LabTok> > > >([this](auto &sink) {
            for (LabTok::val_type i = 1; i < this->parsed_sents2.size(); i++) {
                sink(std::make_pair(LabTok(i), std::cref(this->parsed_sents2[i])));
            }
        }
    );
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
                throw std::runtime_error("Could not find the template assertion");
            } else {
                std::cerr << "Could not find a template assertion for:" << std::endl;
                for (const auto &hyp : data.templ_hyps) {
                    std::cerr << " * " << hyp << std::endl;
                }
                std::cerr << " => " << data.templ_thesis << std::endl;
                return;
            }
        }
        //cerr << "Resolving registered prover with label " << this->resolve_label(std::get<0>(unification[0])) << endl;
        inst_data = RegisteredProverInstanceData(unification[0], this->resolve_label(std::get<0>(unification[0])));
    }
}

std::pair<LabTok, SymTok> LibraryToolbox::new_temp_var(SymTok type_sym) const
{
    return this->temp_generator->new_temp_var(type_sym);
}

LabTok LibraryToolbox::new_temp_label(std::string name) const
{
    return this->temp_generator->new_temp_label(name);
}

void LibraryToolbox::release_temp_var(LabTok lab) const
{
    this->temp_generator->release_temp_var(lab);
}

std::string SentencePrinter::to_string() const
{
    std::ostringstream buf;
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

std::string FileToolboxCache::get_digest() {
    return this->digest;
}

void FileToolboxCache::set_digest(std::string digest) {
    this->digest = digest;
}

LRParser< SymTok, LabTok >::CachedData FileToolboxCache::get_lr_parser_data() {
    return this->lr_parser_data;
}

void FileToolboxCache::set_lr_parser_data(const LRParser< SymTok, LabTok >::CachedData &cached_data) {
    this->lr_parser_data = cached_data;
}

std::string ProofPrinter::to_string() const
{
    std::ostringstream buf;
    buf << *this;
    return buf.str();
}

Prover<ProofEngine> make_throwing_prover(const std::__cxx11::string &msg) {
    return [msg](ProofEngine&) -> bool {
        throw std::runtime_error(msg);
    };
}

Prover<ProofEngine> trivial_prover(LabTok label) {
    return [label](ProofEngine &e) {
        e.process_label(label);
        return true;
    };
}

temp_stacked_allocator::temp_stacked_allocator(const LibraryToolbox &tb) : tb(tb) {
    this->new_temp_var_frame();
}

temp_stacked_allocator::~temp_stacked_allocator() {
    while (!this->stack.empty()) {
        this->release_temp_var_frame();
    }
}

std::pair<LabTok, SymTok> temp_stacked_allocator::new_temp_var(SymTok type_sym) {
    auto ret = this->tb.new_temp_var(type_sym);
    this->stack.back().push_back(ret.first);
    return ret;
}

void temp_stacked_allocator::new_temp_var_frame() {
    this->stack.emplace_back();
}

void temp_stacked_allocator::release_temp_var_frame() {
    for (const auto &lab : this->stack.back()) {
        tb.release_temp_var(lab);
    }
    this->stack.pop_back();
}
