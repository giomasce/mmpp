
#include "gapt.h"

#include <type_traits>

#include <giolib/main.h>
#include <giolib/static_block.h>
#include <giolib/utils.h>
#include <giolib/containers.h>
#include <giolib/std_printers.h>

#include "mm/setmm_loader.h"
#include "fof.h"
#include "fof_to_mm.h"
#include "ndproof_to_mm.h"

namespace gio::mmpp::provers::ndproof {

term parse_gapt_term(std::istream &is) {
    using namespace gio::mmpp::provers::fof;
    std::string type;
    is >> type;
    if (type == "var") {
        std::string name;
        is >> name;
        return Variable::create(name);
    } else if (type == "unint") {
        std::string name;
        size_t num;
        is >> name >> num;
        std::vector<term> args;
        for (size_t i = 0; i < num; i++) {
            args.push_back(parse_gapt_term(is));
        }
        return Functor::create(name, args);
    } else {
        throw std::runtime_error("invalid formula type " + type);
    }
}

formula parse_gapt_formula(std::istream &is) {
    using namespace gio::mmpp::provers::fof;
    std::string type;
    is >> type;
    if (type == "exists") {
        auto var = std::dynamic_pointer_cast<const Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::runtime_error>(bool(var), "missing variable after quantifier");
        auto body = parse_gapt_formula(is);
        return Exists::create(var, body);
    } else if (type == "forall") {
        auto var = std::dynamic_pointer_cast<const Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::runtime_error>(bool(var), "missing variable after quantifier");
        auto body = parse_gapt_formula(is);
        return Forall::create(var, body);
    } else if (type == "imp") {
        auto left = parse_gapt_formula(is);
        auto right = parse_gapt_formula(is);
        return Implies::create(left, right);
    } else if (type == "and") {
        auto left = parse_gapt_formula(is);
        auto right = parse_gapt_formula(is);
        return And::create(left, right);
    } else if (type == "or") {
        auto left = parse_gapt_formula(is);
        auto right = parse_gapt_formula(is);
        return Or::create(left, right);
    } else if (type == "not") {
        auto arg = parse_gapt_formula(is);
        return Not::create(arg);
    } else if (type == "equal") {
        auto left = parse_gapt_term(is);
        auto right = parse_gapt_term(is);
        return Equal::create(left, right);
    } else if (type == "false") {
        return False::create();
    } else if (type == "true") {
        return True::create();
    } else if (type == "unint") {
        std::string name;
        size_t num;
        is >> name >> num;
        std::vector<term> args;
        for (size_t i = 0; i < num; i++) {
            args.push_back(parse_gapt_term(is));
        }
        return Predicate::create(name, args);
    } else {
        throw std::runtime_error("invalid formula type " + type);
    }
}

sequent parse_gapt_sequent(std::istream &is) {
    size_t num;
    std::vector<formula> ants;
    std::vector<formula> succs;
    is >> num;
    for (size_t i = 0; i < num; i++) {
        ants.push_back(parse_gapt_formula(is));
    }
    is >> num;
    for (size_t i = 0; i < num; i++) {
        succs.push_back(parse_gapt_formula(is));
    }
    return std::make_pair(ants, succs);
}

ndsequent parse_gapt_ndsequent(std::istream &is) {
    sequent seq = parse_gapt_sequent(is);
    if (seq.second.size() != 1) {
        throw std::invalid_argument("sequent does not have exactly one succedent");
    }
    ndsequent ret;
    ret.first = std::move(seq.first);
    ret.second = std::move(seq.second.front());
    return ret;
}

std::shared_ptr<const NDProof> parse_gapt_proof(std::istream &is) {
    auto thesis = parse_gapt_ndsequent(is);
    std::string type;
    is >> type;
    if (type == "LogicalAxiom") {
        auto form = parse_gapt_formula(is);
        return LogicalAxiom::create(thesis, form);
    } else if (type == "Weakening") {
        auto form = parse_gapt_formula(is);
        auto subproof = parse_gapt_proof(is);
        return WeakeningRule::create(thesis, form, subproof);
    } else if (type == "Contraction") {
        idx_t idx1, idx2;
        is >> idx1 >> idx2;
        auto subproof = parse_gapt_proof(is);
        return ContractionRule::create(thesis, idx1, idx2, subproof);
    } else if (type == "AndIntro") {
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return AndIntroRule::create(thesis, left_proof, right_proof);
    } else if (type == "AndElim1") {
        auto subproof = parse_gapt_proof(is);
        return AndElim1Rule::create(thesis, subproof);
    } else if (type == "AndElim2") {
        auto subproof = parse_gapt_proof(is);
        return AndElim2Rule::create(thesis, subproof);
    } else if (type == "OrIntro1") {
        auto disjunct = parse_gapt_formula(is);
        auto subproof = parse_gapt_proof(is);
        return OrIntro1Rule::create(thesis, disjunct, subproof);
    } else if (type == "OrIntro2") {
        auto disjunct = parse_gapt_formula(is);
        auto subproof = parse_gapt_proof(is);
        return OrIntro2Rule::create(thesis, disjunct, subproof);
    } else if (type == "OrElim") {
        idx_t middle_idx, right_idx;
        is >> middle_idx >> right_idx;
        auto left_proof = parse_gapt_proof(is);
        auto middle_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return OrElimRule::create(thesis, middle_idx, right_idx, left_proof, middle_proof, right_proof);
    } else if (type == "NegElim") {
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return NegElimRule::create(thesis, left_proof, right_proof);
    } else if (type == "ImpIntro") {
        idx_t ant_idx;
        is >> ant_idx;
        auto subproof = parse_gapt_proof(is);
        return ImpIntroRule::create(thesis, ant_idx, subproof);
    } else if (type == "ImpElim") {
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return ImpElimRule::create(thesis, left_proof, right_proof);
    } else if (type == "BottomElim") {
        auto form = parse_gapt_formula(is);
        auto subproof = parse_gapt_proof(is);
        return BottomElimRule::create(thesis, form, subproof);
    } else if (type == "ForallIntro") {
        auto var = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(var), "missing quantified variable");
        auto eigenvar = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(eigenvar), "missing eigenvariable");
        auto subproof = parse_gapt_proof(is);
        return ForallIntroRule::create(thesis, var, eigenvar, subproof);
    } else if (type == "ForallElim") {
        auto term = parse_gapt_term(is);
        auto subproof = parse_gapt_proof(is);
        return ForallElimRule::create(thesis, term, subproof);
    } else if (type == "ExistsIntro") {
        auto form = parse_gapt_formula(is);
        auto var = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(var), "missing substitution variable");
        auto term = parse_gapt_term(is);
        auto subproof = parse_gapt_proof(is);
        return ExistsIntroRule::create(thesis, form, var, term, subproof);
    } else if (type == "ExistsElim") {
        idx_t idx;
        is >> idx;
        auto eigenvar = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(eigenvar), "missing eigenvariable");
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return ExistsElimRule::create(thesis, idx, eigenvar, left_proof, right_proof);
    } else if (type == "EqualityIntro") {
        auto t = parse_gapt_term(is);
        return EqualityIntroRule::create(thesis, t);
    } else if (type == "EqualityElim") {
        auto var = std::dynamic_pointer_cast<const gio::mmpp::provers::fof::Variable>(parse_gapt_term(is));
        gio::assert_or_throw<std::invalid_argument>(bool(var), "missing substitution variable");
        auto form = parse_gapt_formula(is);
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return EqualityElimRule::create(thesis, var, form, left_proof, right_proof);
    } else if (type == "ExcludedMiddle") {
        idx_t left_idx, right_idx;
        is >> left_idx >> right_idx;
        auto left_proof = parse_gapt_proof(is);
        auto right_proof = parse_gapt_proof(is);
        return ExcludedMiddleRule::create(thesis, left_idx, right_idx, left_proof, right_proof);
    } else {
        throw std::runtime_error("invalid proof type " + type);
    }
}

