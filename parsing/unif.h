#pragma once

#include <functional>
#include <unordered_map>
#include <map>

#include "parsing/parser.h"
#include "parsing/algos.h"
#include "utils/vectormap.h"

/*template< typename SymType, typename LabType >
using SubstMap = VectorMap< LabType, ParsingTree< SymType, LabType > >;

template< typename SymType, typename LabType >
using SubstMap2 = VectorMap< LabType, ParsingTree2< SymType, LabType > >;

template< typename SymType, typename LabType >
using SimpleSubstMap2 = VectorMap< LabType, LabType >;*/

template< typename SymType, typename LabType >
using SubstMap = std::map< LabType, ParsingTree< SymType, LabType > >;

template< typename SymType, typename LabType >
using SubstMap2 = std::map< LabType, ParsingTree2< SymType, LabType > >;

template< typename SymType, typename LabType >
using SimpleSubstMap2 = std::map< LabType, LabType >;

/*template< typename SymType, typename LabType >
using SubstMap = std::unordered_map< LabType, ParsingTree< SymType, LabType > >;

template< typename SymType, typename LabType >
using SubstMap2 = std::unordered_map< LabType, ParsingTree2< SymType, LabType > >;

template< typename SymType, typename LabType >
using SimpleSubstMap2 = std::unordered_map< LabType, LabType >;*/

template< typename SymType, typename LabType >
SubstMap2< SymType, LabType > subst_to_subst2(const SubstMap< SymType, LabType > &subst) {
    SubstMap2< SymType, LabType > subst2;
    for (const auto &x : subst) {
        subst2.insert(std::make_pair(x.first, pt_to_pt2(x.second)));
    }
    return subst2;
}

template< typename SymType, typename LabType >
SubstMap< SymType, LabType > subst2_to_subst(const SubstMap2< SymType, LabType > &subst2) {
    SubstMap< SymType, LabType > subst;
    for (const auto &x : subst2) {
        subst.insert(std::make_pair(x.first, pt2_to_pt(x.second)));
    }
    return subst;
}

template< typename SymType, typename LabType >
ParsingTree< SymType, LabType > substitute(const ParsingTree< SymType, LabType > &pt,
                                           const std::function< bool(LabType) > &is_var,
                                           const SubstMap< SymType, LabType > &subst) {
    if (is_var(pt.label)) {
        assert(pt.children.empty());
        auto it = subst.find(pt.label);
        if (it == subst.end()) {
            return pt;
        } else {
            return it->second;
        }
    } else {
        ParsingTree< SymType, LabType > ret;
        ret.label = pt.label;
        ret.type = pt.type;
        for (auto &child : pt.children) {
            ret.children.push_back(substitute(child, is_var, subst));
        }
        return ret;
    }
}


template< typename SymType, typename LabType >
size_t substitute2_count(const ParsingTree2< SymType, LabType > &pt,
                         const std::function< bool(LabType) > &is_var,
                         const SubstMap2< SymType, LabType > &subst) {
    auto nodes_len = pt.get_nodes_len();
    const auto &nodes = pt.get_nodes();
    size_t ret = nodes_len;
    for (size_t i = 0; i < nodes_len; i++) {
        auto lab = nodes[i].label;
        if (is_var(lab)) {
            auto it = subst.find(lab);
            if (it != subst.end()) {
                ret += it->second.get_nodes_len() - 1;
            }
        }
    }
    return ret;
}

