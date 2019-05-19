
#include <boost/range/adaptor/reversed.hpp>

#include <giolib/static_block.h>
#include <giolib/main.h>
#include <giolib/containers.h>

#include "subst.h"

#include "mm/ptengine.h"
#include "mm/setmm_loader.h"
#include "mm/mmutils.h"
#include "utils/utils.h"

std::map< SymTok, std::tuple< LabTok, LabTok, LabTok > > compute_equalities(const LibraryToolbox &tb) {
    std::map< SymTok, std::tuple< LabTok, LabTok, LabTok > > ret;
    ret.insert(std::make_pair(tb.get_symbol("wff"), std::make_tuple(tb.get_label("wb"), tb.get_label("biid"), tb.get_label("wnf"))));
    ret.insert(std::make_pair(tb.get_symbol("class"), std::make_tuple(tb.get_label("wceq"), tb.get_label("eqid"), tb.get_label("wnfc"))));
    return ret;
}

std::map< SymTok, std::pair< SymTok, LabTok > > compute_type_adaptors(const LibraryToolbox &tb) {
    std::map< SymTok, std::pair< SymTok, LabTok > > ret;
    ret.insert(std::make_pair(tb.get_symbol("setvar"), std::make_pair(tb.get_symbol("class"), tb.get_label("cv"))));
    return ret;
}

std::set< SymTok > compute_var_types(const std::map< SymTok, std::tuple< LabTok, LabTok, LabTok > > &equalities, const std::map< SymTok, std::pair< SymTok, LabTok > > &adaptors) {
    std::set< SymTok > ret;
    for (const auto &p : equalities) {
        ret.insert(p.first);
    }
    for (const auto &p : adaptors) {
        ret.insert(p.first);
    }
    return ret;
}

using RawBoundDataTuple = std::tuple< std::string, std::string, std::vector< std::pair< std::string, std::string > > >;

std::vector< RawBoundDataTuple > raw_bound_data = {
    RawBoundDataTuple{ "class { x | ph }", "ph", { { "x", "" } } },
    RawBoundDataTuple{ "class { x e. A | ph }", "ph", { { "x", "A" } } },
    RawBoundDataTuple{ "class [_ A / x ]_ B", "B", { { "x", "" } } },
    RawBoundDataTuple{ "class U_ x e. A B", "B", { { "x", "A" } } },
    RawBoundDataTuple{ "class |^|_ x e. A B", "B", { { "x", "A" } } },
    RawBoundDataTuple{ "class { <. x , y >. | ph }", "ph", { { "x", "" }, { "y", "" } } },
    RawBoundDataTuple{ "class ( x e. A |-> B )", "B", { { "x", "A" } } },
    RawBoundDataTuple{ "class ( iota x ph )", "ph", { { "x", "" } } },
    RawBoundDataTuple{ "class { <. <. x , y >. , z >. | ph }", "ph", { { "x", "" }, { "y", "" }, { "z", "" } } },
    RawBoundDataTuple{ "class ( x e. A , y e. B |-> C )", "C", { { "x", "A" }, { "y", "B" } } },
    RawBoundDataTuple{ "class ( iota_ x e. A ph )", "ph", { { "x", "" } } },
    RawBoundDataTuple{ "class X_ x e. A B", "B", { { "x", "A" } } },
    RawBoundDataTuple{ "class sum_ x e. A B", "B", { { "x", "A" } } },
    RawBoundDataTuple{ "class S. A B _d x", "B", { { "x", "A" } } },
    RawBoundDataTuple{ "class S_ [ A -> B ] C _d x", "C", { { "x", "" } } },    // Might be relaxed
    RawBoundDataTuple{ "class sum* x e. A B", "B", { { "x", "A" } } },
    RawBoundDataTuple{ "class prod_ x e. A B", "B", { { "x", "A" } } },

    RawBoundDataTuple{ "wff A. x ph", "ph", { { "x", "" } } },
    RawBoundDataTuple{ "wff E. x ph", "ph", { { "x", "" } } },
    RawBoundDataTuple{ "wff F/ x ph", "ph", { { "x", "" } } },
    RawBoundDataTuple{ "wff [ x / y ] ph", "ph", { { "y", "" } } },
    RawBoundDataTuple{ "wff E! x ph", "ph", { { "x", "" } } },
    RawBoundDataTuple{ "wff E* x ph", "ph", { { "x", "" } } },
    RawBoundDataTuple{ "wff F/_ x A", "A", { { "x", "" } } },
    RawBoundDataTuple{ "wff A. x e. A ph", "ph", { { "x", "A" } } },
    RawBoundDataTuple{ "wff E. x e. A ph", "ph", { { "x", "A" } } },
    RawBoundDataTuple{ "wff E! x e. A ph", "ph", { { "x", "A" } } },
    RawBoundDataTuple{ "wff E* x e. A ph", "ph", { { "x", "A" } } },
    // CondEq??
    RawBoundDataTuple{ "wff [. A / x ]. ph", "ph", { { "x", "" } } },
    RawBoundDataTuple{ "wff Disj_ x e. A B", "B", { { "x", "A" } } },
};

size_t get_idx(const std::vector< ParsingTree< SymTok, LabTok > > &v, const LabTok x) {
    auto it = std::find_if(v.begin(), v.end(), [x](const auto &y) { return y.label == x; });
    assert(it != v.end());
    return static_cast< size_t >(it - v.begin());
}

