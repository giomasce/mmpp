
#include "fof_to_mm.h"

#include <boost/range/adaptor/reversed.hpp>
#include <boost/type_index.hpp>

#include "setmm.h"

namespace gio::mmpp::provers::fof {

template<typename RealEngine, typename Engine>
class prover_checker {
public:
    prover_checker(const LibraryToolbox &tb, const Prover<Engine> &prover, const typename RealEngine::SentType &sent) : tb(tb), prover(prover), sent(sent) {}
    bool operator()(Engine &engine) {
        bool res = this->prover(engine);
        gio::assert_or_throw<std::runtime_error>(res, "prover failed");
        if (res) {
            auto rengine = dynamic_cast<RealEngine*>(&engine);
            gio::assert_or_throw<std::runtime_error>(rengine, "cannot cast engine");
            const auto &stack = rengine->get_stack();
            gio::assert_or_throw<std::runtime_error>(stack.size() >= 1, "engine's stack is empty");
            gio::assert_or_throw<std::runtime_error>(stack.back() == this->sent, gio_make_string("wrong sentence on the top of the stack: " << tb.print_sentence(stack.back()) << " instead of " << tb.print_sentence(this->sent)));
        }
        return res;
    }

private:
    const LibraryToolbox &tb;
    Prover<Engine> prover;
    typename RealEngine::SentType sent;
};

fof_to_mm_ctx::fof_to_mm_ctx(const LibraryToolbox &tb) : tb(tb), tsa(tb) {}

void fof_to_mm_ctx::alloc_var(const std::string &name, LabTok label) {
    using namespace gio::mmpp::setmm;
    auto it = this->vars.find(name);
    gio::assert_or_throw<std::invalid_argument>(it == this->vars.end(), "redefining variable " + name);
    this->vars.insert(std::make_pair(name, label));
}

void fof_to_mm_ctx::alloc_functor(const std::string &name, const std::vector<LabTok> &vars, Prover<CheckpointedProofEngine> final_prover, Prover<CheckpointedProofEngine> sethood_prover, std::function<Prover<CheckpointedProofEngine>(LabTok)> not_free_prover) {
    using namespace gio::mmpp::setmm;
    auto it = this->functs.find(name);
    gio::assert_or_throw<std::invalid_argument>(it == this->functs.end(), "redefining functor " + name);
    this->functs.insert(std::make_pair(name, std::make_tuple(vars, final_prover, sethood_prover, not_free_prover)));
}

void fof_to_mm_ctx::alloc_predicate(const std::string &name, const std::vector<LabTok> &vars, Prover<CheckpointedProofEngine> final_prover, std::function<Prover<CheckpointedProofEngine>(LabTok)> not_free_prover) {
    using namespace gio::mmpp::setmm;
    auto it = this->preds.find(name);
    gio::assert_or_throw<std::invalid_argument>(it == this->preds.end(), "redefining predicate " + name);
    this->preds.insert(std::make_pair(name, std::make_tuple(vars, final_prover, not_free_prover)));
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::convert_prover(const std::shared_ptr<const FOT> &fot, bool make_class) const {
    using namespace gio::mmpp::setmm;
    if (const auto fot_funct = fot->mapped_dynamic_cast<const Functor>()) {
        return this->convert_functor_prover(fot_funct);
    } else if (const auto fot_var = fot->mapped_dynamic_cast<const Variable>()) {
        Prover<CheckpointedProofEngine> prover;
        try {
            prover = build_label_prover(this->tb, this->vars.at(fot_var->get_name()));
        } catch (std::out_of_range &e) {
            std::cerr << "unknown variable " << fot_var->get_name() << "\n";
            throw e;
        }
        if (make_class) {
            prover = build_var_adaptor_prover(this->tb, prover);
        }
        return prover;
    } else {
        gio_should_not_arrive_here_ctx(boost::typeindex::type_id_runtime(*fot).pretty_name());
    }
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::convert_functor_prover(const std::shared_ptr<const Functor> &func, size_t start_idx) const {
    using namespace gio::mmpp::setmm;
    const auto &data = this->functs.at(func->get_name());
    if (start_idx == func->get_args().size()) {
        return std::get<1>(data);
    } else {
        auto prover = this->convert_functor_prover(func, start_idx+1);
        const auto &arg = func->get_args().at(start_idx);
        prover = build_class_subst_prover(this->tb, build_label_prover(this->tb, std::get<0>(data).at(start_idx)), this->convert_prover(arg, true), prover);
        return prover;
    }
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::convert_prover(const std::shared_ptr<const gio::mmpp::provers::fof::FOF> &fof) const {
    using namespace gio::mmpp::setmm;
    if (const auto fof_true = fof->mapped_dynamic_cast<const True>()) {
        return build_true_prover(this->tb);
    } else if (const auto fof_false = fof->mapped_dynamic_cast<const False>()) {
        return build_false_prover(this->tb);
    } else if (const auto fof_not = fof->mapped_dynamic_cast<const Not>()) {
        return build_not_prover(this->tb, this->convert_prover(fof_not->get_arg()));
    } else if (const auto fof_and = fof->mapped_dynamic_cast<const And>()) {
        return build_and_prover(this->tb, this->convert_prover(fof_and->get_left()), this->convert_prover(fof_and->get_right()));
    } else if (const auto fof_or = fof->mapped_dynamic_cast<const Or>()) {
        return build_or_prover(this->tb, this->convert_prover(fof_or->get_left()), this->convert_prover(fof_or->get_right()));
    } else if (const auto fof_implies = fof->mapped_dynamic_cast<const Implies>()) {
        return build_implies_prover(this->tb, this->convert_prover(fof_implies->get_left()), this->convert_prover(fof_implies->get_right()));
    } else if (const auto fof_exists = fof->mapped_dynamic_cast<const Exists>()) {
        return build_exists_prover(this->tb, this->convert_prover(fof_exists->get_var()), this->convert_prover(fof_exists->get_arg()));
    } else if (const auto fof_forall = fof->mapped_dynamic_cast<const Forall>()) {
        return build_forall_prover(this->tb, this->convert_prover(fof_forall->get_var()), this->convert_prover(fof_forall->get_arg()));
    } else if (const auto fof_equal = fof->mapped_dynamic_cast<const Equal>()) {
        return build_equal_prover(this->tb, this->convert_prover(fof_equal->get_left(), true), this->convert_prover(fof_equal->get_right(), true));
    } else if (const auto fof_pred = fof->mapped_dynamic_cast<const Predicate>()) {
        return this->convert_predicate_prover(fof_pred);
    } else {
        gio_should_not_arrive_here_ctx(boost::typeindex::type_id_runtime(*fof).pretty_name());
    }
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::convert_predicate_prover(const std::shared_ptr<const Predicate> &pred, size_t start_idx) const {
    using namespace gio::mmpp::setmm;
    const auto &data = this->preds.at(pred->get_name());
    if (start_idx == pred->get_args().size()) {
        return std::get<1>(data);
    } else {
        auto prover = this->convert_predicate_prover(pred, start_idx+1);
        const auto &arg = pred->get_args().at(start_idx);
        prover = build_subst_prover(this->tb, build_label_prover(this->tb, std::get<0>(data).at(start_idx)), this->convert_prover(arg, true), prover);
        return prover;
    }
}

ParsingTree<SymTok, LabTok> fof_to_mm_ctx::convert(const std::shared_ptr<const FOF> &fof) const {
    return pt2_to_pt(prover_to_pt2(this->tb, this->convert_prover(fof)));
}

const RegisteredProver is_set_trp = LibraryToolbox::register_prover({}, "wff A e. _V");
const RegisteredProver set_set_rp = LibraryToolbox::register_prover({}, "|- x e. _V");
const RegisteredProver set_subst_rp = LibraryToolbox::register_prover({"|- B e. _V"}, "|- [_ A / x ]_ B e. _V");

Prover<CheckpointedProofEngine> fof_to_mm_ctx::sethood_prover(const std::shared_ptr<const FOT> &fot) const {
    Prover<CheckpointedProofEngine> prover;
    if (const auto fot_var = fot->mapped_dynamic_cast<const Variable>()) {
        prover = tb.build_registered_prover(set_set_rp, {{"x", this->convert_prover(fot_var, false)}}, {});
    } else if (const auto fot_funct = fot->mapped_dynamic_cast<const Functor>()) {
        const auto &data = this->functs.at(fot_funct->get_name());
        prover = std::get<2>(data);
        size_t i = std::get<0>(data).size() - 1;
        for (const auto &var : boost::adaptors::reverse(std::get<0>(data))) {
            prover = this->tb.build_registered_prover(set_subst_rp, {{"A", this->convert_prover(fot_funct->get_args().at(i), true)}, {"x", trivial_prover(var)}, {"B", this->convert_functor_prover(fot_funct, i+1)}}, {prover});
            i--;
        }
    } else {
        throw std::runtime_error(gio_make_string("invalid type in sethood_prover: " << boost::typeindex::type_id_runtime(*fot).pretty_name()));
    }
    auto sent = prover_to_pt2(this->tb, this->tb.build_registered_prover(is_set_trp, {{"A", this->convert_prover(fot, true)}}, {}));
    prover = prover_checker<InspectableProofEngine<ParsingTree2<SymTok, LabTok>>, CheckpointedProofEngine>(this->tb, prover, std::make_pair(this->tb.get_turnstile(), sent));
    return prover;
}

const RegisteredProver is_nf_trp = LibraryToolbox::register_prover({}, "wff F/ x ph");
const RegisteredProver is_nf_class_trp = LibraryToolbox::register_prover({}, "wff F/_ x A");
const RegisteredProver nf_true_rp = LibraryToolbox::register_prover({}, "|- F/ x T.");
const RegisteredProver nf_false_rp = LibraryToolbox::register_prover({}, "|- F/ x F.");
const RegisteredProver nf_not_rp = LibraryToolbox::register_prover({"|- F/ x ph"}, "|- F/ x -. ph");
const RegisteredProver nf_and_rp = LibraryToolbox::register_prover({"|- F/ x ph", "|- F/ x ps"}, "|- F/ x ( ph /\\ ps )");
const RegisteredProver nf_or_rp = LibraryToolbox::register_prover({"|- F/ x ph", "|- F/ x ps"}, "|- F/ x ( ph \\/ ps )");
const RegisteredProver nf_implies_rp = LibraryToolbox::register_prover({"|- F/ x ph", "|- F/ x ps"}, "|- F/ x ( ph -> ps )");
const RegisteredProver nf_equals_rp = LibraryToolbox::register_prover({"|- F/_ x A", "|- F/_ x B"}, "|- F/ x A = B");
const RegisteredProver nf_forall_rp = LibraryToolbox::register_prover({}, "|- F/ x A. x ph");
const RegisteredProver nf_forall_dist_rp = LibraryToolbox::register_prover({"|- F/ x ph"}, "|- F/ x A. y ph");
const RegisteredProver nf_exists_rp = LibraryToolbox::register_prover({}, "|- F/ x E. x ph");
const RegisteredProver nf_exists_dist_rp = LibraryToolbox::register_prover({"|- F/ x ph"}, "|- F/ x E. y ph");
const RegisteredProver nf_subst_dists_rp = LibraryToolbox::register_prover({"|- F/ x ph", "|- F/_ x A"}, "|- F/ x [. A / y ]. ph");
const RegisteredProver nf_subst_dists_class_rp = LibraryToolbox::register_prover({"|- F/_ x B", "|- F/_ x A"}, "|- F/_ x [_ A / y ]_ B");
const RegisteredProver nf_set_rp = LibraryToolbox::register_prover({}, "|- F/_ x y");

Prover<CheckpointedProofEngine> fof_to_mm_ctx::not_free_prover(const std::shared_ptr<const FOT> &fot, const std::string &var_name) const {
    using namespace gio::mmpp::setmm;
    const auto var = Variable::create(var_name);
    Prover<CheckpointedProofEngine> prover;
    if (const auto fot_var = fot->mapped_dynamic_cast<const Variable>()) {
        if (eq_cmp(fot_cmp())(*var, *fot_var)) {
            throw std::runtime_error("variable is free in itself");
        }
        prover = tb.build_registered_prover(nf_set_rp, {{"x", this->convert_prover(var, false)}, {"y", this->convert_prover(fot_var, false)}}, {});
    } else if (const auto fot_func = fot->mapped_dynamic_cast<const Functor>()) {
        prover = this->not_free_functor_prover(fot_func, var_name);
    } else {
        throw std::runtime_error(gio_make_string("invalid type in not_free_prover: " << boost::typeindex::type_id_runtime(*fot).pretty_name()));
    }
    auto sent = prover_to_pt2(this->tb, this->tb.build_registered_prover(is_nf_class_trp, {{"x", this->convert_prover(var, false)}, {"A", this->convert_prover(fot, true)}}, {}));
    prover = prover_checker<InspectableProofEngine<ParsingTree2<SymTok, LabTok>>, CheckpointedProofEngine>(this->tb, prover, std::make_pair(this->tb.get_turnstile(), sent));
    return prover;
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::not_free_functor_prover(const std::shared_ptr<const Functor> &func, const std::string &var_name, size_t start_idx) const {
    using namespace gio::mmpp::setmm;
    const auto &data = this->functs.at(func->get_name());
    const auto var = Variable::create(var_name);
    if (start_idx == func->get_args().size()) {
        return std::get<3>(data)(this->vars.at(var_name));
    } else {
        auto prover = this->not_free_functor_prover(func, var_name, start_idx+1);
        const auto &arg = func->get_args().at(start_idx);
        prover = this->tb.build_registered_prover(nf_subst_dists_class_rp, {{"x", this->convert_prover(var, false)}, {"A", this->convert_prover(arg, true)},
                                                                            {"y", build_label_prover(this->tb, std::get<0>(data).at(start_idx))}, {"B", this->convert_functor_prover(func, start_idx+1)}},
                                                  {prover, this->not_free_prover(arg, var_name)});
        return prover;
    }
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::internal_not_free_prover(const std::shared_ptr<const FOT> &fot, LabTok var_lab) const {
    Prover<CheckpointedProofEngine> prover;
    if (const auto fot_var = fot->mapped_dynamic_cast<const Variable>()) {
        if (var_lab == this->vars.at(fot_var->get_name())) {
            throw std::runtime_error("variable is free in itself");
        }
        prover = tb.build_registered_prover(nf_set_rp, {{"x", trivial_prover(var_lab)}, {"y", this->convert_prover(fot_var, false)}}, {});
    } else if (const auto fot_func = fot->mapped_dynamic_cast<const Functor>()) {
        prover = this->internal_not_free_functor_prover(fot_func, var_lab);
    } else {
        throw std::runtime_error(gio_make_string("invalid type in internal_not_free_prover: " << boost::typeindex::type_id_runtime(*fot).pretty_name()));
    }
    auto sent = prover_to_pt2(this->tb, this->tb.build_registered_prover(is_nf_class_trp, {{"x", trivial_prover(var_lab)}, {"A", this->convert_prover(fot, true)}}, {}));
    prover = prover_checker<InspectableProofEngine<ParsingTree2<SymTok, LabTok>>, CheckpointedProofEngine>(this->tb, prover, std::make_pair(this->tb.get_turnstile(), sent));
    return prover;
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::internal_not_free_functor_prover(const std::shared_ptr<const Functor> &func, LabTok var_lab, size_t start_idx) const {
    using namespace gio::mmpp::setmm;
    const auto &data = this->functs.at(func->get_name());
    if (start_idx == func->get_args().size()) {
        return std::get<3>(data)(var_lab);
    } else {
        auto prover = this->internal_not_free_functor_prover(func, var_lab, start_idx+1);
        const auto &arg = func->get_args().at(start_idx);
        prover = this->tb.build_registered_prover(nf_subst_dists_class_rp, {{"x", trivial_prover(var_lab)}, {"A", this->convert_prover(arg, true)},
                                                                            {"y", build_label_prover(this->tb, std::get<0>(data).at(start_idx))}, {"B", this->convert_functor_prover(func, start_idx+1)}},
                                                  {prover, this->internal_not_free_prover(arg, var_lab)});
        return prover;
    }
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::not_free_prover(const std::shared_ptr<const FOF> &fof, const std::string &var_name) const {
    using namespace gio::mmpp::setmm;
    const auto var = Variable::create(var_name);
    Prover<CheckpointedProofEngine> prover;
    if (const auto fof_true = fof->mapped_dynamic_cast<const True>()) {
        prover = tb.build_registered_prover(nf_true_rp, {{"x", this->convert_prover(var, false)}}, {});
    } else if (const auto fof_false = fof->mapped_dynamic_cast<const False>()) {
        prover = tb.build_registered_prover(nf_false_rp, {{"x", this->convert_prover(var, false)}}, {});
    } else if (const auto fof_not = fof->mapped_dynamic_cast<const Not>()) {
        prover = tb.build_registered_prover(nf_not_rp, {{"x", this->convert_prover(var, false)}, {"ph", this->convert_prover(fof_not->get_arg())}}, {this->not_free_prover(fof_not->get_arg(), var_name)});
    } else if (const auto fof_and = fof->mapped_dynamic_cast<const And>()) {
        prover = tb.build_registered_prover(nf_and_rp, {{"x", this->convert_prover(var, false)}, {"ph", this->convert_prover(fof_and->get_left())}, {"ps", this->convert_prover(fof_and->get_right())}},
                                            {this->not_free_prover(fof_and->get_left(), var_name), this->not_free_prover(fof_and->get_right(), var_name)});
    } else if (const auto fof_or = fof->mapped_dynamic_cast<const Or>()) {
        prover = tb.build_registered_prover(nf_or_rp, {{"x", this->convert_prover(var, false)}, {"ph", this->convert_prover(fof_or->get_left())}, {"ps", this->convert_prover(fof_or->get_right())}},
                                            {this->not_free_prover(fof_or->get_left(), var_name), this->not_free_prover(fof_or->get_right(), var_name)});
    } else if (const auto fof_implies = fof->mapped_dynamic_cast<const Implies>()) {
        prover = tb.build_registered_prover(nf_implies_rp, {{"x", this->convert_prover(var, false)}, {"ph", this->convert_prover(fof_implies->get_left())}, {"ps", this->convert_prover(fof_implies->get_right())}},
                                            {this->not_free_prover(fof_implies->get_left(), var_name), this->not_free_prover(fof_implies->get_right(), var_name)});
    } else if (const auto fof_exists = fof->mapped_dynamic_cast<const Exists>()) {
        if (eq_cmp(fot_cmp())(*var, *fof_exists->get_var())) {
            prover = tb.build_registered_prover(nf_exists_rp, {{"x", this->convert_prover(var, false)}, {"ph", this->convert_prover(fof_exists->get_arg())}}, {});
        } else {
            prover = tb.build_registered_prover(nf_exists_dist_rp, {{"x", this->convert_prover(var, false)}, {"y", this->convert_prover(fof_exists->get_var(), false)}, {"ph", this->convert_prover(fof_exists->get_arg())}},
                                                {this->not_free_prover(fof_exists->get_arg(), var_name)});
        }
    } else if (const auto fof_forall = fof->mapped_dynamic_cast<const Forall>()) {
        if (eq_cmp(fot_cmp())(*var, *fof_forall->get_var())) {
            prover = tb.build_registered_prover(nf_forall_rp, {{"x", this->convert_prover(var, false)}, {"ph", this->convert_prover(fof_forall->get_arg())}}, {});
        } else {
            prover = tb.build_registered_prover(nf_forall_dist_rp, {{"x", this->convert_prover(var, false)}, {"y", this->convert_prover(fof_forall->get_var(), false)}, {"ph", this->convert_prover(fof_forall->get_arg())}},
                                                {this->not_free_prover(fof_forall->get_arg(), var_name)});
        }
    } else if (const auto fof_equal = fof->mapped_dynamic_cast<const Equal>()) {
        prover = tb.build_registered_prover(nf_equals_rp, {{"x", this->convert_prover(var, false)}, {"A", this->convert_prover(fof_equal->get_left(), true)}, {"B", this->convert_prover(fof_equal->get_right(), true)}},
                                            {this->not_free_prover(fof_equal->get_left(), var_name), this->not_free_prover(fof_equal->get_right(), var_name)});
    } else if (const auto fof_pred = fof->mapped_dynamic_cast<const Predicate>()) {
        prover = this->not_free_predicate_prover(fof_pred, var_name);
    } else {
        throw std::runtime_error(gio_make_string("invalid type in not_free_prover: " << boost::typeindex::type_id_runtime(*fof).pretty_name()));
    }
    auto sent = prover_to_pt2(this->tb, this->tb.build_registered_prover(is_nf_trp, {{"x", this->convert_prover(var, false)}, {"ph", this->convert_prover(fof)}}, {}));
    prover = prover_checker<InspectableProofEngine<ParsingTree2<SymTok, LabTok>>, CheckpointedProofEngine>(this->tb, prover, std::make_pair(this->tb.get_turnstile(), sent));
    return prover;
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::not_free_predicate_prover(const std::shared_ptr<const Predicate> &pred, const std::string &var_name, size_t start_idx) const {
    using namespace gio::mmpp::setmm;
    const auto &data = this->preds.at(pred->get_name());
    const auto var = Variable::create(var_name);
    if (start_idx == pred->get_args().size()) {
        return std::get<2>(data)(this->vars.at(var_name));
    } else {
        auto prover = this->not_free_predicate_prover(pred, var_name, start_idx+1);
        const auto &arg = pred->get_args().at(start_idx);
        prover = this->tb.build_registered_prover(nf_subst_dists_rp, {{"x", this->convert_prover(var, false)}, {"A", this->convert_prover(arg, true)},
                                                                      {"y", build_label_prover(this->tb, std::get<0>(data).at(start_idx))}, {"ph", this->convert_predicate_prover(pred, start_idx+1)}},
                                                  {prover, this->not_free_prover(arg, var_name)});
        return prover;
    }
}

const RegisteredProver is_repl_trp = LibraryToolbox::register_prover({}, "wff ( [. A / x ]. ph <-> ps )");
const RegisteredProver is_repl_class_trp = LibraryToolbox::register_prover({}, "wff [_ A / x ]_ B = C");
const RegisteredProver repl_true_rp = LibraryToolbox::register_prover({"|- A e. _V"}, "|- ( [. A / x ]. T. <-> T. )");
const RegisteredProver repl_false_rp = LibraryToolbox::register_prover({"|- A e. _V"}, "|- ( [. A / x ]. F. <-> F. )");
const RegisteredProver repl_not_rp = LibraryToolbox::register_prover({"|- A e. _V", "|- ( [. A / x ]. ph <-> ps )"}, "|- ( [. A / x ]. -. ph <-> -. ps )");
const RegisteredProver repl_and_rp = LibraryToolbox::register_prover({"|- ( [. A / x ]. ph <-> ch )", "|- ( [. A / x ]. ps <-> et )"}, "|- ( [. A / x ]. ( ph /\\ ps ) <-> ( ch /\\ et ) )");
const RegisteredProver repl_or_rp = LibraryToolbox::register_prover({"|- ( [. A / x ]. ph <-> ch )", "|- ( [. A / x ]. ps <-> et )"}, "|- ( [. A / x ]. ( ph \\/ ps ) <-> ( ch \\/ et ) )");
const RegisteredProver repl_imp_rp = LibraryToolbox::register_prover({"|- A e. _V", "|- ( [. A / x ]. ph <-> ch )", "|- ( [. A / x ]. ps <-> et )"}, "|- ( [. A / x ]. ( ph -> ps ) <-> ( ch -> et ) )");
const RegisteredProver repl_equals_rp = LibraryToolbox::register_prover({"|- A e. _V", "|- [_ A / x ]_ B = D", "|- [_ A / x ]_ C = E"}, "|- ( [. A / x ]. B = C <-> D = E )");
const RegisteredProver repl_forall_rp = LibraryToolbox::register_prover({"|- A e. _V"}, "|- ( [. A / x ]. A. x ph <-> A. x ph )");
const RegisteredProver repl_forall_dists_rp = LibraryToolbox::register_prover({"|- F/_ y A", "|- ( [. A / x ]. ph <-> ps )"}, "|- ( [. A / x ]. A. y ph <-> A. y ps )");
const RegisteredProver repl_exists_rp = LibraryToolbox::register_prover({"|- A e. _V"}, "|- ( [. A / x ]. E. x ph <-> E. x ph )");
const RegisteredProver repl_exists_dists_rp = LibraryToolbox::register_prover({"|- F/_ y A", "|- ( [. A / x ]. ph <-> ps )"}, "|- ( [. A / x ]. E. y ph <-> E. y ps )");
const RegisteredProver repl_set_rp = LibraryToolbox::register_prover({"|- A e. _V"}, "|- [_ A / x ]_ x = A");
const RegisteredProver repl_set_dists_rp = LibraryToolbox::register_prover({"|- A e. _V"}, "|- [_ A / x ]_ y = y");

Prover<CheckpointedProofEngine> fof_to_mm_ctx::replace_prover(const std::shared_ptr<const FOT> &fot, const std::string &var_name, const std::shared_ptr<const FOT> &term) const {
    using namespace gio::mmpp::setmm;
    const auto var = Variable::create(var_name);
    Prover<CheckpointedProofEngine> prover;
    if (const auto fot_var = fot->mapped_dynamic_cast<const Variable>()) {
        if (eq_cmp(fot_cmp())(*var, *fot_var)) {
            prover = tb.build_registered_prover(repl_set_rp, {{"x", this->convert_prover(var, false)}, {"A", this->convert_prover(term, true)}}, {this->sethood_prover(term)});
        } else {
            prover = tb.build_registered_prover(repl_set_dists_rp, {{"x", this->convert_prover(var, false)}, {"A", this->convert_prover(term, true)}, {"y", this->convert_prover(fot_var, false)}}, {this->sethood_prover(term)});
        }
    } else if (const auto fot_func = fot->mapped_dynamic_cast<const Functor>()) {
        prover = this->replace_functor_prover(fot_func, var_name, term);
    } else {
        throw std::runtime_error(gio_make_string("invalid type in replace_prover: " << boost::typeindex::type_id_runtime(*fot).pretty_name()));
    }
    auto sent = prover_to_pt2(this->tb, this->tb.build_registered_prover(is_repl_class_trp, {{"x", this->convert_prover(var, false)}, {"A", this->convert_prover(term, true)},
                                                                                             {"B", this->convert_prover(fot, true)}, {"C", this->convert_prover(fot->replace(var_name, term), true)}}, {}));
    prover = prover_checker<InspectableProofEngine<ParsingTree2<SymTok, LabTok>>, CheckpointedProofEngine>(this->tb, prover, std::make_pair(this->tb.get_turnstile(), sent));
    return prover;
}

const RegisteredProver repl_subst_dist_class_rp = LibraryToolbox::register_prover({"|- A e. _V", "|- F/_ y A", "|- [_ A / x ]_ B = C", "|- [_ A / x ]_ D = E"}, "|- [_ A / x ]_ [_ B / y ]_ D = [_ C / y ]_ E");
const RegisteredProver repl_subst_final_class_rp = LibraryToolbox::register_prover({"|- A e. _V", "|- F/_ x B"}, "|- [_ A / x ]_ B = B");

Prover<CheckpointedProofEngine> fof_to_mm_ctx::replace_functor_prover(const std::shared_ptr<const Functor> &func, const std::string &var_name, const std::shared_ptr<const FOT> &term, size_t start_idx) const {
    using namespace gio::mmpp::setmm;
    const auto &data = this->functs.at(func->get_name());
    const auto var = Variable::create(var_name);
    if (start_idx == func->get_args().size()) {
        return this->tb.build_registered_prover(repl_subst_final_class_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)}, {"B", this->convert_functor_prover(func, start_idx)}},
                                                {this->sethood_prover(term), this->not_free_functor_prover(func, var_name, start_idx)});
    } else {
        auto prover = this->replace_functor_prover(func, var_name, term, start_idx+1);
        const auto &arg = func->get_args().at(start_idx);
        prover = this->tb.build_registered_prover(repl_subst_dist_class_rp, {{"x", this->convert_prover(var, false)}, {"A", this->convert_prover(term, true)},
                                                                             {"B", this->convert_prover(arg, true)}, {"C", this->convert_prover(arg->replace(var_name, term), true)},
                                                                             {"y", build_label_prover(this->tb, std::get<0>(data).at(start_idx))}, {"D", this->convert_functor_prover(func, start_idx+1)},
                                                                             {"E", this->convert_functor_prover(func->replace(var_name, term)->safe_mapped_dynamic_cast<const Functor>(), start_idx+1)}},
                                                  {this->sethood_prover(term), this->internal_not_free_prover(term, std::get<0>(data).at(start_idx)), this->replace_prover(arg, var_name, term), prover});
        return prover;
    }
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::replace_prover(const std::shared_ptr<const FOF> &fof, const std::string &var_name, const std::shared_ptr<const FOT> &term) const {
    using namespace gio::mmpp::setmm;
    const auto var = Variable::create(var_name);
    Prover<CheckpointedProofEngine> prover;
    if (const auto fof_true = fof->mapped_dynamic_cast<const True>()) {
        prover = tb.build_registered_prover(repl_true_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)}}, {this->sethood_prover(term)});
    } else if (const auto fof_false = fof->mapped_dynamic_cast<const False>()) {
        prover = tb.build_registered_prover(repl_false_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)}}, {this->sethood_prover(term)});
    } else if (const auto fof_not = fof->mapped_dynamic_cast<const Not>()) {
        prover = tb.build_registered_prover(repl_not_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)},
                                                          {"ph", this->convert_prover(fof_not->get_arg())}, {"ps", this->convert_prover(fof_not->get_arg()->replace(var_name, term))}},
                                            {this->sethood_prover(term), this->replace_prover(fof_not->get_arg(), var_name, term)});
    } else if (const auto fof_and = fof->mapped_dynamic_cast<const And>()) {
        prover = tb.build_registered_prover(repl_and_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)},
                                                          {"ph", this->convert_prover(fof_and->get_left())}, {"ch", this->convert_prover(fof_and->get_left()->replace(var_name, term))},
                                                          {"ps", this->convert_prover(fof_and->get_right())}, {"et", this->convert_prover(fof_and->get_right()->replace(var_name, term))}},
                                            {this->replace_prover(fof_and->get_left(), var_name, term), this->replace_prover(fof_and->get_right(), var_name, term)});
    } else if (const auto fof_or = fof->mapped_dynamic_cast<const Or>()) {
        prover = tb.build_registered_prover(repl_or_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)},
                                                         {"ph", this->convert_prover(fof_or->get_left())}, {"ch", this->convert_prover(fof_or->get_left()->replace(var_name, term))},
                                                         {"ps", this->convert_prover(fof_or->get_right())}, {"et", this->convert_prover(fof_or->get_right()->replace(var_name, term))}},
                                            {this->replace_prover(fof_or->get_left(), var_name, term), this->replace_prover(fof_or->get_right(), var_name, term)});
    } else if (const auto fof_implies = fof->mapped_dynamic_cast<const Implies>()) {
        prover = tb.build_registered_prover(repl_imp_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)},
                                                          {"ph", this->convert_prover(fof_implies->get_left())}, {"ch", this->convert_prover(fof_implies->get_left()->replace(var_name, term))},
                                                          {"ps", this->convert_prover(fof_implies->get_right())}, {"et", this->convert_prover(fof_implies->get_right()->replace(var_name, term))}},
                                            {this->sethood_prover(term), this->replace_prover(fof_implies->get_left(), var_name, term), this->replace_prover(fof_implies->get_right(), var_name, term)});
    } else if (const auto fof_equals = fof->mapped_dynamic_cast<const Equal>()) {
        prover = tb.build_registered_prover(repl_equals_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)},
                                                             {"B", this->convert_prover(fof_equals->get_left(), true)}, {"D", this->convert_prover(fof_equals->get_left()->replace(var_name, term), true)},
                                                             {"C", this->convert_prover(fof_equals->get_right(), true)}, {"E", this->convert_prover(fof_equals->get_right()->replace(var_name, term), true)}},
                                            {this->sethood_prover(term), this->replace_prover(fof_equals->get_left(), var_name, term), this->replace_prover(fof_equals->get_right(), var_name, term)});
    } else if (const auto fof_exists = fof->mapped_dynamic_cast<const Exists>()) {
        if (eq_cmp(fot_cmp())(*var, *fof_exists->get_var())) {
            prover = tb.build_registered_prover(repl_exists_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)},
                                                                 {"ph", this->convert_prover(fof_exists->get_arg())}},
                                                {this->sethood_prover(term)});
        } else {
            prover = tb.build_registered_prover(repl_exists_dists_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)}, {"y", this->convert_prover(fof_exists->get_var(), false)},
                                                                       {"ph", this->convert_prover(fof_exists->get_arg())}, {"ps", this->convert_prover(fof_exists->get_arg()->replace(var_name, term))}},
                                                      {this->not_free_prover(term, fof_exists->get_var()->get_name()), this->replace_prover(fof_exists->get_arg(), var_name, term)});
        }
    } else if (const auto fof_forall = fof->mapped_dynamic_cast<const Forall>()) {
        if (eq_cmp(fot_cmp())(*var, *fof_forall->get_var())) {
            prover = tb.build_registered_prover(repl_forall_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)},
                                                                 {"ph", this->convert_prover(fof_forall->get_arg())}},
                                                {this->sethood_prover(term)});
        } else {
            prover = tb.build_registered_prover(repl_forall_dists_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)}, {"y", this->convert_prover(fof_forall->get_var(), false)},
                                                                       {"ph", this->convert_prover(fof_forall->get_arg())}, {"ps", this->convert_prover(fof_forall->get_arg()->replace(var_name, term))}},
                                                      {this->not_free_prover(term, fof_forall->get_var()->get_name()), this->replace_prover(fof_forall->get_arg(), var_name, term)});
        }
    } else if (const auto fof_pred = fof->mapped_dynamic_cast<const Predicate>()) {
        prover = this->replace_predicate_prover(fof_pred, var_name, term);
    } else {
        throw std::runtime_error(gio_make_string("invalid type in replace_prover: " << boost::typeindex::type_id_runtime(*fof).pretty_name()));
    }
    auto sent = prover_to_pt2(this->tb, this->tb.build_registered_prover(is_repl_trp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)},
                                                                                       {"ph", this->convert_prover(fof)}, {"ps", this->convert_prover(fof->replace(var_name, term))}}, {}));
    prover = prover_checker<InspectableProofEngine<ParsingTree2<SymTok, LabTok>>, CheckpointedProofEngine>(this->tb, prover, std::make_pair(this->tb.get_turnstile(), sent));
    return prover;
}