template< typename SymType, typename LabType >
ParsingTree2< SymType, LabType > substitute2(const ParsingTree2< SymType, LabType > &pt,
                                             const std::function< bool(LabType) > &is_var,
                                             const SubstMap2< SymType, LabType > &subst) {
    size_t final_size = substitute2_count(pt, is_var, subst);
    ParsingTree2Generator< SymType, LabType > gen;
    gen.reserve(final_size);
    auto it = pt.get_multi_iterator();
    bool discard_next_close = false;
    //for (auto x = it.next(); x.first != it.Finished; x = it.next()) {
    while (true) {
        const auto x = it.next();
        if (x.first == it.Finished) {
            break;
        }
        if (x.first == it.Open) {
            assert(!discard_next_close);
            if (is_var(x.second.label)) {
                auto it = subst.find(x.second.label);
                if (it != subst.end()) {
                    discard_next_close = true;
                    gen.copy_tree(it->second);
                } else {
                    gen.open_node(x.second.label, x.second.type);
                }
            } else {
                gen.open_node(x.second.label, x.second.type);
            }
        } else {
            assert(x.first == it.Close);
            if (discard_next_close) {
                discard_next_close = false;
            } else {
                gen.close_node();
            }
        }
    }
    ParsingTree2< SymType, LabType > ret = gen.get_parsing_tree();
#ifdef UNIFICATOR_SELF_TEST
    assert(ret == pt_to_pt2(substitute(pt2_to_pt(pt), is_var, subst2_to_subst(subst))));
#endif
    assert(final_size == ret.nodes_storage.size());
    return ret;
}

template< typename SymType, typename LabType >
ParsingTree2< SymType, LabType > substitute2_simple(const ParsingTree2< SymType, LabType > &pt,
                                             const std::function< bool(LabType) > &is_var,
                                             const SimpleSubstMap2< SymType, LabType > &subst) {
    ParsingTree2< SymType, LabType > ret = pt;
    ret.refresh();
    for (auto &x : ret.nodes_storage) {
        if (is_var(x.label)) {
            auto it = subst.find(x.label);
            if (it != subst.end()) {
                x.label = it->second;
            }
        }
    }
    return ret;
}

template< typename SymType, typename LabType >
SubstMap< SymType, LabType > compose(const SubstMap< SymType, LabType > &first, const SubstMap< SymType, LabType > &second, const std::function< bool(LabType) > &is_var) {
    // Algorithm described in Chang, Lee (Symbolic logic and mechanical theorem proving), section 5.3 Substitution and unification
    SubstMap< SymType, LabType > ret;
    for (auto &first_pair : first) {
        auto tmp = substitute(first_pair.second, is_var, second);
        if (is_var(tmp.label)) {
            assert(tmp.children.empty());
            // We skip trivial substitutions, both for efficiency and to avoid hiding actual sostitutions from the second map
            if (tmp.label == first_pair.first) {
                continue;
            }
        }
        ret.insert(std::make_pair(first_pair.first, tmp));
    }
    for (auto &second_pair : second) {
        // Substitutions from the second map are automatically discarded by unordered_set if they are hidden by the first one
        ret.insert(second_pair);
    }
    return ret;
}

template< typename SymType, typename LabType >
SubstMap2< SymType, LabType > compose2(const SubstMap2< SymType, LabType > &first, const SubstMap2< SymType, LabType > &second, const std::function< bool(LabType) > &is_var) {
    // Algorithm described in Chang, Lee (Symbolic logic and mechanical theorem proving), section 5.3 Substitution and unification
    SubstMap2< SymType, LabType > ret;
    for (auto &first_pair : first) {
        auto tmp = substitute(first_pair.second, is_var, second);
        if (is_var(tmp.label)) {
            assert(tmp.children.empty());
            // We skip trivial substitutions, both for efficiency and to avoid hiding actual sostitutions from the second map
            if (tmp.label == first_pair.first) {
                continue;
            }
        }
        ret.insert(std::make_pair(first_pair.first, tmp));
    }
    for (auto &second_pair : second) {
        // Substitutions from the second map are automatically discarded by unordered_set if they are hidden by the first one
        ret.insert(second_pair);
    }
#ifdef UNIFICATOR_SELF_TEST
    assert(ret == subst_to_subst2(compose(subst2_to_subst(first), subst2_to_subst(second), is_var)));
#endif
    return ret;
}

template< typename SymType, typename LabType >
SubstMap< SymType, LabType > update(const SubstMap< SymType, LabType > &first, const SubstMap< SymType, LabType > &second, bool assert_disjoint = false) {
    SubstMap< SymType, LabType > ret = first;
    for (auto &second_pair : second) {
        bool inserted;
        std::tie(std::ignore, inserted) = ret.insert(second_pair);
        if (assert_disjoint) {
            assert(inserted);
        }
    }
    return ret;
}