decltype(auto) preprocess_bound_data(const LibraryToolbox &tb) {
    std::map< LabTok, std::pair< size_t, std::map< size_t, size_t > > > ret;
    for (const auto &datum : raw_bound_data) {
        const auto &sent_str = std::get<0>(datum);
        const auto &body_str = std::get<1>(datum);
        const auto &bound_str = std::get<2>(datum);
        const auto pt = tb.parse_sentence(tb.read_sentence(sent_str));
        for (const auto &child : pt.children) {
#ifdef NDEBUG
            (void) child;
#endif
            assert(child.children.empty());
            assert(tb.get_standard_is_var()(child.label));
        }
        const auto label = pt.label;
        const auto body_label = tb.get_var_sym_to_lab(tb.get_symbol(body_str));
        auto body_idx = get_idx(pt.children, body_label);
        auto &entry = ret[label];
        entry.first = body_idx;
        for (const auto &bound_pair : bound_str) {
            const auto bound_var = tb.get_var_sym_to_lab(tb.get_symbol(bound_pair.first));
            const auto bound_var_idx = get_idx(pt.children, bound_var);
            entry.second[bound_var_idx] = bound_pair.second == "" ? pt.children.size() : get_idx(pt.children, tb.get_var_sym_to_lab(tb.get_symbol(bound_pair.second)));
        }
    }
    return ret;
}

ParsingTree< SymTok, LabTok > create_adaptor_pt(const LibraryToolbox &tb, const std::map< SymTok, std::pair< SymTok, LabTok > > &adaptors, const ParsingTree< SymTok, LabTok > &pt) {
#ifdef NDEBUG
    (void) tb;
#endif
    ParsingTree< SymTok, LabTok > ret;
    auto &data = adaptors.at(pt.type);
    ret.type = data.first;
    ret.label = data.second;
    ret.children.push_back(pt);
    assert(ret.validate(tb.get_validation_rule()));
    return ret;
}

ParsingTree< SymTok, LabTok > create_equality_pt(const LibraryToolbox &tb, const std::map< SymTok, std::tuple< LabTok, LabTok, LabTok > > &equalities, const std::map< SymTok, std::pair< SymTok, LabTok > > &adaptors,
                                                 const ParsingTree< SymTok, LabTok > &pt1, const ParsingTree< SymTok, LabTok > &pt2) {
    assert(pt1.type == pt2.type);
    auto it = equalities.find(pt1.type);
    if (it != equalities.end()) {
        ParsingTree< SymTok, LabTok > ret;
        ret.type = tb.get_turnstile_alias();
        ret.label = std::get<0>(it->second);
        ret.children.push_back(pt1);
        ret.children.push_back(pt2);
        assert(ret.validate(tb.get_validation_rule()));
        return ret;
    } else {
        return create_equality_pt(tb, equalities, adaptors, create_adaptor_pt(tb, adaptors, pt1), create_adaptor_pt(tb, adaptors, pt2));
    }
}

ParsingTree< SymTok, LabTok > create_not_free_pt(const LibraryToolbox &tb, const std::map< SymTok, std::tuple< LabTok, LabTok, LabTok > > &equalities, const std::map< SymTok, std::pair< SymTok, LabTok > > &adaptors,
                                                 const ParsingTree< SymTok, LabTok > &pt1, const ParsingTree< SymTok, LabTok > &pt2) {
    assert(pt1.type == tb.get_symbol("setvar"));
    auto it = equalities.find(pt2.type);
    if (it != equalities.end()) {
        ParsingTree< SymTok, LabTok > ret;
        ret.type = tb.get_turnstile_alias();
        ret.label = std::get<2>(it->second);
        ret.children.push_back(pt1);
        ret.children.push_back(pt2);
        assert(ret.validate(tb.get_validation_rule()));
        return ret;
    } else {
        return create_not_free_pt(tb, equalities, adaptors, pt1, create_adaptor_pt(tb, adaptors, pt2));
    }
}

ParsingTree< SymTok, LabTok > create_conjunction_pt(const LibraryToolbox &tb, const ParsingTree< SymTok, LabTok > &pt1, const ParsingTree< SymTok, LabTok > &pt2) {
    return create_pt(tb, "wff ( ph /\\ ps )", { { "ph", pt1 }, { "ps", pt2 } });
}

ParsingTree< SymTok, LabTok > create_implication_pt(const LibraryToolbox &tb, const ParsingTree< SymTok, LabTok > &pt1, const ParsingTree< SymTok, LabTok > &pt2) {
    return create_pt(tb, "wff ( ph -> ps )", { { "ph", pt1 }, { "ps", pt2 } });
}

ParsingTree< SymTok, LabTok > create_forall_pt(const LibraryToolbox &tb, const ParsingTree< SymTok, LabTok > &pt1, const ParsingTree< SymTok, LabTok > &pt2) {
    return create_pt(tb, "wff A. x ph", { { "x", pt1 }, { "ph", pt2 } });
}

ParsingTree< SymTok, LabTok > create_restr_forall_pt(const LibraryToolbox &tb, const ParsingTree< SymTok, LabTok > &pt1, const ParsingTree< SymTok, LabTok > &pt2, const ParsingTree< SymTok, LabTok > &pt3) {
    return create_pt(tb, "wff A. x e. A ph", { { "x", pt1 }, { "A", pt2 }, { "ph", pt3 } });
}

