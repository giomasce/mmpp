#ifndef UNIF_H
#define UNIF_H

#include <functional>
#include <unordered_map>

#include "parser.h"

#define UNIFICATOR_SELF_TEST

template< typename SymType, typename LabType >
bool unify_internal(const ParsingTree< SymType, LabType > &templ, const ParsingTree< SymType, LabType > &target,
                    const std::function< bool(LabType) > &is_var, std::unordered_map< LabType, ParsingTree< SymType, LabType > > &subst) {
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
                                           std::unordered_map< LabType, ParsingTree< SymType, LabType > > &subst) {
    if (is_var(pt.label)) {
        assert(pt.children.empty());
        return subst.at(pt.label);
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
bool unify2_internal(const ParsingTree< SymType, LabType > &pt1, const ParsingTree< SymType, LabType > &pt2,
                     bool sub1, bool sub2,
                     const std::function< bool(LabType) > &is_var, std::unordered_map< LabType, ParsingTree< SymType, LabType > > &subst) {
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
           const std::function< bool(LabType) > &is_var, std::unordered_map< LabType, ParsingTree< SymType, LabType > > &subst) {
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
            bool sub1, bool sub2,
            const std::function< bool(LabType) > &is_var, std::unordered_map< LabType, ParsingTree< SymType, LabType > > &subst) {
    bool ret = unify2_internal(pt1, pt2, sub1, sub2, is_var, subst);
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
std::pair< bool, std::unordered_map< LabType, ParsingTree< SymType, LabType > > > unify(const ParsingTree< SymType, LabType > &templ,
                                                                                        const ParsingTree< SymType, LabType > &target,
                                                                                        const std::function< bool(LabType) > &is_var) {
    std::unordered_map< LabType, ParsingTree< SymType, LabType > > subst;
    bool res = unify(templ, target, is_var, subst);
    if (!res) {
        subst = {};
    }
    return std::make_pair(res, subst);
}

#endif // UNIF_H