template< typename SymType, typename LabType >
SubstMap2< SymType, LabType > update2(const SubstMap2< SymType, LabType > &first, const SubstMap2< SymType, LabType > &second, bool assert_disjoint = false) {
    SubstMap2< SymType, LabType > ret = first;
    for (auto &second_pair : second) {
        bool inserted;
        std::tie(std::ignore, inserted) = ret.insert(second_pair);
        if (assert_disjoint) {
            assert(inserted);
        }
    }
#ifdef UNIFICATOR_SELF_TEST
    assert(ret == subst_to_subst2(update(subst2_to_subst(first), subst2_to_subst(second))));
#endif
    return ret;
}

template< typename SymType, typename LabType >
bool contains_var(const ParsingTree< SymType, LabType > &pt, LabType var) {
    if (pt.label == var) {
        return true;
    }
    for (auto &child : pt.children) {
        if (contains_var(child, var)) {
            return true;
        }
    }
    return false;
}

template< typename SymType, typename LabType >
bool contains_var2(const ParsingTreeIterator< SymType, LabType > &it, LabType var) {
    if (it.get_node().label == var) {
        return true;
    }
    for (auto &child : it) {
        if (contains_var2(child, var)) {
            return true;
        }
    }
    return false;
}

template< typename SymType, typename LabType >
bool contains_var2(const ParsingTree2< SymType, LabType > &pt, LabType var) {
    return contains_var2(pt.get_root(), var);
}

template< typename SymType, typename LabType >
void collect_variables(const ParsingTree< SymType, LabType > &pt, const std::function< bool(LabType) > &is_var, std::set< LabType > &vars) {
    if (is_var(pt.label)) {
        vars.insert(pt.label);
    } else {
        for (const auto &child : pt.children) {
            collect_variables(child, is_var, vars);
        }
    }
}

template< typename SymType, typename LabType >
void collect_variables2(const ParsingTreeIterator< SymType, LabType > &it, const std::function< bool(LabType) > &is_var, std::set< LabType > &vars) {
    const auto tree = it.get_view();
    auto nodes_len = tree.get_nodes_len();
    const auto &nodes = tree.get_nodes();
    for (size_t i = 0; i < nodes_len; i++) {
        auto lab = nodes[i].label;
        if (is_var(lab)) {
            vars.insert(lab);
        }
    }
}

template< typename SymType, typename LabType >
void collect_variables2(const ParsingTree2< SymType, LabType > &pt, const std::function< bool(LabType) > &is_var, std::set< LabType > &vars) {
    return collect_variables2(pt.get_root(), is_var, vars);
}

#ifdef UNIFICATOR_SELF_TEST
// Slow unilateral unification

template< typename SymType, typename LabType >
bool unify_slow_internal(const ParsingTree< SymType, LabType > &templ, const ParsingTree< SymType, LabType > &target,
                    const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
    if (is_var(templ.label)) {
        assert(templ.children.empty());
        auto it = subst.find(templ.label);
        if (it == subst.end()) {
            subst.insert(std::make_pair(templ.label, target));
            return true;
        } else {
            return it->second == target;
        }
    } else {
        if (templ.label != target.label) {
            return false;
        } else {
            assert(templ.children.size() == target.children.size());
            for (size_t i = 0; i < templ.children.size(); i++) {
                auto res = unify_slow(templ.children[i], target.children[i], is_var, subst);
                if (!res) {
                    return false;
                }
            }
            return true;
        }
    }
}

template< typename SymType, typename LabType >
bool unify_slow(const ParsingTree< SymType, LabType > &templ, const ParsingTree< SymType, LabType > &target,
           const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
    bool ret = unify_slow_internal(templ, target, is_var, subst);
    if (ret) {
        assert(substitute(templ, is_var, subst) == target);
    }
    return ret;
}
#endif

// Unilateral unification

