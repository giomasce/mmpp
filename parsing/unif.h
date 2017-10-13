#ifndef UNIF_H
#define UNIF_H

#include <functional>
#include <unordered_map>

#include "parsing/parser.h"

template< typename SymType, typename LabType >
using SubstMap = std::unordered_map< LabType, ParsingTree< SymType, LabType > >;

// Implementation from https://en.wikipedia.org/wiki/Disjoint-set_data_structure
template< typename LabType >
class DisjointSet {
public:
    void make_set(LabType lab) {
        // Only insert if the node does not exists yet
        this->parent.insert(std::make_pair(lab, lab));
        this->rank.insert(std::make_pair(lab, 0));
    }

    LabType find_set(LabType lab) {
        if (this->parent[lab] != lab) {
            this->parent[lab] = this->find_set(parent[lab]);
        }
        return parent[lab];
    }

    std::pair< bool, LabType > union_set(LabType l1, LabType l2) {
        l1 = this->find_set(l1);
        l2 = this->find_set(l2);
        if (l1 == l2) {
            return std::make_pair(false, l1);
        }
        if (this->rank[l1] < this->rank[l2]) {
            this->parent[l1] = l2;
            return std::make_pair(true, l2);
        } else if (this->rank[l1] > this->rank[l2]) {
            this->parent[l2] = l1;
            return std::make_pair(true, l1);
        } else {
            this->parent[l2] = l1;
            this->rank[l1]++;
            return std::make_pair(true, l1);
        }
    }

private:
    std::unordered_map< LabType, LabType > parent;
    std::unordered_map< LabType, size_t > rank;
};

template< typename SymType, typename LabType >
void collect_variables(const ParsingTree< SymType, LabType > &pt, const std::function< bool(LabType) > &is_var, std::set< LabType > &vars) {
    if (is_var(pt.label)) {
        assert(pt.children.empty());
        vars.insert(pt.label);
    } else {
        for (const auto &child : pt.children) {
            collect_variables(child, is_var, vars);
        }
    }
}

template< typename SymType, typename LabType >
bool unify_internal(const ParsingTree< SymType, LabType > &templ, const ParsingTree< SymType, LabType > &target,
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
                if (!unify(templ.children[i], target.children[i], is_var, subst)) {
                    return false;
                }
            }
            return true;
        }
    }
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
SubstMap< SymType, LabType > update(const SubstMap< SymType, LabType > &first, const SubstMap< SymType, LabType > &second) {
    SubstMap< SymType, LabType > ret = first;
    for (auto &second_pair : second) {
        ret.insert(second_pair);
    }
    return ret;
}

