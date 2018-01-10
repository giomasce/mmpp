
#include "uct.h"

#include "utils/utils.h"
#include "test/test_env.h"
#include "platform.h"
#include "mm/proof.h"

#include <memory>
#include <iostream>

#define LOG_UCT

using namespace std;

#ifdef LOG_UCT
constexpr auto &cuct = cout;
#else
constexpr auto &cuct = cnull;
#endif

struct VisitContext {
    static uint32_t depth;
    string action;
    VisitContext(string action) : action(action) {
        this->log() << "Beginning " << this->action << endl;
        this->depth++;
    }
    ~VisitContext() {
        this->depth--;
        //this->log() << "Finishing " << this->action << endl;
    }
    static void insert_space() {
        for (uint32_t i = 0; i < VisitContext::depth; i++) {
            cuct << "  ";
        }
    }
    static ostream &log() {
        VisitContext::insert_space();
        return cuct;
    }
};
uint32_t VisitContext::depth = 0;

static ostream &cuct_log() {
    return VisitContext::log();
}

VisitResult UCTProver::visit()
{
    auto vc = VisitContext("global visit");
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
    //cuct_log() << this << ": Constructing UCTProver" << endl;
}

UCTProver::~UCTProver()
{
    //cuct_log() << this << ": Destructing UCTProver" << endl;
}

void UCTProver::init()
{
    this->compute_useful_assertions();
    this->root = SentenceNode::create(this->weak_from_this(), weak_ptr< StepNode >(), thesis);
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

VisitResult SentenceNode::visit()
{
    auto strong_uct = this->uct.lock();
    auto &rand = strong_uct->get_rand();
    auto &tb = strong_uct->get_toolbox();
    assert(!this->exhausted);
    this->visit_num++;
    auto vc = VisitContext("visiting SentenceNode for " + tb.print_sentence(this->sentence, SentencePrinter::STYLE_ANSI_COLORS_SET_MM).to_string());

    // First visit: do some trivial checks, but do not create new children
    if (this->visit_num == 1) {
        cuct_log() << "First visit" << endl;
        auto &hyps = strong_uct->get_hypotheses();
        auto it = find(hyps.begin(), hyps.end(), this->sentence);
        if (it != hyps.end()) {
            cuct_log() << "Proved with an hypothesis!" << endl;
            this->exhausted = true;
            return PROVED;
        } else {
            //cuct_log() << "Not proved with an hypothesis" << endl;
            return CONTINUE;
        }
    }

    // We might try to create a new child, if there are too few
    bool created_child = false;
    //cuct_log() << "Later visit" << endl;
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
                cuct_log() << "Creating a new StepNode child" << endl;
                this->children.push_back(StepNode::create(this->uct, this->weak_from_this(), ass.get_thesis(), subst_map));
                created_child = true;
                break;
            }
        }
    }

    // And now let us visit a child; if we just created one, we visit that one
    std::vector< std::shared_ptr< StepNode > >::iterator child_it;
    if (created_child) {
        //cuct_log() << "Visiting the child we just created" << endl;
        child_it = this->children.end() - 1;
    } else {
        // FIXME Implement a better policy
        //cuct_log() << "Visiting a random child" << endl;
        child_it = random_choose(this->children.begin(), this->children.end(), rand);
    }
    auto child = *child_it;
    this->total_children_value -= child->get_value();
    VisitResult res = child->visit();
    this->total_children_value += child->get_value();

    if (res == DEAD) {
        // If the node is dead, we remove it from the children
        cuct_log() << "Child is dead, removing it" << endl;
        this->value -= child->get_value();
        this->children.erase(child_it);
    } else if (res == PROVED) {
        // If the visit succeeded, bingo! This node is proved, and we can evict all children exept for the one we just visited
        cuct_log() << "We found a proof!" << endl;
        this->exhausted = true;
        this->children = { child };
        return PROVED;
    }

    this->value = this->total_children_value / this->visit_num;
    return CONTINUE;
}

float SentenceNode::get_value() {
    return this->value;
}