std::pair< bool, bool > search_theorem(const LibraryToolbox &tb, const std::vector<std::pair<SymTok, ParsingTree<SymTok, LabTok> > > &hypotheses, const std::pair<SymTok, ParsingTree<SymTok, LabTok> > &thesis,
                                       const std::set< std::pair< SymTok, SymTok > > &acceptable_dists = {}) {
    bool found = false;
    bool has_dists = false;
    for (const auto &hyp : hypotheses) {
        std::cout << "      & " << tb.print_sentence(hyp.second) << std::endl;
        hyp.second.validate(tb.get_validation_rule());
    }
    std::cout << "     => " << tb.print_sentence(thesis.second) << std::endl;
    thesis.second.validate(tb.get_validation_rule());

    if (hypotheses.size() >= 10) {
        std::cout << "     Too many hypotheses, I refuse to process..." << std::endl;
        return std::make_pair(false, false);
    }

    auto res = tb.unify_assertion(hypotheses, thesis);
    if (!res.empty()) {
        std::cout << "     Found match " << tb.resolve_label(std::get<0>(res[0])) << std::endl;
        found = true;
        VectorMap< SymTok, Sentence > subst(std::get<2>(res[0]).begin(), std::get<2>(res[0]).end());
        auto dists = propagate_dists< Sentence >(tb.get_assertion(std::get<0>(res[0])), subst, tb);
        if (!gio::is_included(dists.begin(), dists.end(), acceptable_dists.begin(), acceptable_dists.end())) {
            std::cout << "     It has (excessive) DISTINCT VARIABLES provisions!" << std::endl;
            has_dists = true;
        }
    } else {
        std::cout << "     Found NO match..." << std::endl;
    }
    return std::make_pair(found, has_dists);
}