const RegisteredProver repl_subst_dist_rp = LibraryToolbox::register_prover({"|- A e. _V", "|- F/_ y A", "|- [_ A / x ]_ B = C", "|- ( [. A / x ]. ph <-> ps )"}, "|- ( [. A / x ]. [. B / y ]. ph <-> [. C / y ]. ps )");
const RegisteredProver repl_subst_final_rp = LibraryToolbox::register_prover({"|- A e. _V", "|- F/ x ph"}, "|- ( [. A / x ]. ph <-> ph )");

Prover<CheckpointedProofEngine> fof_to_mm_ctx::replace_predicate_prover(const std::shared_ptr<const Predicate> &pred, const std::string &var_name, const std::shared_ptr<const FOT> &term, size_t start_idx) const {
    using namespace gio::mmpp::setmm;
    const auto &data = this->preds.at(pred->get_name());
    const auto var = Variable::create(var_name);
    if (start_idx == pred->get_args().size()) {
        return this->tb.build_registered_prover(repl_subst_final_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)}, {"ph", this->convert_predicate_prover(pred, start_idx)}},
                                                {this->sethood_prover(term), this->not_free_predicate_prover(pred, var_name, start_idx)});
    } else {
        auto prover = this->replace_predicate_prover(pred, var_name, term, start_idx+1);
        const auto &arg = pred->get_args().at(start_idx);
        prover = this->tb.build_registered_prover(repl_subst_dist_rp, {{"x", this->convert_prover(var, false)}, {"A", this->convert_prover(term, true)},
                                                                       {"B", this->convert_prover(arg, true)}, {"C", this->convert_prover(arg->replace(var_name, term), true)},
                                                                       {"y", build_label_prover(this->tb, std::get<0>(data).at(start_idx))}, {"ph", this->convert_predicate_prover(pred, start_idx+1)},
                                                                       {"ps", this->convert_predicate_prover(pred->replace(var_name, term)->safe_mapped_dynamic_cast<const Predicate>(), start_idx+1)}},
                                                  {this->sethood_prover(term), this->internal_not_free_prover(term, std::get<0>(data).at(start_idx)), this->replace_prover(arg, var_name, term), prover});
        return prover;
    }
}

}
