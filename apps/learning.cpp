
#include <iostream>
#include <random>
#include <functional>

#include <boost/functional/hash.hpp>

#include "test/test_env.h"
#include "mm/toolbox.h"
#include "mm/proof.h"
#include "parsing/unif.h"
#include "utils/utils.h"

void print_parsing_tree(const ParsingTree< SymTok, LabTok > &pt) {
    bool first = true;
    for (const auto &child : pt.children) {
        if (!first) {
            std::cout << " ";
        }
        print_parsing_tree(child);
        first = false;
    }
    if (!first) {
        std::cout << " ";
    }
    std::cout << pt.children.size() << " " << pt.label;
}

void print_trace(const ProofTree< Sentence > &pt, const LibraryToolbox &tb, const Assertion &ass) {
    size_t essentials_num = 0;
    for (const auto &child : pt.children) {
        if (child.essential) {
            print_trace(child, tb, ass);
            essentials_num++;
        }
    }
    auto it = find(ass.get_ess_hyps().begin(), ass.get_ess_hyps().end(), pt.label);
    auto parsing_tree = tb.parse_sentence(pt.sentence.begin()+1, pt.sentence.end(), tb.get_turnstile_alias());
    if (it == ass.get_ess_hyps().end()) {
        std::cout << "# " << essentials_num << " " << tb.resolve_label(pt.label) << " " << tb.print_sentence(pt.sentence, SentencePrinter::STYLE_PLAIN) << std::endl;
        //std::cout << essentials_num << " " << pt.label << " " << tb.print_sentence(pt.sentence, SentencePrinter::STYLE_NUMBERS) << std::endl;
        std::cout << essentials_num << " " << pt.label << " ";
        print_parsing_tree(parsing_tree);
        std::cout << std::endl;
    } else {
        std::cout << "# 0 _hyp" << (it - ass.get_ess_hyps().begin()) << " " << tb.print_sentence(pt.sentence, SentencePrinter::STYLE_PLAIN) << std::endl;
        //std::cout << "0 0 " << tb.print_sentence(pt.sentence, SentencePrinter::STYLE_NUMBERS) << std::endl;
        std::cout << "0 0 ";
        print_parsing_tree(parsing_tree);
        std::cout << std::endl;
    }
}

int dissector_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    auto &tb = data.tb;

    const Assertion &ass = tb.get_assertion(tb.get_label("ftalem7"));
    UncompressedProof unc_proof = ass.get_proof_operator(tb)->uncompress();
    auto pe = unc_proof.get_executor< Sentence >(tb, ass, true);
    pe->execute();
    const ProofTree< Sentence > &pt = pe->get_proof_tree();
    print_trace(pt, tb, ass);

    return 0;
}
static_block {
    register_main_function("dissector", dissector_main);
}

struct ProofStat {
    size_t proof_size = 0;
    size_t ess_proof_size = 0;
    size_t ess_hyp_num = 0;
    size_t ess_hyp_steps = 0;
};

std::ostream &operator<<(std::ostream &os, const ProofStat &stat) {
    return os << stat.proof_size << " " << stat.ess_proof_size << " " << stat.ess_hyp_num << " " << stat.ess_hyp_steps;
}

struct StepContext {
    ParsingTree2< SymTok, LabTok > thesis;
    std::vector< ParsingTree2< SymTok, LabTok > > hypotheses;
    std::set< std::pair< LabTok, LabTok > > dists;
};

namespace boost {
template <>
struct hash< StepContext > {
    typedef StepContext argument_type;
    typedef std::size_t result_type;
    result_type operator()(const argument_type &x) const noexcept {
        result_type res = 0;
        boost::hash_combine(res, x.thesis);
        boost::hash_combine(res, x.hypotheses);
        boost::hash_combine(res, x.dists);
        return res;
    }
};
}

struct StepProof {
    LabTok label;
    std::vector< StepContext > children;
};

std::unordered_map< StepContext, StepProof, boost::hash< StepContext > > mega_map;

void proof_stat_unwind_tree(const ProofTree< Sentence > &pt, const Assertion &ass, ProofStat &stat) {
    if (pt.essential) {
        stat.ess_proof_size++;
        if (find(ass.get_ess_hyps().begin(), ass.get_ess_hyps().end(), pt.label) != ass.get_ess_hyps().end()) {
            stat.ess_hyp_steps++;
        }
    }
    for (const auto &child : pt.children) {
        proof_stat_unwind_tree(child, ass, stat);
    }
}