int subst_search_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    bool search_global = true;
    bool search_local = true;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto &ders = tb.get_derivations();
    auto equalities = compute_equalities(tb);
    auto adaptors = compute_type_adaptors(tb);
    auto var_types = compute_var_types(equalities, adaptors);
    auto bound_data = preprocess_bound_data(tb);

    int attempted = 0;
    int found = 0;
    for (const auto &der : ders) {
        const auto type = der.first;
        for (const auto &der2 : der.second) {
            tb.new_temp_var_frame();
            Finally f1([&tb]() {
                tb.release_temp_var_frame();
            });

            const auto label = der2.first;
            std::pair< size_t, std::map< size_t, size_t > > this_bound_data = { {}, {} };
            bool has_bound_data = false;
            auto this_bound_data_it = bound_data.find(label);
            if (this_bound_data_it != bound_data.end()) {
                this_bound_data = this_bound_data_it->second;
                has_bound_data = true;
            }
            const auto &rule = der2.second;
            bool has_vars = false;
            for (const auto sym : rule) {
                if (ders.find(sym) != ders.end()) {
                    has_vars = true;
                }
            }
            if (!has_vars) {
                continue;
            }

            std::cout << std::endl << "Considering derivation " << tb.resolve_label(label) <<" for type " << tb.resolve_symbol(type) << ", with variables:";
            for (const auto sym : rule) {
                if (ders.find(sym) != ders.end()) {
                    std::cout << " " << tb.resolve_symbol(sym);
                }
            }
            std::cout << std::endl;

            bool this_found = false;
            Finally f2([&this_found,&attempted,&found]() {
                attempted++;
                if (this_found) {
                    std::cout << "   Substitution rules found!" << std::endl;
                    found++;
                } else {
                    std::cout << "   Substitution rules NOT FOUND..." << std::endl;
                }
            });

            if (search_global) {
                ParsingTree< SymTok, LabTok > pt_left;
                ParsingTree< SymTok, LabTok > pt_right;
                pt_left.label = label;
                pt_left.type = type;
                pt_right.label = label;
                pt_right.type = type;
                std::vector< ParsingTree< SymTok, LabTok > > pts_eq_hyps;
                std::vector< ParsingTree< SymTok, LabTok > > pts_nf_hyps;
                std::cout << " * Search for a global substitution rule" << std::endl;
                size_t hyp_body_idx = 0;
                for (size_t i = 0; i < rule.size(); i++) {
                    auto var_type = rule[i];
                    size_t current_pos = pt_left.children.size();
                    if (var_types.find(var_type) != var_types.end()) {
                        auto bound_data_it = this_bound_data.second.find(current_pos);
                        if (bound_data_it != this_bound_data.second.end()) {
                            auto pt_var = create_temp_var_pt(tb, var_type);
                            pt_left.children.push_back(pt_var);
                            pt_right.children.push_back(pt_var);
                        } else {
                            auto pt_var1 = create_temp_var_pt(tb, var_type);
                            auto pt_var2 = create_temp_var_pt(tb, var_type);
                            if (current_pos == this_bound_data.first) {
                                hyp_body_idx = pts_eq_hyps.size();
                            }
                            pts_eq_hyps.push_back(create_equality_pt(tb, equalities, adaptors, pt_var1, pt_var2));
                            pt_left.children.push_back(pt_var1);
                            pt_right.children.push_back(pt_var2);
                        }
                    }
                }
                std::set< std::pair< SymTok, SymTok > > acceptable_dists;
                if (has_bound_data) {
                    // If we have bound_data for this syntax constructor, then we need to patch the body hypothesis
                    // and create not-free hypotheses
                    std::set< size_t > vars_idx;
                    for (const auto &p : boost::adaptors::reverse(this_bound_data.second)) {
                        auto var_idx = p.first;
                        auto range_idx = p.second;
                        for (const auto &prev_var_idx : vars_idx) {
                            assert(prev_var_idx != var_idx);
                            assert(pt_left.children[prev_var_idx].label != pt_left.children[var_idx].label);
                            acceptable_dists.insert(std::minmax(tb.get_var_lab_to_sym(pt_left.children[prev_var_idx].label), tb.get_var_lab_to_sym(pt_left.children[var_idx].label)));
                        }
                        vars_idx.insert(var_idx);
                        assert(pt_left.children[var_idx] == pt_right.children[var_idx]);
                        if (range_idx < pt_left.children.size()) {
                            pts_eq_hyps[hyp_body_idx] = create_restr_forall_pt(tb, pt_left.children[var_idx], pt_left.children[range_idx], pts_eq_hyps[hyp_body_idx]);
                        } else {
                            pts_eq_hyps[hyp_body_idx] = create_forall_pt(tb, pt_left.children[var_idx], pts_eq_hyps[hyp_body_idx]);
                        }
                    }
                    for (const auto &p : this_bound_data.second) {
                        for (const auto &var_idx : vars_idx) {
                            auto range_idx = p.second;
                            if (range_idx < pt_left.children.size()) {
                                pts_nf_hyps.push_back(create_not_free_pt(tb, equalities, adaptors, pt_left.children[var_idx], pt_left.children[range_idx]));
                                pts_nf_hyps.push_back(create_not_free_pt(tb, equalities, adaptors, pt_left.children[var_idx], pt_right.children[range_idx]));
                            }
                        }
                    }
                }
                auto pt_equal = create_equality_pt(tb, equalities, adaptors, pt_left, pt_right);

                auto pt_imp_thesis = pt_equal;
                if (!pts_eq_hyps.empty()) {
                    auto pt_all_hyps = pts_eq_hyps[0];
                    for (size_t j = 1; j < pts_eq_hyps.size(); j++) {
                        pt_all_hyps = create_conjunction_pt(tb, pt_all_hyps, pts_eq_hyps[j]);
                    }
                    pt_imp_thesis = create_implication_pt(tb, pt_all_hyps, pt_equal);
                }
                std::vector< std::pair< SymTok, ParsingTree< SymTok, LabTok > > > pts_imp_hyps;
                for (const auto &pt : pts_nf_hyps) {
                    pts_imp_hyps.push_back(std::make_pair(tb.get_turnstile(), pt));
                }
                std::cout << "   Implication form" << std::endl;
                auto res = search_theorem(tb, pts_imp_hyps, std::make_pair(tb.get_turnstile(), pt_imp_thesis), acceptable_dists);
                if (res.first && !res.second) {
                    this_found = true;
                }

                auto pt_ded_var = create_temp_var_pt(tb, tb.get_turnstile_alias());
                auto pt_ded_thesis = create_implication_pt(tb, pt_ded_var, pt_equal);
                std::vector< std::pair< SymTok, ParsingTree< SymTok, LabTok > > > pts_ded_hyps;
                for (const auto &pt : pts_eq_hyps) {
                    pts_ded_hyps.push_back(std::make_pair(tb.get_turnstile(), create_implication_pt(tb, pt_ded_var, pt)));
                }
                for (const auto &pt : pts_nf_hyps) {
                    pts_ded_hyps.push_back(std::make_pair(tb.get_turnstile(), pt));
                }
                std::cout << "   Deduction form" << std::endl;
                res = search_theorem(tb, pts_ded_hyps, std::make_pair(tb.get_turnstile(), pt_ded_thesis), acceptable_dists);
                if (res.first && !res.second) {
                    this_found = true;
                }
            }

            if (search_local) {
                bool all_local_found = true;
                Finally f3([&all_local_found,&this_found]() {
                    if (all_local_found) {
                        this_found = true;
                    }
                });
                size_t pivot_pos = 0;
                for (size_t i = 0; i < rule.size(); i++) {
                    auto pivot_var_type = rule[i];
                    if (var_types.find(pivot_var_type) != var_types.end()) {
                        auto bound_data_it = this_bound_data.second.find(pivot_pos);
                        if (bound_data_it != this_bound_data.second.end()) {
                            pivot_pos++;
                            continue;
                        }

                        bool this_local_found = false;
                        Finally f4([&this_local_found,&all_local_found]() {
                            if (!this_local_found) {
                                all_local_found = false;
                            }
                        });
                        std::cout << " * Search for a substitution rule for " << tb.resolve_symbol(pivot_var_type) << " in position " << i << std::endl;
                        ParsingTree< SymTok, LabTok > pt_hyp;
                        ParsingTree< SymTok, LabTok > pt_left;
                        ParsingTree< SymTok, LabTok > pt_right;
                        pt_left.label = label;
                        pt_left.type = type;
                        pt_right.label = label;
                        pt_right.type = type;
                        for (size_t j = 0; j < rule.size(); j++) {
                            auto var_type = rule[j];
                            if (var_types.find(var_type) != var_types.end()) {
                                ParsingTree< SymTok, LabTok > pt_var1;
                                ParsingTree< SymTok, LabTok > pt_var2;
                                pt_var1 = create_temp_var_pt(tb, var_type);
                                if (i == j) {
                                    pt_var2 = create_temp_var_pt(tb, var_type);
                                    pt_hyp = create_equality_pt(tb, equalities, adaptors, pt_var1, pt_var2);
                                } else {
                                    pt_var2 = pt_var1;
                                }
                                pt_left.children.push_back(pt_var1);
                                pt_right.children.push_back(pt_var2);
                            }
                        }
                        ParsingTree< SymTok, LabTok > pt_thesis = create_equality_pt(tb, equalities, adaptors, pt_left, pt_right);

                        std::set< std::pair< SymTok, SymTok > > acceptable_dists;
                        std::vector< ParsingTree< SymTok, LabTok > > pts_nf_hyps;
                        if (has_bound_data && pivot_pos == this_bound_data.first) {
                            // If we have bound_data for this syntax constructor, then we need to patch the body hypothesis
                            // and create not-free hypotheses
                            std::set< size_t > vars_idx;
                            for (const auto &p : boost::adaptors::reverse(this_bound_data.second)) {
                                auto var_idx = p.first;
                                auto range_idx = p.second;
                                for (const auto &prev_var_idx : vars_idx) {
                                    assert(prev_var_idx != var_idx);
                                    assert(pt_left.children[prev_var_idx].label != pt_left.children[var_idx].label);
                                    acceptable_dists.insert(std::minmax(tb.get_var_lab_to_sym(pt_left.children[prev_var_idx].label), tb.get_var_lab_to_sym(pt_left.children[var_idx].label)));
                                }
                                vars_idx.insert(var_idx);
                                assert(pt_left.children[var_idx] == pt_right.children[var_idx]);
                                if (range_idx < pt_left.children.size()) {
                                    pt_hyp = create_restr_forall_pt(tb, pt_left.children[var_idx], pt_left.children[range_idx], pt_hyp);
                                } else {
                                    pt_hyp = create_forall_pt(tb, pt_left.children[var_idx], pt_hyp);
                                }
                            }
                            for (const auto &p : this_bound_data.second) {
                                for (const auto &var_idx : vars_idx) {
                                    auto range_idx = p.second;
                                    if (range_idx < pt_left.children.size()) {
                                        pts_nf_hyps.push_back(create_not_free_pt(tb, equalities, adaptors, pt_left.children[var_idx], pt_left.children[range_idx]));
                                        pts_nf_hyps.push_back(create_not_free_pt(tb, equalities, adaptors, pt_left.children[var_idx], pt_right.children[range_idx]));
                                    }
                                }
                            }
                        }

                        /*std::cout << "   Inference form" << std::endl;
                        auto res = search_theorem(tb, {std::make_pair(tb.get_turnstile(), pt_hyp)}, std::make_pair(tb.get_turnstile(), pt_thesis));*/

                        ParsingTree< SymTok, LabTok > pt_thm = create_implication_pt(tb, pt_hyp, pt_thesis);
                        std::cout << "   Implication form" << std::endl;
                        auto res = search_theorem(tb, {}, std::make_pair(tb.get_turnstile(), pt_thm));
                        if (res.first && !res.second) {
                            this_local_found = true;
                        }

                        ParsingTree< SymTok, LabTok > pt_ph = create_temp_var_pt(tb, tb.get_turnstile_alias());
                        ParsingTree< SymTok, LabTok > pt_hypd = create_implication_pt(tb, pt_ph, pt_hyp);
                        ParsingTree< SymTok, LabTok > pt_thesisd = create_implication_pt(tb, pt_ph, pt_thesis);
                        std::cout << "   Deduction form" << std::endl;
                        res = search_theorem(tb, {std::make_pair(tb.get_turnstile(), pt_hypd)}, std::make_pair(tb.get_turnstile(), pt_thesisd));
                        if (res.first && !res.second) {
                            this_local_found = true;
                        }
                        pivot_pos++;
                    } else if (false) {  //(rule[i] == var_type) {
                        if (false) {
                            std::cout << " * Search for a not-free rule for " << tb.resolve_symbol(rule[i]) << " in position " << i << std::endl;
                            tb.new_temp_var_frame();
                            Finally f([&tb]() {
                                tb.release_temp_var_frame();
                            });
                            ParsingTree< SymTok, LabTok > pt_body;
                            pt_body.label = label;
                            pt_body.type = type;
                            ParsingTree< SymTok, LabTok > pt_var;
                            pt_var.type = {};  //var_type;
                            for (unsigned j = 0; j < rule.size(); j++) {
                                if (ders.find(rule[j]) != ders.end()) {
                                    ParsingTree< SymTok, LabTok > pt_var2;
                                    auto var = tb.new_temp_var(rule[j]);
                                    pt_var2.label = var.first;
                                    pt_var2.type = rule[j];
                                    pt_body.children.push_back(pt_var2);
                                    if (i == j) {
                                        pt_var.label = var.first;
                                    }
                                }
                            }
                            ParsingTree< SymTok, LabTok > pt_nf;
                            pt_nf.type = tb.get_turnstile_alias();
                            pt_nf.label = std::get<2>(equalities.at(type));
                            pt_nf.children.push_back(pt_var);
                            pt_nf.children.push_back(pt_body);
                            assert(pt_nf.validate(tb.get_validation_rule()));
                            std::cout << "   Searching for " << tb.print_sentence(pt_nf) << std::endl;
                            auto res = tb.unify_assertion({}, std::make_pair(tb.get_turnstile(), pt_nf));
                            if (!res.empty()) {
                                std::cout << "     Found match " << tb.resolve_label(std::get<0>(res[0])) << std::endl;
                                if (!tb.get_assertion(std::get<0>(res[0])).get_mand_dists().empty()) {
                                    std::cout << "     It has DISTINCT VARIABLES provisions!" << std::endl;
                                }
                            } else {
                                std::cout << "     Found NO match..." << std::endl;
                            }
                        }
                    }
                }
            }
        }
    }

    std::cout << std::endl << "Found " << found << " out of " << attempted << " attempts" << std::endl;

    return 0;
}
gio_static_block {
    gio::register_main_function("subst_search", subst_search_main);
}

