
#include "uct.h"

#include "utils/utils.h"
#include "test/test_env.h"

#include <memory>

using namespace std;

bool UCTProver::visit()
{
    return this->root->visit();
}

const std::vector<ParsingTree2<SymTok, LabTok> > &UCTProver::get_hypotheses() const {
    return this->hypotheses;
}

const std::vector<const Assertion *> &UCTProver::get_useful_asses() const {
    return this->useful_asses;
}

LibraryToolbox &UCTProver::get_toolbox() const {
    return this->tb;
}

std::ranlux48 &UCTProver::get_rand() {
    return this->rand;
}

UCTProver::UCTProver(LibraryToolbox &tb, const ParsingTree2<SymTok, LabTok> &thesis, const std::vector<ParsingTree2<SymTok, LabTok> > &hypotheses) : tb(tb), thesis(thesis), hypotheses(hypotheses), rand(2204) {
    this->root = SentenceNode::create(this->weak_from_this(), thesis);
    this->compute_useful_assertions();
}

void UCTProver::compute_useful_assertions()
{
    for (const auto &ass : this->tb.get_library().get_assertions()) {
        if (ass.is_valid() && this->tb.get_sentence(ass.get_thesis()).at(0) == this->tb.get_turnstile()) {
            if (ass.is_theorem() && ass.has_proof() && ass.get_proof_executor(this->tb)->is_trivial()) {
                continue;
            }
            /*if (ass.get_thesis() == target_label || !ass.get_thesis()) {
                continue;
            }*/
            this->useful_asses.push_back(&ass);
        }
    }
    // We sort assertions in order to favour theorems with few hypotheses and more specific thesis
    sort(this->useful_asses.begin(), this->useful_asses.end(), [this](const auto &x, const auto &y) {
        return x->get_ess_hyps().size() < y->get_ess_hyps().size() || (x->get_ess_hyps().size() == y->get_ess_hyps().size() && this->tb.get_sentence(x->get_thesis()).size() > this->tb.get_sentence(y->get_thesis()).size());
    });
    //cout << "There are " << this->useful_asses.size() << " useful assertions" << endl;
}

bool SentenceNode::visit()
{
    auto strong_uct = this->uct.lock();
    auto &rand = strong_uct->get_rand();
    auto &tb = strong_uct->get_toolbox();
    assert(!this->proved);
    this->visit_num++;

    // First visit: do some trivial checks, but do not create new children
    if (this->visit_num == 1) {
        auto &hyps = strong_uct->get_hypotheses();
        auto it = find(hyps.begin(), hyps.end(), this->sentence);
        if (it != hyps.end()) {
            this->proved = true;
            return true;
        } else {
            return false;
        }
    }

    // We might try to create a new child, if there are too few
    bool created_child = false;
    if (this->children.size() == 0 || this->children.size() < (this->visit_num / 3)) {
        while (this->ass_it != strong_uct->get_useful_asses().cend()) {
            const Assertion &ass = **this->ass_it;
            this->ass_it++;
            ParsingTree2< SymTok, LabTok > thesis = tb.get_parsed_sents2()[ass.get_thesis()];
            UnilateralUnificator< SymTok, LabTok > unif(tb.get_standard_is_var());
            unif.add_parsing_trees2(thesis, this->sentence);
            bool unifiable;
            SubstMap2< SymTok, LabTok > subst_map;
            tie(unifiable, subst_map) = unif.unify2();
            if (!unifiable) {
                continue;
            } else {
                this->children.push_back(StepNode::create(this->uct, ass.get_thesis(), subst_map));
                created_child = true;
            }
        }
    }

    // And now let us visit a child; if we just created one, we visit that one
    std::vector< std::shared_ptr< StepNode > >::iterator child_it;
    if (created_child) {
        child_it = this->children.end() - 1;
    } else {
        // FIXME Implement a better policy
        child_it = random_choose(this->children.begin(), this->children.end(), rand);
    }
    auto child = *child_it;
    this->total_children_value -= child->get_value();
    bool res = child->visit();
    this->total_children_value += child->get_value();
    this->value = this->total_children_value / this->visit_num;

    // If the visit succeeded, bingo! This node is proved, and we can evict all children exept for the one we just visited
    if (res) {
        this->proved = true;
        this->children = { child };
        return true;
    }

    return false;
}

float SentenceNode::get_value() {
    return this->value;
}

uint32_t SentenceNode::get_visit_num() {
    return this->visit_num;
}

bool StepNode::visit()
{
    auto strong_uct = this->uct.lock();
    auto &rand = strong_uct->get_rand();
    assert(!this->proved);

    return false;
}

float StepNode::get_value() {
    return this->value;
}

uint32_t StepNode::get_visit_num() {
    return this->visit_num;
}

void StepNode::init() {
    this->create_children();
}

void StepNode::create_children()
{
    auto strong_uct = this->uct.lock();
    auto &tb = strong_uct->get_toolbox();

    this->unconst_subst_map = tb.build_refreshing_full_subst_map2(tb.get_assertion_unconst_vars()[this->label]);
    auto full_subst_map = update2(this->const_subst_map, this->unconst_subst_map, true);
    const Assertion &ass = tb.get_assertion(this->label);
    assert(ass.is_valid());
    for (auto hyp_tok : ass.get_ess_hyps()) {
        auto subst_hyp = substitute2(tb.get_parsed_sents2()[hyp_tok], tb.get_standard_is_var(), full_subst_map);
        this->children.push_back(SentenceNode::create(this->uct, subst_hyp));
    }
    this->active_children = this->children;
    for (size_t i = 0; i < this->children.size(); i++) {
        // Do the first visit backwards, so that if some child is immediately evicted because it is trivial there is no problem
        this->visit_child(this->children.size() - i - 1);
    }
    if (!this->proved) {
        const auto &worst = *min_element(this->active_children.begin(), this->active_children.end(), [](auto &a, auto &b) { return a->get_value() < b->get_value(); });
        this->value = worst->get_value();
        this->visit_num = worst->get_visit_num();
    }
}

bool StepNode::visit_child(size_t i)
{
    bool res = this->active_children[i]->visit();
    if (res) {
        this->active_children.erase(this->active_children.begin() + i);
        if (this->active_children.empty()) {
            this->proved = true;
            return true;
        }
    }
    return false;
}

int uct_test_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    auto thesis_sent = tb.read_sentence("|- ( ph -> ph )");
    auto thesis_pt = tb.parse_sentence(thesis_sent);
    auto thesis_pt2 = pt_to_pt2(thesis_pt);

    auto prover = UCTProver::create(tb, thesis_pt2, std::vector< ParsingTree2< SymTok, LabTok > >{});
    prover->visit();

    return 0;
}
static_block {
    register_main_function("uct_test", uct_test_main);
}