int proofs_stats_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    auto &lib = data.lib;
    //auto &tb = data.tb;

    TextProgressBar tpb(100, (double) lib.get_assertions().size());
    std::vector< std::pair< LabTok, ProofStat > > proofs_stats;
    for (const Assertion &ass : lib.get_assertions()) {
        if (!ass.is_valid()) {
            continue;
        }
        if (!ass.is_theorem()) {
            continue;
        }
        if (!ass.has_proof()) {
            continue;
        }
        auto proof = ass.get_proof_operator(lib)->compress();
        auto exec = proof.get_executor< Sentence >(lib, ass, true);
        exec->execute();

        ProofStat stat;
        //stat.proof_size = proof.get_labels().size();
        stat.ess_hyp_num = ass.get_ess_hyps().size();
        proof_stat_unwind_tree(exec->get_proof_tree(), ass, stat);
        proofs_stats.push_back(std::make_pair(ass.get_thesis(), stat));
        tpb.report(ass.get_thesis());
    }
    tpb.finished();

    sort(proofs_stats.begin(), proofs_stats.end(), [](const auto &x, const auto &y) {
        return (x.second.ess_proof_size - x.second.ess_hyp_steps) < (y.second.ess_proof_size - y.second.ess_hyp_steps);
    });

    for (size_t i = 0; i < 20; i++) {
        std::cout << lib.resolve_label(proofs_stats[i].first) << ": " << proofs_stats[i].second << std::endl;
    }

    return 0;
}
static_block {
    register_main_function("proofs_stats", proofs_stats_main);
}

void gen_theorems(const BilateralUnificator< SymTok, LabTok > &unif,
                  const std::vector< ParsingTree2< SymTok, LabTok > > &open_hyps,
                  const std::vector< LabTok > &steps,
                  size_t hyps_pos,
                  const std::vector< const Assertion* > &useful_asses,
                  const ParsingTree2< SymTok, LabTok > &final_thesis,
                  LibraryToolbox &tb,
                  size_t depth,
                  const std::function< void(const ParsingTree2< SymTok, LabTok >&, const std::vector< ParsingTree2< SymTok, LabTok > >&, const std::vector< LabTok >&, LibraryToolbox&)> &callback) {
    if (depth == 0 || hyps_pos == open_hyps.size()) {
        auto unif2 = unif;
        SubstMap2< SymTok, LabTok > subst;
        bool res;
        tie(res, subst) = unif2.unify2();
        if (!res) {
            return;
        }
        auto thesis = substitute2(final_thesis, tb.get_standard_is_var(), subst);
        std::vector< ParsingTree2< SymTok, LabTok > > hyps;
        for (const auto &hyp : open_hyps) {
            hyps.push_back(substitute2(hyp, tb.get_standard_is_var(), subst));
        }
        callback(thesis, hyps, steps, tb);
    } else {
        gen_theorems(unif, open_hyps, steps, hyps_pos+1, useful_asses, final_thesis, tb, depth, callback);
        for (const auto assp : useful_asses) {
            tb.new_temp_var_frame();
            const Assertion &ass = *assp;
            ParsingTree2< SymTok, LabTok > thesis;
            std::vector< ParsingTree2< SymTok, LabTok > > hyps;
            tie(hyps, thesis) = tb.refresh_assertion2(ass);
            auto unif2 = unif;
            auto steps2 = steps;
            unif2.add_parsing_trees2(open_hyps[hyps_pos], thesis);
            steps2.push_back(ass.get_thesis());
            if (unif2.is_unifiable()) {
                //std::cout << "Attaching " << tb.resolve_label(ass.get_thesis()) << " in position " << hyps_pos << std::endl;
                auto open_hyps2 = open_hyps;
                open_hyps2.erase(open_hyps2.begin() + hyps_pos);
                open_hyps2.insert(open_hyps2.end(), hyps.begin(), hyps.end());
                gen_theorems(unif2, open_hyps2, steps2, hyps_pos, useful_asses, final_thesis, tb, depth-1, callback);
            }
            tb.release_temp_var_frame();
        }
    }
}

size_t count = 0;
void print_theorem(const ParsingTree2< SymTok, LabTok > &thesis, const std::vector< ParsingTree2< SymTok, LabTok > >&hyps, const std::vector< LabTok > &steps, LibraryToolbox &tb) {
    ::count++;
    bool finished = hyps.empty();
    if (::count % 10000 == 0 || finished) {
        std::cout << ::count << std::endl;
        std::cout << tb.print_sentence(thesis) << std::endl;
        std::cout << "with the hypotheses:" << std::endl;
        for (const auto &hyp : hyps) {
            std::cout << " * " << tb.print_sentence(hyp) << std::endl;
        }
        std::cout << "Proved with steps: " << tb.print_proof(steps) << std::endl;
    }
    if (finished) {
        exit(0);
    }
    /*if (::count == 100000) {
        exit(0);
    }*/
}