uint32_t SentenceNode::get_visit_num() {
    return this->visit_num;
}

std::weak_ptr<StepNode> SentenceNode::get_parent() {
    return this->parent;
}

const ParsingTree2<SymTok, LabTok> &SentenceNode::get_sentence()
{
    return this->sentence;
}

SentenceNode::SentenceNode(std::weak_ptr<UCTProver> uct, std::weak_ptr<StepNode> parent, const ParsingTree2<SymTok, LabTok> &sentence) : uct(uct), parent(parent), sentence(sentence) {
    //cuct_log() << this << ": Constructing SentenceNode" << endl;
    this->ass_it = this->uct.lock()->get_useful_asses().cbegin();
}

SentenceNode::~SentenceNode()
{
    //cuct_log() << this << ": Destructing SentenceNode" << endl;
}

VisitResult StepNode::visit()
{
    auto strong_uct = this->uct.lock();
    auto &rand = strong_uct->get_rand();
    auto &tb = strong_uct->get_toolbox();
    assert(!this->exhausted);
    auto vc = VisitContext("visiting StepNode for label " + tb.resolve_label(this->label));

    // If we have no children this must be the first visit, because it is illegal to visit a node that has already been proved
    if (this->children.empty()) {
        cuct_log() << "First visit, let us create children" << endl;
        return this->create_children();
    }

    // Then we visit a random child
    //cuct_log() << "Later visit, let us visit a random child" << endl;
    size_t i = random_choose(this->active_children.begin(), this->active_children.end(), rand) - this->active_children.begin();
    return this->visit_child(i);
}

float StepNode::get_value() {
    if (this->active_children.empty()) {
        return 0.0;
    }
    return this->active_children[this->worst_child]->get_value();
}

uint32_t StepNode::get_visit_num() {
    if (this->active_children.empty()) {
        return 0;
    }
    return this->active_children[this->worst_child]->get_visit_num();
}

std::weak_ptr<SentenceNode> StepNode::get_parent()
{
    return this->parent;
}

StepNode::StepNode(std::weak_ptr<UCTProver> uct, std::weak_ptr<SentenceNode> parent, LabTok label, const SubstMap2<SymTok, LabTok> &const_subst_map) : uct(uct), parent(parent), label(label), const_subst_map(const_subst_map) {
    //cuct_log() << this << ": Constructing StepNode" << endl;
}

StepNode::~StepNode()
{
    //cuct_log() << this << ": Destructing StepNode" << endl;
}