int read_gapt_main(int argc, char *argv[]) {
    using namespace gio;
    using namespace gio::std_printers;
    using namespace gio::mmpp::provers::fof;

    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto proof = parse_gapt_proof(std::cin);
    std::cout << *proof << "\n";
    bool valid = proof->check();
    gio::assert_or_throw<std::runtime_error>(valid, "invalid proof!");

    auto vars_functs_preds = proof->collect_vars_functs_preds();
    std::cout << vars_functs_preds << "\n";

    fof_to_mm_ctx ctx(tb);
    nd_proof_to_mm_ctx ctx2(tb, ctx);
    ctx.alloc_vars(std::get<0>(vars_functs_preds));
    ctx.alloc_vars(std::vector<std::string>{"x", "y", "z"});
    ctx.alloc_functs(std::get<1>(vars_functs_preds));
    ctx.alloc_preds(std::get<2>(vars_functs_preds));
    auto pt = ctx2.convert_ndsequent(proof->get_thesis());
    std::cout << tb.print_sentence(pt, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << "\n";

    try {
        auto prover = ctx2.convert_proof(proof);
        //auto prover = ctx.replace_prover(Forall::create(Variable::create("z"), Equal::create(Variable::create("x"), Variable::create("y"))), "z", Variable::create("x"));
        //auto prover = ctx.not_free_prover(False::create(), "x");
        CreativeProofEngineImpl<ParsingTree2<SymTok, LabTok>> engine(tb);
        bool res = prover(engine);
        if (!res) {
            std::cout << "Proof failed...\n";
        } else {
            std::cout << "Proof proved: " << tb.print_sentence(engine.get_stack().back().second, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << "\n";
            std::cout << "Proof:";
            const auto &labels = engine.get_proof_labels();
            for (const auto &label : labels) {
                if (label != LabTok{}) {
                    std::cout << " " << tb.resolve_label(label);
                } else {
                    std::cout << " *";
                }
            }
            std::cout << "\n";
        }
    } catch (const std::exception &e) {
        std::cout << "This proof is not supported yet...\n";
        std::cout << e.what() << "\n";
    } catch (const ProofException<ParsingTree2<SymTok, LabTok>> &e) {
        std::cout << e.get_reason() << "\n";
        tb.dump_proof_exception(e, std::cout);
    }

    return 0;
}
gio_static_block {
    gio::register_main_function("read_gapt", read_gapt_main);
}

}
