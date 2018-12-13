
#include "z3resolver.h"

#include <type_traits>

#include <z3++.h>

/* A collection of tricks needed because Z3 does not seem to support
 * compile-time version detection, and some identifiers appear or
 * disappear in different versions. */

// First some stuff from library fundamentals TS v2
struct nonesuch {
    ~nonesuch() = delete;
    nonesuch(const nonesuch&) = delete;
    void operator=(const nonesuch&) = delete;
};

namespace detail {
template <class Default, class AlwaysVoid,
          template<class...> class Op, class... Args>
struct detector {
  using value_t = std::false_type;
  using type = Default;
};

template <class Default, template<class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
  // Note that std::void_t is a C++17 feature
  using value_t = std::true_type;
  using type = Op<Args...>;
};

} // namespace detail

template <template<class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template <template<class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template <class Default, template<class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

// Then some auxiliary template for the identifiers to be tested
template<typename T>
using has_Z3_OP_PR_PULL_QUANT_STAR_t = decltype(T::Z3_OP_PR_PULL_QUANT_STAR);

template<typename T>
constexpr bool has_Z3_OP_PR_PULL_QUANT_STAR = is_detected<has_Z3_OP_PR_PULL_QUANT_STAR_t, T>::value;

template<typename T>
constexpr unsigned Z3_OP_PR_PULL_QUANT_STAR_value = T::Z3_OP_PR_PULL_QUANT_STAR;

template<typename T>
using has_Z3_OP_PR_NNF_STAR_t = decltype(T::Z3_OP_PR_NNF_STAR);

template<typename T>
constexpr bool has_Z3_OP_PR_NNF_STAR = is_detected<has_Z3_OP_PR_NNF_STAR_t, T>::value;

template<typename T>
constexpr unsigned Z3_OP_PR_NNF_STAR_value = T::Z3_OP_PR_NNF_STAR;

template<typename T>
using has_Z3_OP_PR_CNF_STAR_t = decltype(T::Z3_OP_PR_CNF_STAR);

template<typename T>
constexpr bool has_Z3_OP_PR_CNF_STAR = is_detected<has_Z3_OP_PR_CNF_STAR_t, T>::value;

template<typename T>
constexpr unsigned Z3_OP_PR_CNF_STAR_value = T::Z3_OP_PR_CNF_STAR;

// And at last the resolver function, with special cases for identifiers that change among versions
template<typename T>
std::string get_inference_str_impl(unsigned x) {
    if constexpr (has_Z3_OP_PR_PULL_QUANT_STAR<T>) {
        if (x == Z3_OP_PR_PULL_QUANT_STAR_value<T>) {
            return "Z3_OP_PR_PULL_QUANT_STAR";
        }
    }
    if constexpr (has_Z3_OP_PR_NNF_STAR<T>) {
        if (x == Z3_OP_PR_NNF_STAR_value<T>) {
            return "Z3_OP_PR_NNF_STAR";
        }
    }
    if constexpr (has_Z3_OP_PR_CNF_STAR<T>) {
        if (x == Z3_OP_PR_CNF_STAR_value<T>) {
            return "Z3_OP_PR_CNF_STAR";
        }
    }
    switch (x) {
    case Z3_OP_PR_UNDEF: return "Z3_OP_PR_UNDEF";
    case Z3_OP_PR_TRUE: return "Z3_OP_PR_TRUE";
    case Z3_OP_PR_ASSERTED: return "Z3_OP_PR_ASSERTED";
    case Z3_OP_PR_GOAL: return "Z3_OP_PR_GOAL";
    case Z3_OP_PR_MODUS_PONENS: return "Z3_OP_PR_MODUS_PONENS";
    case Z3_OP_PR_REFLEXIVITY: return "Z3_OP_PR_REFLEXIVITY";
    case Z3_OP_PR_SYMMETRY: return "Z3_OP_PR_SYMMETRY";
    case Z3_OP_PR_TRANSITIVITY: return "Z3_OP_PR_TRANSITIVITY";
    case Z3_OP_PR_TRANSITIVITY_STAR: return "Z3_OP_PR_TRANSITIVITY_STAR";
    case Z3_OP_PR_MONOTONICITY: return "Z3_OP_PR_MONOTONICITY";
    case Z3_OP_PR_QUANT_INTRO: return "Z3_OP_PR_QUANT_INTRO";
    case Z3_OP_PR_DISTRIBUTIVITY: return "Z3_OP_PR_DISTRIBUTIVITY";
    case Z3_OP_PR_AND_ELIM: return "Z3_OP_PR_AND_ELIM";
    case Z3_OP_PR_NOT_OR_ELIM: return "Z3_OP_PR_NOT_OR_ELIM";
    case Z3_OP_PR_REWRITE: return "Z3_OP_PR_REWRITE";
    case Z3_OP_PR_REWRITE_STAR: return "Z3_OP_PR_REWRITE_STAR";
    case Z3_OP_PR_PULL_QUANT: return "Z3_OP_PR_PULL_QUANT";
    case Z3_OP_PR_PUSH_QUANT: return "Z3_OP_PR_PUSH_QUANT";
    case Z3_OP_PR_ELIM_UNUSED_VARS: return "Z3_OP_PR_ELIM_UNUSED_VARS";
    case Z3_OP_PR_DER: return "Z3_OP_PR_DER";
    case Z3_OP_PR_QUANT_INST: return "Z3_OP_PR_QUANT_INST";
    case Z3_OP_PR_HYPOTHESIS: return "Z3_OP_PR_HYPOTHESIS";
    case Z3_OP_PR_LEMMA: return "Z3_OP_PR_LEMMA";
    case Z3_OP_PR_UNIT_RESOLUTION: return "Z3_OP_PR_UNIT_RESOLUTION";
    case Z3_OP_PR_IFF_TRUE: return "Z3_OP_PR_IFF_TRUE";
    case Z3_OP_PR_IFF_FALSE: return "Z3_OP_PR_IFF_FALSE";
    case Z3_OP_PR_COMMUTATIVITY: return "Z3_OP_PR_COMMUTATIVITY";
    case Z3_OP_PR_DEF_AXIOM: return "Z3_OP_PR_DEF_AXIOM";
    case Z3_OP_PR_DEF_INTRO: return "Z3_OP_PR_DEF_INTRO";
    case Z3_OP_PR_APPLY_DEF: return "Z3_OP_PR_APPLY_DEF";
    case Z3_OP_PR_IFF_OEQ: return "Z3_OP_PR_IFF_OEQ";
    case Z3_OP_PR_NNF_POS: return "Z3_OP_PR_NNF_POS";
    case Z3_OP_PR_NNF_NEG: return "Z3_OP_PR_NNF_NEG";
    case Z3_OP_PR_SKOLEMIZE: return "Z3_OP_PR_SKOLEMIZE";
    case Z3_OP_PR_MODUS_PONENS_OEQ: return "Z3_OP_PR_MODUS_PONENS_OEQ";
    case Z3_OP_PR_TH_LEMMA: return "Z3_OP_PR_TH_LEMMA";
    case Z3_OP_PR_HYPER_RESOLVE: return "Z3_OP_PR_HYPER_RESOLVE";
    default: return "(unknown)";
    }
}

std::string get_inference_str(unsigned x) {
    return get_inference_str_impl<Z3_decl_kind>(x);
}
