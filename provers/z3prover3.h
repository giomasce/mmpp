#pragma once

#include <z3++.h>

namespace gio::mmpp::z3prover {

unsigned expr_get_num_bound(const z3::expr &e);
z3::symbol expr_get_quantifier_bound_name(const z3::expr &e, unsigned i);
z3::sort expr_get_quantifier_bound_sort(const z3::expr &e, unsigned i);
bool expr_is_quantifier_forall(const z3::expr &e);
unsigned expr_get_var_index(const z3::expr &e);

}