template< typename SymType, typename LabType >
class UnilateralUnificator {
public:
    UnilateralUnificator(const std::function< bool(LabType) > &is_var) : failed(false), is_var(&is_var) {
#ifdef UNIFICATOR_SELF_TEST
        this->pt1.label = {};
        this->pt2.label = {};
        this->pt1.type = {};
        this->pt2.type = {};
#endif
    }

    void add_parsing_trees(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2) {
        this->add_parsing_trees2(pt_to_pt2(pt1), pt_to_pt2(pt2));
    }

    bool has_failed() {
        return this->failed;
    }

    bool is_unifiable() {
        return !this->failed;
    }

    std::pair< bool, SubstMap< SymType, LabType > > unify() {
        auto ret = this->unify2();
        return std::make_pair(ret.first, subst2_to_subst(ret.second));
    }

    std::pair< bool, SubstMap2< SymType, LabType > > unify2() {
#ifdef UNIFICATOR_SELF_TEST
        bool res2;
        SubstMap< SymType, LabType > subst2;
        res2 = ::unify_slow(this->pt1, this->pt2, *this->is_var, subst2);
        assert(res2 == !this->failed);
        if (!this->failed) {
            auto s1 = substitute(this->pt1, *this->is_var, subst2_to_subst(this->subst));
            auto &s2 = this->pt2;
            auto s3 = substitute(this->pt1, *this->is_var, subst2);
            auto &s4 = this->pt2;
            assert(s1 == s2);
            assert(s3 == s4);
        }
#endif
        return std::make_pair(!this->failed, this->subst);
    }

    void add_parsing_trees2(const ParsingTree2< SymType, LabType > &pt1, const ParsingTree2< SymType, LabType > &pt2) {
#ifdef UNIFICATOR_SELF_TEST
        this->pt1.children.push_back(pt2_to_pt(pt1));
        this->pt2.children.push_back(pt2_to_pt(pt2));
#endif
        if (this->failed) {
            return;
        }
        bool res = this->process_tree(pt1.get_root(), pt2.get_root());
        if (!res) {
            this->fail();
        }
    }

private:
    void fail() {
        this->failed = true;
        this->subst.clear();
    }

    bool process_tree(ParsingTreeIterator< SymType, LabType > pt1, ParsingTreeIterator< SymType, LabType > pt2) {
        const auto end1 = pt1.end();
        const auto end2 = pt2.end();
        while (true) {
            if (pt1 == end1) {
                return pt2 == end2;
            }
            if (pt2 == end2) {
                return false;
            }
            const auto &n1 = pt1.get_node();
            const auto &n2 = pt2.get_node();
            if ((*this->is_var)(n1.label)) {
                assert(n1.descendants_num == 0);
                if (n1.type != n2.type) {
                    return false;
                }
                auto cur_subst = this->subst.find(n1.label);
                auto match = pt2.get_view();
                if (cur_subst == this->subst.end()) {
                    match.refresh();
                    this->subst[n1.label] = match;
                } else {
                    if (cur_subst->second != match) {
                        return false;
                    }
                }
                ++pt1;
                ++pt2;
            } else {
                if (n1.label != n2.label) {
                    return false;
                }
                pt1.advance();
                pt2.advance();
            }
        }
    }

    bool failed;
    const std::function< bool(LabType) > *is_var;
    SubstMap2< SymType, LabType > subst;

#ifdef UNIFICATOR_SELF_TEST
    ParsingTree< SymType, LabType > pt1;
    ParsingTree< SymType, LabType > pt2;
#endif
};

#ifdef UNIFICATOR_SELF_TEST
// Slow bilateral unification

template< typename SymType, typename LabType >
std::tuple< bool, bool > unify2_slow_step(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2,
                                                   const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
    if (pt1.label == pt2.label) {
        assert(pt1.children.size() == pt2.children.size());
        for (size_t i = 0; i < pt1.children.size(); i++) {
            bool finished;
            bool success;
            std::tie(finished, success) = unify2_slow_step(pt1.children[i], pt2.children[i], is_var, subst);
            if (!finished || !success) {
                return std::make_pair(finished, success);
            }
        }
    } else {
        if (is_var(pt1.label) && !contains_var(pt2, pt1.label)) {
            assert(pt1.children.empty());
            subst.insert(std::make_pair(pt1.label, pt2));
            return std::make_pair(false, true);
        } else if (is_var(pt2.label) && !contains_var(pt1, pt2.label)) {
            assert(pt2.children.empty());
            subst.insert(std::make_pair(pt2.label, pt1));
            return std::make_pair(false, true);
        } else {
            return std::make_pair(true, false);
        }
    }
    return std::make_pair(true, true);
}