template< typename SymType, typename LabType >
bool contains_var(const ParsingTree< SymType, LabType > &pt, LabType var) {
    if (pt.label == var) {
        assert(pt.children.empty());
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

template< typename SymType, typename LabType >
bool unify2_quick_process_tree(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2,
                               const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst,
                               std::unordered_map< LabType, std::set< LabType > > &deps,
                               DisjointSet< LabType > &djs) {
    if (pt1.label == pt2.label) {
        assert(pt1.children.size() == pt2.children.size());
        for (size_t i = 0; i < pt1.children.size(); i++) {
            bool res = unify2_quick_process_tree(pt1.children[i], pt2.children[i], is_var, subst, deps, djs);
            if (!res) {
                return false;
            }
        }
        return true;
    } else {
        LabType var;
        ParsingTree< SymTok, LabTok > pt_temp;
        const ParsingTree< SymType, LabType > *content;
        bool v1 = is_var(pt1.label);
        bool v2 = is_var(pt2.label);
        assert(!(v1 && !pt1.children.empty()));
        assert(!(v2 && !pt2.children.empty()));
        if (v1 && v2) {
            // If both are variables, then we use the disjoint set structure to determine how to substitute
            djs.make_set(pt1.label);
            djs.make_set(pt2.label);
            LabType l1 = djs.find_set(pt1.label);
            LabType l2 = djs.find_set(pt2.label);
            if (l1 != l2) {
                bool res;
                LabTok l3;
                std::tie(res, l3) = djs.union_set(l1, l2);
                assert(res);
                assert(l3 == l1 || l3 == l2);
                if (l3 == l1) {
                    pt_temp.label = l1;
                    pt_temp.type = pt1.type;
                    var = l2;
                    content = &pt_temp;
                } else {
                    pt_temp.label = l2;
                    pt_temp.type = pt2.type;
                    var = l1;
                    content = &pt_temp;
                }
            } else {
                return true;
            }
        } else if (v1) {
            var = pt1.label;
            content = &pt2;
        } else if (v2) {
            var = pt2.label;
            content = &pt1;
        } else {
            return false;
        }
        bool res;
        typename SubstMap< SymType, LabType >::iterator it;
        std::tie(it, res) = subst.insert(std::make_pair(var, *content));
        if (res) {
            collect_variables(*content, is_var, deps[var]);
            return true;
        } else {
            return unify2_quick_process_tree(it->second, *content, is_var, subst, deps, djs);
        }
    }
}

enum Status { Unvisited = 0, Visiting, Visited };
template< typename SymType, typename LabType >
bool unify2_quick_resolve_deps(LabType lab, SubstMap< SymType, LabType > &subst, std::unordered_map< LabType, Status > &status, const std::function< bool(LabType) > &is_var, const std::unordered_map< LabType, std::set< LabType > > &deps) {
    auto it = deps.find(lab);
    if (it == deps.end()) {
        return true;
    }
    if (status[lab] == Visited) {
        return true;
    }
    if (status[lab] == Visiting) {
        return false;
    }
    status[lab] = Visiting;
    for (const auto &new_lab : it->second) {
        bool res = unify2_quick_resolve_deps(new_lab, subst, status, is_var, deps);
        if (!res) {
            return false;
        }
    }
    subst[lab] = substitute(subst[lab], is_var, subst);
    status[lab] = Visited;
    return true;
}

template< typename SymType, typename LabType >
std::pair< bool, SubstMap< SymType, LabType > > unify2_quick(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2,
                  const std::function< bool(LabType) > &is_var) {
    SubstMap< SymType, LabType > subst;
    std::unordered_map< LabType, std::set< LabType > > deps;
    DisjointSet< LabType > djs;
    bool res = unify2_quick_process_tree(pt1, pt2, is_var, subst, deps, djs);
#ifdef UNIFICATOR_SELF_TEST
    SubstMap< SymType, LabType > subst2;
    bool res2 = unify2_slow(pt1, pt2, is_var, subst2);
#endif
    if (!res) {
#ifdef UNIFICATOR_SELF_TEST
        assert(!res2);
#endif
        subst.clear();
        return std::make_pair(false, subst);
    }
    std::unordered_map< LabType, Status > status;
    for (const auto &p : deps) {
        bool res = unify2_quick_resolve_deps(p.first, subst, status, is_var, deps);
        if (!res) {
#ifdef UNIFICATOR_SELF_TEST
            assert(!res2);
#endif
            subst.clear();
            return std::make_pair(false, subst);
        }
    }
#ifdef UNIFICATOR_SELF_TEST
    assert(res2);
    auto s1 = substitute(pt1, is_var, subst);
    auto s2 = substitute(pt2, is_var, subst);
    auto s3 = substitute(pt1, is_var, subst2);
    auto s4 = substitute(pt2, is_var, subst2);
    assert(s1 == s2);
    assert(s3 == s4);
#endif
    return std::make_pair(true, subst);
}

template< typename SymType, typename LabType >
bool unify2_quick_adapter(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2,
                  const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
    ParsingTree< SymType, LabType > pt1s = substitute(pt1, is_var, subst);
    ParsingTree< SymType, LabType > pt2s = substitute(pt2, is_var, subst);
    bool ret;
    SubstMap< SymType, LabType > new_subst;
    std::tie(ret, new_subst) = unify2_quick(pt1s, pt2s, is_var);
    if (ret) {
        subst = compose(subst, new_subst, is_var);
    }
    return ret;
}

// FIXME Finish
/*template< typename SymType, typename LabType >
bool unify2_internal(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2,
                     bool sub1, bool sub2,
                     const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
    throw "Not yet finished";
    if (sub1 && is_var(pt1.label)) {
        assert(pt1.children.empty());

    } else if (sub2 && is_var(pt2.label)) {

    } else {
        if (pt1.label != pt2.label) {
            return false;
        } else {
            assert(pt1.children.size() == pt2.children.size());
            for (size_t i = 0; i < pt1.children.size(); i++) {
                if (!unify2(pt1.children[i], pt2.children[i], sub1, sub2, is_var, subst)) {
                    return false;
                }
            }
            return true;
        }
    }
}*/

template< typename SymType, typename LabType >
bool unify(const ParsingTree< SymType, LabType > &templ, const ParsingTree< SymType, LabType > &target,
           const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
    bool ret = unify_internal(templ, target, is_var, subst);
#ifdef UNIFICATOR_SELF_TEST
    if (ret) {
        assert(substitute(templ, is_var, subst) == target);
    }
#endif
    return ret;
}

template< typename SymType, typename LabType >
bool unify2(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2,
            const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
#ifdef UNIFICATOR_SELF_TEST
    SubstMap< SymType, LabType > subst2 = subst;
#endif
    bool ret = unify2_quick_adapter(pt1, pt2, is_var, subst);
#ifdef UNIFICATOR_SELF_TEST
    bool ret2 = unify2_slow(pt1, pt2, is_var, subst2);
    assert(ret == ret2);
    if (ret) {
        auto s1 = substitute(pt1, is_var, subst);
        auto s2 = substitute(pt2, is_var, subst);
        auto s3 = substitute(pt1, is_var, subst2);
        auto s4 = substitute(pt2, is_var, subst2);
        assert(s1 == s2);
        assert(s3 == s4);
    }
#endif
    return ret;
}

template< typename SymType, typename LabType >
std::pair< bool, SubstMap< SymType, LabType > > unify(const ParsingTree< SymType, LabType > &templ,
                                                                                        const ParsingTree< SymType, LabType > &target,
                                                                                        const std::function< bool(LabType) > &is_var) {
    SubstMap< SymType, LabType > subst;
    bool res = unify(templ, target, is_var, subst);
    if (!res) {
        subst = {};
    }
    return std::make_pair(res, subst);
}

template< typename SymType, typename LabType >
class Unificator {
public:
    Unificator(const std::function< bool(LabType) > &is_var) : is_var(is_var) {
        this->pt1.label = {};
        this->pt2.label = {};
        this->pt1.type = {};
        this->pt2.type = {};
    }

    void add_parsing_trees(const ParsingTree< SymType, LabType > &new_pt1, const ParsingTree< SymType, LabType > &new_pt2) {
        this->pt1.children.push_back(new_pt1);
        this->pt2.children.push_back(new_pt2);
    }

    std::pair< bool, SubstMap< SymType, LabType > > unify() const {
        bool ret1;
        SubstMap< SymType, LabType > ret2;
        std::tie(ret1, ret2) = ::unify(this->pt1, this->pt2, this->is_var);
        return std::make_pair(ret1, ret2);
    }

    std::pair< bool, SubstMap< SymType, LabType > > unify2() const {
        bool ret1;
        SubstMap< SymType, LabType > ret2;
        std::tie(ret1, ret2) = ::unify2_quick(this->pt1, this->pt2, this->is_var);
        return std::make_pair(ret1, ret2);
    }

private:
    const std::function< bool(LabType) > &is_var;
    ParsingTree< SymType, LabType > pt1;
    ParsingTree< SymType, LabType > pt2;
};

#endif // UNIF_H