std::set< LabTok > get_defless_labels(const LibraryToolbox &tb) {
    /* These rules have no definition, either because they are primitive
     * notions, or because they are defined in a custom way. */
    std::set< LabTok > defless_labels = {
        tb.get_label("cv"),     // class-set adapter
        tb.get_label("wn"),     // logic negation
        tb.get_label("wi"),     // logic implication
        tb.get_label("wal"),    // universal quantification

        /* The class abstraction has no definition in the usual sense: it is
         * only defined by its membership test in df-clab. */
        tb.get_label("cab"),

        /* Membership between two sets is a primitive notation; membership
         * between two classes is defined by df-clel; membership between a
         * set and a class is again defined by df-clel, but the definition is
         * circlar: once the class variable is substituted with a class
         * abstraction, df-clab can be used to solve the membership. */
        tb.get_label("wcel"),
    };
    return defless_labels;
}

std::map< LabTok, std::tuple< LabTok, std::vector< LabTok >, std::vector< LabTok >, ParsingTree< SymTok, LabTok > > > compute_defs(const LibraryToolbox &tb) {
    auto &ders = tb.get_derivations();

    auto equalities = compute_equalities(tb);

    auto defless_labels = get_defless_labels(tb);

    /* These rules can be treated as all the other, but for technical reasons
     * their actual definition is not in the form we want; however, since our
     * task here is not evaluate consistency, but apply definitions, once the
     * technical machinery is set up there are appropriate theorems that we
     * can take as definitions. */
    std::map< LabTok, LabTok > hardcoded_labels = {
        { tb.get_label("wb"), tb.get_label("dfbi1") },      // logic biimplication
        { tb.get_label("wceq"), tb.get_label("dfcleq") },   // class equality
    };

    std::map< LabTok, std::tuple< LabTok, std::vector< LabTok >, std::vector< LabTok >, ParsingTree< SymTok, LabTok > > > defs;
    for (const auto &der : ders) {
        const auto type = der.first;
        for (const auto &der2 : der.second) {
            const auto label = der2.first;
            const auto &rule = der2.second;
            if (rule.size() == 1 && tb.get_standard_is_var_sym()(rule[0])) {
                //std::cout << "   Ignoring because it is a variable" << std::endl;
                continue;
            }
            if (defless_labels.find(label) != defless_labels.end()) {
                continue;
            }
            //std::cout << "Considering derivation " << tb.resolve_label(label) <<" for type " << tb.resolve_symbol(type) << " with rule " << tb.print_sentence(rule) << std::endl;

            tb.new_temp_var_frame();
            Finally f([&tb]() {
                tb.release_temp_var_frame();
            });
            ParsingTree< SymTok, LabTok > pt_right;
            pt_right.type = type;
            pt_right.label = tb.new_temp_var(type).first;
            ParsingTree< SymTok, LabTok > pt_left;
            pt_left.type = type;
            pt_left.label = label;
            for (const auto sym : rule) {
                if (ders.find(sym) != ders.end()) {
                    auto temp_var = tb.new_temp_var(sym);
                    ParsingTree< SymTok, LabTok > pt_var;
                    pt_var.type = sym;
                    pt_var.label = temp_var.first;
                    pt_left.children.push_back(pt_var);
                }
            }
            ParsingTree< SymTok, LabTok > pt_def;
            pt_def.type = tb.get_turnstile_alias();
            pt_def.label = std::get<0>(equalities.at(type));
            pt_def.children.push_back(pt_left);
            pt_def.children.push_back(pt_right);
            assert(pt_def.validate(tb.get_validation_rule()));

            //std::cout << "   Searching for " << tb.print_sentence(pt_def) << std::endl;
            LabTok res = {};
            std::vector< LabTok > vars;
            std::vector< LabTok > fresh_vars;
            ParsingTree< SymTok, LabTok > def_body;
            for (const Assertion &ass3 : tb.gen_assertions()) {
                const Assertion *ass2 = &ass3;
                // Some notations have hardcoded defaults
                auto hard_it = hardcoded_labels.find(label);
                if (hard_it != hardcoded_labels.end()) {
                    ass2 = &tb.get_assertion(hard_it->second);
                }
                const Assertion &ass = *ass2;
                /*if (ass.is_usage_disc()) {
                    continue;
                }*/
                if (!ass.get_ess_hyps().empty()) {
                    continue;
                }
                if (tb.get_sentence(ass.get_thesis())[0] != tb.get_turnstile()) {
                    continue;
                }
                const auto &pt = tb.get_parsed_sent(ass.get_thesis());
                if (pt.label != pt_def.label) {
                    continue;
                }
                if (pt.children.at(0).label != pt_left.label) {
                    continue;
                }
                res = ass.get_thesis();
                def_body = pt.children.at(1);
                for (const auto &child : pt.children.at(0).children) {
                    assert(child.children.empty());
                    assert(tb.get_standard_is_var()(child.label));
                    vars.push_back(child.label);
                }
                std::set< LabTok > vars_set(vars.begin(), vars.end());
                std::set< LabTok > body_vars;
                collect_variables(def_body, tb.get_standard_is_var(), body_vars);
                std::set_difference(body_vars.begin(), body_vars.end(), vars_set.begin(), vars_set.end(), std::back_inserter(fresh_vars));
                break;
            }
            if (res != LabTok{}) {
                auto label_str = tb.resolve_label(res);
                //std::cout << "     Found match " << label_str << std::endl;
                std::string def_prefix = "df-";
                if (label_str.size() < def_prefix.size() || !std::equal(def_prefix.begin(), def_prefix.end(), label_str.begin())) {
                    //std::cout << "    STRANGE! It does not appear to be a definition!" << std::endl;
                }
                defs[label] = std::make_tuple(res, vars, fresh_vars, def_body);
            } else {
                //std::cout << "     Found NO match..." << std::endl;
                std::cout << "Could not find a definition for " << tb.resolve_label(label) << std::endl;
            }
        }
    }

    return defs;
}