template< typename SymType, typename LabType >
bool unify2_slow(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2,
                          const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
    // Algorithm described in Chang, Lee (Symbolic logic and mechanical theorem proving), section 5.4 Unification algorithm
    // It seems to be rather inefficient, but it is also simple, so it easier to trust; it can be used
    // to check that other implementations are correct
    ParsingTree< SymType, LabType > pt1s = substitute(pt1, is_var, subst);
    ParsingTree< SymType, LabType > pt2s = substitute(pt2, is_var, subst);
    while (true) {
        bool finished;
        bool success;
        SubstMap< SymType, LabType > new_subst;
        std::tie(finished, success) = unify2_slow_step(pt1s, pt2s, is_var, new_subst);
        if (finished) {
            return success;
        }
        pt1s = substitute(pt1s, is_var, new_subst);
        pt2s = substitute(pt2s, is_var, new_subst);
        subst = compose(subst, new_subst, is_var);
    }
}
#endif

// Bilateral unification

template< typename SymType, typename LabType >
class BilateralUnificator {
public:
    BilateralUnificator(const std::function< bool(LabType) > &is_var) : failed(false), is_var(&is_var) {
#ifdef UNIFICATOR_SELF_TEST
        this->pt1.label = {};
        this->pt2.label = {};
        this->pt1.type = {};
        this->pt2.type = {};
#endif
    }

    void add_parsing_trees(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2) {
        this->add_parsing_trees2(pt_to_pt2(pt1), pt_to_pt2(pt2));
    }

    /*
     * Return an approximation from above of failed: is has_failed() returns true, then all unifications from now on will return false.
     * However, it is possible that has_failed() returns false, but unification fails anyway.
     */
    bool has_failed() {
        return this->failed;
    }

    void add_parsing_trees2(const ParsingTree2< SymType, LabType > &pt1, const ParsingTree2< SymType, LabType > &pt2) {
#ifdef UNIFICATOR_SELF_TEST
        this->pt1.children.push_back(pt2_to_pt(pt1));
        this->pt2.children.push_back(pt2_to_pt(pt2));
#endif
        if (this->failed) {
            return;
        }
        bool res = this->process_tree(pt1.get_root(), pt2.get_root()    );
        if (!res) {
            this->fail();
        }
    }

    bool is_unifiable() {
        if (this->failed) {
            return false;
        }
        if (!this->cycle_detector.is_acyclic()) {
            this->fail();
            return false;
        }
        return true;
    }

    std::pair< bool, SubstMap< SymType, LabType > > unify() {
        auto ret = this->unify2();
        return std::make_pair(ret.first, subst2_to_subst(ret.second));
    }