int gen_random_theorems_main(int argc, char *argv[]) {
    std::random_device rand_dev;
    std::mt19937 rand_mt;
    rand_mt.seed(rand_dev());

    auto &data = get_set_mm();
    auto &lib = data.lib;
    auto &tb = data.tb;
    auto standard_is_var = tb.get_standard_is_var();

    //string target_label_str(argv[1]);
    //LabTok target_label = lib.get_label(target_label_str);
    //auto target_pt = tb.get_parsed_sents2()[target_label];

    std::ostringstream oss;
    for (int i = 1; i < argc; i++) {
        oss << argv[i] << " ";
    }
    Sentence target_sent = tb.read_sentence(oss.str());
    auto target_pt1 = tb.parse_sentence(target_sent, tb.get_turnstile_alias());
    auto target_pt = pt_to_pt2(target_pt1);
    LabTok target_label = 0;

    std::vector< const Assertion* > useful_asses;
    for (const auto &ass : lib.get_assertions()) {
        if (ass.is_valid() && lib.get_sentence(ass.get_thesis()).at(0) == tb.get_turnstile()) {
            /*if (ass.get_thesis() >= target_label) {
                break;
            }*/
            if (ass.is_theorem() && ass.has_proof() && ass.get_proof_operator(lib)->is_trivial()) {
                //std::cout << "Proof for " << lib.resolve_label(ass.get_thesis()) << " is trivial" << std::endl;
            } else {
                if (ass.get_thesis() != target_label && ass.get_thesis()) {
                    useful_asses.push_back(&ass);
                }
            }
        }
    }
    std::sort(useful_asses.begin(), useful_asses.end(), [&lib](const auto &x, const auto &y) {
        return x->get_ess_hyps().size() < y->get_ess_hyps().size() || (x->get_ess_hyps().size() == y->get_ess_hyps().size() && lib.get_sentence(x->get_thesis()).size() > lib.get_sentence(y->get_thesis()).size());
    });
    std::cout << "There are " << useful_asses.size() << " useful assertions" << std::endl;

    std::set< LabTok > target_vars;
    collect_variables2(target_pt, standard_is_var, target_vars);
    std::function< bool(LabTok) > is_var = [&target_vars,&standard_is_var](LabTok x) {
        if (target_vars.find(x) != target_vars.end()) {
            return false;
        }
        return standard_is_var(x);
    };

    BilateralUnificator< SymTok, LabTok > unif(is_var);
    std::vector< ParsingTree2< SymTok, LabTok > > open_hyps;
    LabTok th_label;
    std::tie(th_label, std::ignore) = tb.new_temp_var(tb.get_turnstile_alias());
    ParsingTree2< SymTok, LabTok > final_thesis = var_parsing_tree(th_label, tb.get_turnstile_alias());
    final_thesis = target_pt;
    open_hyps.push_back(final_thesis);
    std::vector< LabTok > steps;

    gen_theorems(unif, open_hyps, steps, 0, useful_asses, final_thesis, tb, 2, print_theorem);
    return 0;

    for (size_t i = 0; i < 5; i++) {
        if (open_hyps.empty()) {
            std::cout << "Terminating early" << std::endl;
            break;
        }
        while (true) {
            // Select a random hypothesis, a random open hypothesis and let them match
            size_t ass_idx = std::uniform_int_distribution< size_t >(0, useful_asses.size()-1)(rand_mt);
            size_t hyp_idx = std::uniform_int_distribution< size_t >(0, open_hyps.size()-1)(rand_mt);
            const Assertion &ass = *useful_asses[ass_idx];
            if (i == 0 && ass.get_ess_hyps().size() == 0) {
                continue;
            }
            if (i > 1 && ass.get_ess_hyps().size() > 0) {
                continue;
            }
            ParsingTree2< SymTok, LabTok > thesis;
            std::vector< ParsingTree2< SymTok, LabTok > > hyps;
            tie(hyps, thesis) = tb.refresh_assertion2(ass);
            auto unif2 = unif;
            unif2.add_parsing_trees2(open_hyps[hyp_idx], thesis);
            if (unif2.unify2().first) {
                std::cout << "Attaching " << tb.resolve_label(ass.get_thesis()) << " in position " << hyp_idx << std::endl;
                unif = unif2;
                open_hyps.erase(open_hyps.begin() + hyp_idx);
                open_hyps.insert(open_hyps.end(), hyps.begin(), hyps.end());
                break;
            }
        }
    }

    SubstMap2< SymTok, LabTok > subst;
    bool res;
    tie(res, subst) = unif.unify2();

    if (res) {
        std::cout << "Unification succedeed and proved:" << std::endl;
        std::cout << tb.print_sentence(substitute2(final_thesis, tb.get_standard_is_var(), subst)) << std::endl;
        std::cout << "with the hypotheses:" << std::endl;
        for (const auto &hyp : open_hyps) {
            std::cout << " * " << tb.print_sentence(substitute2(hyp, tb.get_standard_is_var(), subst)) << std::endl;
        }
    } else {
        std::cout << "Unification failed" << std::endl;
    }

    return 0;
}
static_block {
    register_main_function("gen_random_theorems", gen_random_theorems_main);
}