static void compute_bound_vars_internal(const ParsingTree< SymTok, LabTok > &pt, std::set< std::pair< size_t, size_t > > &this_bound_vars,
                                        const std::vector< LabTok > &vars, const LibraryToolbox &tb,
                                        const std::map< LabTok, std::set< std::pair< size_t, size_t > > > &bound_vars,
                                        const std::map< LabTok, std::tuple< LabTok, std::vector< LabTok >, std::vector< LabTok >, ParsingTree< SymTok, LabTok > > > &defs) {
    // Iterate on the children
    for (const auto &child : pt.children) {
        compute_bound_vars_internal(child, this_bound_vars, vars, tb, bound_vars, defs);
    }

    // Check if the current syntax constructor introduces bound variables
    auto it = bound_vars.find(pt.label);
    if (it != bound_vars.end()) {
        for (const auto &dep : it->second) {
            auto var_idx = dep.first;
            auto term_idx = dep.second;
            assert(pt.children.at(var_idx).type == tb.get_symbol("setvar"));
            auto var_lab = pt.children.at(var_idx).label;
            auto var_it = std::find(vars.begin(), vars.end(), var_lab);
            if (var_it == vars.end()) {
                continue;
            }
            size_t var_idx2 = var_it - vars.begin();
            std::set< LabTok > vars2;
            collect_variables(pt.children.at(term_idx), tb.get_standard_is_var(), vars2);
            for (size_t i = 0; i < vars.size(); i++) {
                if (tb.get_var_lab_to_type_sym(vars[i]) == tb.get_symbol("setvar")) {
                    continue;
                }
                if (vars2.find(vars[i]) == vars2.end()) {
                    continue;
                }
                this_bound_vars.insert(std::make_pair(var_idx2, i));
            }
        }
    }
}