    std::pair< bool, SubstMap2< SymType, LabType > > unify2() {
#ifdef UNIFICATOR_SELF_TEST
        SubstMap< SymType, LabType > subst2;
        bool res2 = unify2_slow(this->pt1, this->pt2, *this->is_var, subst2);
#endif
        if (this->failed) {
#ifdef UNIFICATOR_SELF_TEST
            assert(!res2);
#endif
            return std::make_pair(false, this->subst);
        }
        //std::cerr << "Final graph has " << this->cycle_detector.get_node_num() << " nodes and " << this->cycle_detector.get_edge_num() << " edges (load factor: " << ((double) this->cycle_detector.get_edge_num()) / ((double) this->cycle_detector.get_node_num()) << ")" << std::endl;
        bool res;
        std::vector< LabType > topo_sort;
        tie(res, topo_sort) = this->cycle_detector.find_topo_sort();
        if (!res) {
#ifdef UNIFICATOR_SELF_TEST
            assert(!res2);
#endif
            this->fail();
            return std::make_pair(false, this->subst);
        }
        SubstMap2< SymType, LabType > actual_subst;
        for (const LabType &lab : topo_sort) {
            actual_subst[lab] = substitute2(this->subst[lab], *this->is_var, actual_subst);
        }
#ifdef UNIFICATOR_SELF_TEST
        assert(res2);
        auto s1 = substitute(this->pt1, *this->is_var, subst2_to_subst(actual_subst));
        auto s2 = substitute(this->pt2, *this->is_var, subst2_to_subst(actual_subst));
        auto s3 = substitute(this->pt1, *this->is_var, subst2);
        auto s4 = substitute(this->pt2, *this->is_var, subst2);
        assert(s1 == s2);
        assert(s3 == s4);
#endif
        return std::make_pair(true, actual_subst);
    }

private:
    void fail() {
        // Once we have failed, there is not turning back: we can directly release resources
        this->failed = true;
        this->subst.clear();
        this->djs.clear();
        this->cycle_detector.clear();
    }

    bool process_tree(const ParsingTreeIterator< SymType, LabType > &pt1, const ParsingTreeIterator< SymType, LabType > &pt2) {
        const auto &n1 = pt1.get_node();
        const auto &n2 = pt2.get_node();
        if (n1.label == n2.label) {
            auto it1 = pt1.begin();
            auto it2 = pt2.begin();
            while (it1 != pt1.end() && it2 != pt2.end()) {
                bool res = this->process_tree(*it1, *it2);
                if (!res) {
                    return false;
                }
                ++it1;
                ++it2;
            }
            assert(it1 == pt1.end() && it2 == pt2.end());
            return true;
        } else {
            LabType var;
            ParsingTree2< SymType, LabType > pt_temp;
            bool v1 = (*this->is_var)(n1.label);
            bool v2 = (*this->is_var)(n2.label);
            assert(!(v1 && pt1.has_children()));
            assert(!(v2 && pt2.has_children()));
            if (n1.type != n2.type) {
                return false;
            }
            if (v1 && v2) {
                // If both are variables, then we use the disjoint set structure to determine how to substitute
                this->djs.make_set(n1.label);
                this->djs.make_set(n2.label);
                LabType l1 = this->djs.find_set(n1.label);
                LabType l2 = this->djs.find_set(n2.label);
                if (l1 != l2) {
                    bool res;
                    LabType l3;
                    std::tie(res, l3) = this->djs.union_set(l1, l2);
                    assert(res);
                    assert(l3 == l1 || l3 == l2);
                    if (l3 == l1) {
                        pt_temp = var_parsing_tree(l1, n1.type);
                        var = l2;
                    } else {
                        pt_temp = var_parsing_tree(l2, n2.type);
                        var = l1;
                    }
                } else {
                    return true;
                }
            } else if (v1) {
                var = n1.label;
                pt_temp = pt2.get_view();
            } else if (v2) {
                var = n2.label;
                pt_temp = pt1.get_view();
            } else {
                return false;
            }
            bool res;
            typename SubstMap2< SymType, LabType >::iterator it;
            // In general we cannot assume that the caller will retain pt1 and pt2 for a long time
            pt_temp.refresh();
            std::tie(it, res) = this->subst.insert(std::make_pair(var, pt_temp));
            this->cycle_detector.make_node(var);
            if (res) {
                std::set< LabType > vars;
                collect_variables2(pt_temp, *this->is_var, vars);
                for (const auto &var2 : vars) {
                    cycle_detector.make_edge(var, var2);
                }
                return true;
            } else {
                return this->process_tree(it->second.get_root(), pt_temp.get_root());
            }
        }
    }

    bool failed;
    const std::function< bool(LabType) > *is_var;
    SubstMap2< SymType, LabType > subst;
    DisjointSet< LabType > djs;
    NaiveIncrementalCycleDetector< LabType > cycle_detector;

#ifdef UNIFICATOR_SELF_TEST
    ParsingTree< SymType, LabType > pt1;
    ParsingTree< SymType, LabType > pt2;
#endif
};
