#ifndef UNIF_H
#define UNIF_H

#include <functional>
#include <unordered_map>

#include "parser.h"

//#define UNIFICATOR_SELF_TEST

template< typename SymType, typename LabType >
using SubstMap = std::unordered_map< LabType, ParsingTree< SymType, LabType > >;

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
std::tuple< bool, bool > unify2_internal_slow_step(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2,
                                                   bool sub1, bool sub2,
                                                   const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
    if (pt1.label == pt2.label) {
        assert(pt1.children.size() == pt2.children.size());
        for (size_t i = 0; i < pt1.children.size(); i++) {
            bool finished;
            bool success;
            std::tie(finished, success) = unify2_internal_slow_step(pt1.children[i], pt2.children[i], sub1, sub2, is_var, subst);
            if (!finished || !success) {
                return std::make_pair(finished, success);
            }
        }
    } else {
        if (sub1 && is_var(pt1.label) && (!sub2 || !contains_var(pt2, pt1.label))) {
            assert(pt1.children.empty());
            subst.insert(std::make_pair(pt1.label, pt2));
            return std::make_pair(false, true);
        } else if (sub2 && is_var(pt2.label) && (!sub1 || !contains_var(pt1, pt2.label))) {
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
bool unify2_internal_slow(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2,
                          bool sub1, bool sub2,
                          const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
    // Algorithm described in Chang, Lee (Symbolic logic and mechanical theorem proving), section 5.4 Unification algorithm
    // It seems to be rather inefficient, but it is also simple, so it easier to trust; it can be used
    // to check that other implementations are correct
    ParsingTree< SymType, LabType > pt1s = sub1 ? substitute(pt1, is_var, subst) : pt1;
    ParsingTree< SymType, LabType > pt2s = sub2 ? substitute(pt2, is_var, subst) : pt2;
    while (true) {
        bool finished;
        bool success;
        SubstMap< SymType, LabType > new_subst;
        std::tie(finished, success) = unify2_internal_slow_step(pt1s, pt2s, sub1, sub2, is_var, new_subst);
        if (finished) {
            return success;
        }
        if (sub1) {
            pt1s = substitute(pt1s, is_var, new_subst);
        }
        if (sub2) {
            pt2s = substitute(pt2s, is_var, new_subst);
        }
        if (sub1 && sub2) {
            subst = compose(subst, new_subst, is_var);
        } else {
            subst = update(subst, new_subst);
        }
    }
}

// FIXME Finish
template< typename SymType, typename LabType >
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
}

template< typename SymType, typename LabType >
bool unify(const ParsingTree< SymType, LabType > &templ, const ParsingTree< SymType, LabType > &target,
           const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
#ifdef UNIFICATOR_SELF_TEST
    SubstMap< SymType, LabType > subst2 = subst;
#endif
    // The generic use of unify2 does not work yet
    //bool ret = unify2(templ, target, true, false, is_var, subst);
    bool ret = unify_internal(templ, target, is_var, subst);
#ifdef UNIFICATOR_SELF_TEST
    bool ret2 = unify_internal(templ, target, is_var, subst2);
    assert(ret == ret2);
    if (ret) {
        assert(subst == subst2);
        assert(substitute(templ, is_var, subst) == target);
    }
#endif
    return ret;
}

template< typename SymType, typename LabType >
bool unify2(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2,
            bool sub1, bool sub2,
            const std::function< bool(LabType) > &is_var, SubstMap< SymType, LabType > &subst) {
    bool ret = unify2_internal_slow(pt1, pt2, sub1, sub2, is_var, subst);
#ifdef UNIFICATOR_SELF_TEST
    if (ret) {
        auto s1 = sub1 ? substitute(pt1, is_var, subst) : pt1;
        auto s2 = sub2 ? substitute(pt2, is_var, subst) : pt2;
        assert(s1 == s2);
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

#endif // UNIF_H