// FIXME This implementation is broken: for example, for df-ral it thinks that x binds to A
std::map< LabTok, std::set< std::pair< size_t, size_t > > > compute_bound_vars(const LibraryToolbox &tb, const std::map< LabTok, std::tuple< LabTok, std::vector< LabTok >, std::vector< LabTok >, ParsingTree< SymTok, LabTok > > > &defs) {
    std::map< LabTok, std::set< std::pair< size_t, size_t > > > bound_vars;

    // Insert bound variables data for labels without definition
    auto defless_labels = get_defless_labels(tb);
    for (const auto label : defless_labels) {
        bound_vars[label] = {};
    }
    std::vector< LabTok > defless_bound = { tb.get_label("wal"), tb.get_label("cab") };
    for (const auto label : defless_bound) {
        auto &pt = tb.get_parsed_sent(label);
        assert(pt.children.size() == 2);
        if (pt.children[0].type == tb.get_symbol("setvar")) {
            assert(pt.children[0].type == tb.get_symbol("setvar"));
            assert(pt.children[1].type == tb.get_symbol("wff"));
            bound_vars[label].insert(std::make_pair(0, 1));
        } else {
            assert(pt.children[0].type == tb.get_symbol("wff"));
            assert(pt.children[1].type == tb.get_symbol("setvar"));
            bound_vars[label].insert(std::make_pair(1, 0));
        }
    }

    // Then find all other bound variables by induction;
    // we rely on the fact that definitions in defs are already sorted
    for (const auto &def : defs) {
        auto label = std::get<0>(def.second);
        auto &vars = std::get<1>(def.second);
        auto &body = std::get<3>(def.second);
        if (defless_labels.find(label) != defless_labels.end()) {
            continue;
        }
        if (tb.get_sentence_type(label) == FLOATING_HYP) {
            continue;
        }
        compute_bound_vars_internal(body, bound_vars[def.first], vars, tb, bound_vars, defs);
    }

    return bound_vars;
}

int find_bound_vars_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto defs = compute_defs(tb);
    auto bound_vars = compute_bound_vars(tb, defs);

    for (const auto &bv : bound_vars) {
        auto def_it = defs.find(bv.first);
        if (def_it == defs.end()) {
            continue;
        }
        const auto &def = def_it->second;
        const auto &vars = std::get<1>(def);
        std::cout << "Label " << tb.resolve_label(bv.first) << " has bound variables:";
        for (const auto &p : bv.second) {
            std::cout << " (" << tb.resolve_label(vars.at(p.first)) << ", " << tb.resolve_label(vars.at(p.second)) << ")";
        }
        std::cout << std::endl;
    }

    return 0;
}
gio_static_block {
    gio::register_main_function("find_bound_vars", find_bound_vars_main);
}