VisitResult StepNode::create_child(const ParsingTree2<SymTok, LabTok> &sent)
{
    cuct_log() << "Spawning a child for " << this->uct.lock()->get_toolbox().print_sentence(sent, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << endl;
    // Check that we don't have the same sentence of an ancestor
    shared_ptr< SentenceNode > parent_sent = this->parent.lock();
    while (true) {
        if (!parent_sent) {
            break;
        }
        if (parent_sent->get_sentence() == sent) {
            cuct_log() << "New child coincides with one ancestor, dying..." << endl;
            return DEAD;
        }
        shared_ptr< StepNode > parent_step = parent_sent->get_parent().lock();
        if (!parent_step) {
            break;
        }
        parent_sent = parent_step->get_parent().lock();
    }
    this->children.push_back(SentenceNode::create(this->uct, this->weak_from_this(), sent));
    return CONTINUE;
}

VisitResult StepNode::create_children()
{
    auto strong_uct = this->uct.lock();
    auto &tb = strong_uct->get_toolbox();

    this->unconst_subst_map = tb.build_refreshing_full_subst_map2(tb.get_assertion_unconst_vars()[this->label]);
    auto full_subst_map = update2(this->const_subst_map, this->unconst_subst_map, true);
    const Assertion &ass = tb.get_assertion(this->label);
    assert(ass.is_valid());
    for (auto hyp_tok : ass.get_ess_hyps()) {
        auto subst_hyp = substitute2(tb.get_parsed_sents2()[hyp_tok], tb.get_standard_is_var(), full_subst_map);
        VisitResult res = this->create_child(subst_hyp);
        assert(res != PROVED);
        if (res == DEAD) {
            this->children.clear();
            this->exhausted = true;
            return DEAD;
        }
    }
    if (this->children.empty()) {
        cuct_log() << "No children, so nothing to prove!" << endl;
        this->exhausted = true;
        return PROVED;
    }
    cuct_log() << "Visiting each child for the first time" << endl;
    this->active_children = this->children;
    bool found_continue = false;
    for (size_t i = 0; i < this->children.size(); i++) {
        // Do the first visit backwards, so that if some child is immediately evicted because it is trivial there is no problem
        VisitResult res = this->visit_child(this->children.size() - i - 1);
        if (res == DEAD) {
            this->children.clear();
            return DEAD;
        } else if (res == CONTINUE) {
            found_continue = true;
        }
    }
    return found_continue ? CONTINUE : PROVED;
}

VisitResult StepNode::visit_child(size_t i)
{
    VisitResult res = this->active_children[i]->visit();
    if (res == PROVED) {
        cuct_log() << "We found a proof for a child!" << endl;
        this->active_children.erase(this->active_children.begin() + i);
        if (this->active_children.empty()) {
            cuct_log() << "All children finally proved!" << endl;
            this->exhausted = true;
            return PROVED;
        }
    } else if (res == DEAD) {
        this->exhausted = true;
        return DEAD;
    }
    this->worst_child = min_element(this->active_children.begin(), this->active_children.end(), [](auto &a, auto &b) { return a->get_value() < b->get_value(); }) - this->active_children.begin();
    return CONTINUE;
}

ParsingTree2< SymTok, LabTok > string_to_pt2(string sent_str, const LibraryToolbox &tb) {
    auto sent = tb.read_sentence(sent_str);
    auto thesis_pt = tb.parse_sentence(sent.begin()+1, sent.end(), tb.get_turnstile_alias());
    auto thesis_pt2 = pt_to_pt2(thesis_pt);
    return thesis_pt2;
}

vector< pair< ParsingTree2< SymTok, LabTok >, vector< ParsingTree2< SymTok, LabTok > > > > parse_tests(const LibraryToolbox &tb) {
    boost::filesystem::ifstream fin(platform_get_resources_base() / "tests.txt");
    string line;
    bool found_first = false;
    ParsingTree2< SymTok, LabTok > current_thesis;
    vector< ParsingTree2< SymTok, LabTok > > current_hyps;
    vector< pair< ParsingTree2< SymTok, LabTok >, vector< ParsingTree2< SymTok, LabTok > > > > problems;
    while (getline(fin, line)) {
        rtrim(line);
        if (line.empty()) {
            continue;
        }
        if (line.at(0) == '#') {
            continue;
        }
        if (line.at(0) == ' ') {
            current_hyps.push_back(string_to_pt2(line, tb));
        } else {
            if (found_first) {
                problems.push_back(make_pair(current_thesis, current_hyps));
            }
            found_first = true;
            current_hyps.clear();
            current_thesis = string_to_pt2(line, tb);
        }
    }
    if (found_first) {
        problems.push_back(make_pair(current_thesis, current_hyps));
    }
    return problems;
}

int uct_main(int argc, char *argv[]) {
    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    size_t pb_idx = 0;
    if (argc == 2) {
        pb_idx = atoi(argv[1]);
    }

    auto problems = parse_tests(tb);
    auto problem = problems.at(pb_idx);
    auto prover = UCTProver::create(tb, problem.first, problem.second);
    for (int i = 0; i < 100; i++) {
        VisitResult res = prover->visit();
        cuct_log() << endl;
        if (res == PROVED) {
            cuct_log() << "We found a proof after " << i+1 << " iterations, exiting!" << endl;
            break;
        } else if (res == DEAD) {
            cuct_log() << "The node is dead, the search has failed..." << endl;
        }
        while (cin.get() != '\n') {}
    }

    return 0;
}
static_block {
    register_main_function("uct", uct_main);
}
