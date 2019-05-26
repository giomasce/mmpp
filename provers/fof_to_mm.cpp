
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
        //gio::assert_or_throw<std::runtime_error>(res, "prover failed");
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
        const auto &pred = this->preds.at(fof_pred->get_name());
        auto prover = build_label_prover(this->tb, pred.first);
        size_t i = 0;
        for (const auto &arg : boost::adaptors::reverse(fof_pred->get_args())) {
            prover = build_subst_prover(this->tb, build_label_prover(this->tb, pred.second.at(i)), this->convert_prover(arg, true), prover);
            i++;
        }
        return prover;
    } else {
        gio_should_not_arrive_here_ctx(boost::typeindex::type_id_runtime(*fof).pretty_name());
    }
}

Prover<CheckpointedProofEngine> fof_to_mm_ctx::convert_prover(const std::shared_ptr<const FOT> &fot, bool make_class) const {
    using namespace gio::mmpp::setmm;
    if (const auto fot_funct = fot->mapped_dynamic_cast<const Functor>()) {
        const auto &funct = this->functs.at(fot_funct->get_name());
        auto prover = build_label_prover(this->tb, funct.first);
        size_t i = 0;
        for (const auto &arg : boost::adaptors::reverse(fot_funct->get_args())) {
            prover = build_class_subst_prover(this->tb, build_label_prover(this->tb, funct.second.at(i)), this->convert_prover(arg, true), prover);
            i++;
        }
        return prover;
    } else if (const auto fot_var = fot->mapped_dynamic_cast<const Variable>()) {
        auto prover = build_label_prover(this->tb, this->vars.at(fot_var->get_name()));
        if (make_class) {
            prover = build_var_adaptor_prover(this->tb, prover);
        }
        return prover;
    } else {
        gio_should_not_arrive_here_ctx(boost::typeindex::type_id_runtime(*fot).pretty_name());
    }
}

ParsingTree<SymTok, LabTok> fof_to_mm_ctx::convert(const std::shared_ptr<const FOF> &fof) const {
    return pt2_to_pt(prover_to_pt2(this->tb, this->convert_prover(fof)));
}

const RegisteredProver is_set_trp = LibraryToolbox::register_prover({}, "wff A e. _V");
const RegisteredProver set_set_rp = LibraryToolbox::register_prover({}, "|- x e. _V");

Prover<CheckpointedProofEngine> fof_to_mm_ctx::sethood_prover(const std::shared_ptr<const FOT> &fot) const {
    Prover<CheckpointedProofEngine> prover;
    if (const auto fot_var = fot->mapped_dynamic_cast<const Variable>()) {
        prover = tb.build_registered_prover(set_set_rp, {{"x", this->convert_prover(fot_var, false)}}, {});
    } else {
        gio_should_not_arrive_here_ctx(boost::typeindex::type_id_runtime(*fot).pretty_name());
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
const RegisteredProver nf_subst_rp = LibraryToolbox::register_prover({"|- F/ x ph", "|- F/_ x A"}, "|- F/ x [. A / y ]. ph");
const RegisteredProver nf_subst_class_rp = LibraryToolbox::register_prover({"|- F/_ x B", "|- F/_ x A"}, "|- F/_ x [_ A / y ]_ B");
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
    } else {
        gio_should_not_arrive_here_ctx(boost::typeindex::type_id_runtime(*fot).pretty_name());
    }
    auto sent = prover_to_pt2(this->tb, this->tb.build_registered_prover(is_nf_class_trp, {{"x", this->convert_prover(var, false)}, {"A", this->convert_prover(fot, true)}}, {}));
    prover = prover_checker<InspectableProofEngine<ParsingTree2<SymTok, LabTok>>, CheckpointedProofEngine>(this->tb, prover, std::make_pair(this->tb.get_turnstile(), sent));
    return prover;
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
            prover = tb.build_registered_prover(nf_exists_rp, {{"x", this->convert_prover(var, false)}, {"ph", this->convert_prover(fof_forall->get_arg())}}, {});
        } else {
            prover = tb.build_registered_prover(nf_exists_dist_rp, {{"x", this->convert_prover(var, false)}, {"y", this->convert_prover(fof_forall->get_var(), false)}, {"ph", this->convert_prover(fof_forall->get_arg())}},
                                                {this->not_free_prover(fof_forall->get_arg(), var_name)});
        }
    } else if (const auto fof_equal = fof->mapped_dynamic_cast<const Equal>()) {
        prover = tb.build_registered_prover(nf_equals_rp, {{"x", this->convert_prover(var, false)}, {"A", this->convert_prover(fof_equal->get_left())}, {"B", this->convert_prover(fof_equal->get_right())}},
                                            {this->not_free_prover(fof_equal->get_left(), var_name), this->not_free_prover(fof_equal->get_right(), var_name)});
    } else {
        gio_should_not_arrive_here_ctx(boost::typeindex::type_id_runtime(*fof).pretty_name());
    }
    auto sent = prover_to_pt2(this->tb, this->tb.build_registered_prover(is_nf_trp, {{"x", this->convert_prover(var, false)}, {"ph", this->convert_prover(fof)}}, {}));
    prover = prover_checker<InspectableProofEngine<ParsingTree2<SymTok, LabTok>>, CheckpointedProofEngine>(this->tb, prover, std::make_pair(this->tb.get_turnstile(), sent));
    return prover;
}

const RegisteredProver is_repl_trp = LibraryToolbox::register_prover({}, "wff ( [. A / x ]. ph <-> ps )");
const RegisteredProver is_repl_class_trp = LibraryToolbox::register_prover({}, "wff [_ A / x ]_ B = C");
const RegisteredProver repl_true_rp = LibraryToolbox::register_prover({"|- A e. _V"}, "|- ( [. A / x ]. T. <-> T. )");
const RegisteredProver repl_false_rp = LibraryToolbox::register_prover({"|- A e. _V"}, "|- ( [. A / x ]. F. <-> F. )");

Prover<CheckpointedProofEngine> fof_to_mm_ctx::replace_prover(const std::shared_ptr<const FOF> &fof, const std::string &var_name, const std::shared_ptr<const FOT> &term) const {
    using namespace gio::mmpp::setmm;
    const auto var = Variable::create(var_name);
    Prover<CheckpointedProofEngine> prover;
    if (const auto fof_true = fof->mapped_dynamic_cast<const True>()) {
        prover = tb.build_registered_prover(repl_true_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)}}, {this->sethood_prover(term)});
    } else if (const auto fof_false = fof->mapped_dynamic_cast<const False>()) {
        prover = tb.build_registered_prover(repl_false_rp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)}}, {this->sethood_prover(term)});
    } else {
        gio_should_not_arrive_here_ctx(boost::typeindex::type_id_runtime(*fof).pretty_name());
    }
    auto sent = prover_to_pt2(this->tb, this->tb.build_registered_prover(is_repl_trp, {{"A", this->convert_prover(term, true)}, {"x", this->convert_prover(var, false)},
                                                                                       {"ph", this->convert_prover(fof)}, {"ps", this->convert_prover(fof->replace(var_name, term))}}, {}));
    prover = prover_checker<InspectableProofEngine<ParsingTree2<SymTok, LabTok>>, CheckpointedProofEngine>(this->tb, prover, std::make_pair(this->tb.get_turnstile(), sent));
    return prover;
}

}