ParsingTree< SymTok, LabTok > subst_defs(const ParsingTree< SymTok, LabTok > &pt, const LibraryToolbox &tb, const std::map< LabTok, std::tuple< LabTok, std::vector< LabTok >, std::vector< LabTok >, ParsingTree< SymTok, LabTok > > > &defs) {
    auto it = defs.find(pt.label);
    if (it != defs.end()) {
        // In most cases we can use the general procedure, substituting the definition
        auto &vars = std::get<1>(it->second);
        auto &fresh_vars = std::get<2>(it->second);
        auto &def_body = std::get<3>(it->second);
        assert(vars.size() == pt.children.size());
        SubstMap< SymTok, LabTok > subst_map;
        for (size_t i = 0; i < vars.size(); i++) {
            subst_map[vars[i]] = pt.children[i];
        }
        for (const auto var : fresh_vars) {
            auto &new_var = subst_map[var];
            new_var.type = tb.get_var_lab_to_type_sym(var);
            new_var.label = tb.new_temp_var(new_var.type).first;
        }
        ParsingTree< SymTok, LabTok > ret = substitute(def_body, tb.get_standard_is_var(), subst_map);
        return subst_defs(ret, tb, defs);
    } else if (pt.label == tb.get_label("wcel")) {
        // Class/set membership requires special handling
        assert(pt.children.size() == 2);

        // If the left-hand operand is a class which is not a set, then first we use df-clel, to reduce to the case where it is a set
        if (pt.children.at(0).label != tb.get_label("cv")) {
            auto pt_clel = tb.get_parsed_sent(tb.get_label("df-clel"));
            SubstMap< SymTok, LabTok > subst_map;
            subst_map[pt_clel.children.at(0).children.at(0).label] = pt.children.at(0);
            subst_map[pt_clel.children.at(0).children.at(1).label] = pt.children.at(1);
            ParsingTree< SymTok, LabTok > ret = substitute(pt_clel.children.at(1), tb.get_standard_is_var(), subst_map);
            return subst_defs(ret, tb, defs);
        }

        // Now we can assume the left-hand operand is a set, where no substitution can be made.
        // We thus continue substituting in the right-hand side.
        auto pt_right = subst_defs(pt.children.at(1), tb, defs);

        // If we obtained a class abstraction, then we can use df-clab
        if (pt_right.label == tb.get_label(("cab"))) {
            auto pt_clab = tb.get_parsed_sent(tb.get_label("df-clab"));
            SubstMap< SymTok, LabTok > subst_map;
            subst_map[pt_clab.children.at(0).children.at(0).children.at(0).label] = pt.children.at(0).children.at(0);
            subst_map[pt_clab.children.at(0).children.at(1).children.at(0).label] = pt_right.children.at(0);
            subst_map[pt_clab.children.at(0).children.at(1).children.at(1).label] = pt_right.children.at(1);
            ParsingTree< SymTok, LabTok > ret = substitute(pt_clab.children.at(1), tb.get_standard_is_var(), subst_map);
            return subst_defs(ret, tb, defs);
        }

        // In all the other cases we have either a set (in which case the notation is primitive) or a class variable (and we cannot do anything about it)
        ParsingTree< SymTok, LabTok > ret;
        ret.type = pt.type;
        ret.label = pt.label;
        ret.children.push_back(pt.children.at(0));
        ret.children.push_back(pt_right);
        return ret;
    } else {
        // In the end, if we are working with a primitive notation, then we just recur on children
        ParsingTree< SymTok, LabTok > ret;
        ret.type = pt.type;
        ret.label = pt.label;
        ret.children = gio::vector_map(pt.children.begin(), pt.children.end(), [&tb,&defs](const auto &x) { return subst_defs(x, tb, defs); });
        return ret;
    }
}

int find_defs_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto defs = compute_defs(tb);

    auto pt = tb.parse_sentence(tb.read_sentence("wff E. x ( x = _V /\\ A e. B /\\ ( ph <-> ps ) )"));
    //auto pt = tb.parse_sentence(tb.read_sentence("wff ( ph /\\ ps /\\ ch )"));
    pt.validate(tb.get_validation_rule());
    auto pt_defs = subst_defs(pt, tb, defs);
    pt_defs.validate(tb.get_validation_rule());
    std::cout << tb.print_sentence(pt) << std::endl << "becomes" << std::endl << tb.print_sentence(pt_defs) << std::endl;

    return 0;
}
gio_static_block {
    gio::register_main_function("find_defs", find_defs_main);
}

// This is essentially broken and useless code
void search_equalities() {
    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto &ders = tb.get_derivations();

    for (const auto &der : ders) {
        auto type = der.first;
        std::cout << "Searching for a reflexive equality theorem for " << tb.resolve_symbol(type) << std::endl;
        for (const Assertion &ass2 : tb.gen_assertions()) {
            const auto ass = &ass2;
            if (ass->get_ess_hyps().size() != 0) {
                continue;
            }
            if (ass->get_float_hyps().size() != 1) {
                continue;
            }
            const auto &float_hyp = tb.get_parsed_sent(ass->get_float_hyps()[0]);
            assert(float_hyp.children.empty());
            const auto var_lab = float_hyp.label;
            if (tb.get_var_lab_to_type_sym(var_lab) != type) {
                continue;
            }
            const auto &pt = tb.get_parsed_sent(ass->get_thesis());
            const auto &sent = tb.get_sentence(ass->get_thesis());
            if (sent[0] != tb.get_turnstile()) {
                continue;
            }
            if (pt.children.size() != 2 || pt.children[0] != float_hyp || pt.children[1] != float_hyp) {
                continue;
            }
            std::cout << "  Found theorem " << tb.resolve_label(ass->get_thesis()) << std::endl;
        }
    }
}
