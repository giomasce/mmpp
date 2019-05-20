
#include <boost/filesystem/fstream.hpp>

#include <giolib/static_block.h>
#include <giolib/main.h>
#include <giolib/containers.h>

#include "uct.h"

#include "utils/utils.h"
#include "mm/setmm_loader.h"
#include "platform.h"
#include "mm/proof.h"
#include "mm/ptengine.h"

#include <memory>
#include <iostream>
#include <type_traits>
#include <iterator>

// UCT logging is not thread-safe; disable it when using webmmpp
//#define LOG_UCT

// This uses some trickery in https://stackoverflow.com/q/51170934/807307
struct OstreamWrapper {
    OstreamWrapper(std::ostream &out) : out(out) {}

    template< typename T >
    decltype(auto) operator<<(T &&arg) {
        return out << std::forward< T >(arg);
    }

    decltype(auto) operator<<(std::ostream& (&arg)(std::ostream&)) {
        return out << arg;
    }

    std::ostream &out;
};

struct VisitContext {
    static uint32_t depth;
    std::string action;
    VisitContext(std::string action) : action(action) {
#ifdef LOG_UCT
        this->log() << "Begin " << this->action << std::endl;
#endif
        this->depth++;
    }
    VisitContext(const VisitContext&) = delete;
    ~VisitContext() {
        this->depth--;
#ifdef LOG_UCT
        //this->log() << "Finishing " << this->action << std::endl;
#endif
    }
    static void insert_space() {
        for (uint32_t i = 0; i < VisitContext::depth; i++) {
            std::cout << "  ";
        }
    }
    static OstreamWrapper log() {
        VisitContext::insert_space();
        return OstreamWrapper(std::cout);
    }
};
uint32_t VisitContext::depth = 0;

#ifdef LOG_UCT
static inline decltype(auto) visit_log() {
    return VisitContext::log();
}
#endif

VisitResult UCTProver::visit()
{
#ifdef LOG_UCT
    VisitContext vc("global visit");
#endif
    return this->root->visit();
}

const std::vector<ParsingTree2<SymTok, LabTok> > &UCTProver::get_hypotheses() const {
    return this->hypotheses;
}

const LibraryToolbox &UCTProver::get_toolbox() const {
    return this->tb;
}

temp_allocator &UCTProver::get_temp_allocator() {
    return this->tsa;
}

std::ranlux48 &UCTProver::get_rand() {
    return this->rand;
}

const std::set<std::pair<LabTok, LabTok> > &UCTProver::get_antidists() const
{
    return this->antidists;
}

void UCTProver::replay_proof(CheckpointedProofEngine &engine) const
{
    this->root->replay_proof(engine);
}

UCTProver::UCTProver(const LibraryToolbox &tb, const ParsingTree2<SymTok, LabTok> &thesis, const std::vector<ParsingTree2<SymTok, LabTok> > &hypotheses, const std::set<std::pair<LabTok, LabTok> > &antidists)
    : antidists(antidists), tb(tb), tsa(tb), thesis(thesis), hypotheses(hypotheses), rand(2204) {
#ifdef LOG_UCT
    //visit_log() << this << ": Constructing UCTProver" << endl;
#endif
}

UCTProver::~UCTProver()
{
#ifdef LOG_UCT
    //visit_log() << this << ": Destructing UCTProver" << endl;
#endif
}

void UCTProver::init()
{
    this->compute_useful_assertions();
    this->root = SentenceNode::create(this->weak_from_this(), std::weak_ptr< StepNode >(), thesis);
}

bool UCTProver::is_assertion_useful(const Assertion &ass) const
{
    if (!ass.is_valid()) {
        return false;
    }
    if (this->tb.get_sentence(ass.get_thesis()).at(0) != this->tb.get_turnstile()) {
        return false;
    }
    if (ass.is_theorem() && ass.has_proof() && ass.get_proof_operator(this->tb)->is_trivial()) {
        return false;
    }
    if (ass.is_usage_disc()) {
        return false;
    }
    /*if (ass.get_thesis() == target_label || !ass.get_thesis()) {
        return false;
    }*/
    return true;
}

const std::unordered_map<LabTok, std::vector<LabTok> > &UCTProver::get_root_useful_asses() const
{
    return this->root_useful_asses;
}

const std::unordered_map<LabTok, std::vector<LabTok> > &UCTProver::get_imp_con_useful_asses() const
{
    return this->imp_con_useful_asses;
}

void UCTProver::set_children_callbacks(std::vector<std::function<void ()> > &&children_callbacks)
{
    this->children_callbacks = std::move(children_callbacks);
}

std::function<void ()> UCTProver::get_children_callback(size_t idx) const
{
    return this->children_callbacks.at(idx);
}

static std::unordered_map< LabTok, std::vector< LabTok > > filter_assertions(const UCTProver &uct, const std::unordered_map< LabTok, std::vector< LabTok > > &asses) {
    const LibraryToolbox &tb = uct.get_toolbox();
    std::unordered_map< LabTok, std::vector< LabTok > > ret;
    for (const auto &p : asses) {
        LabTok label = p.first;
        auto &new_list = ret[label];
        const auto &orig_list = p.second;
        for (const LabTok &lab2 : orig_list) {
            if (uct.is_assertion_useful(tb.get_assertion(lab2))) {
                new_list.push_back(lab2);
            }
        }
        sort(new_list.begin(), new_list.end(), [&tb](const auto &x, const auto &y) {
            const auto &assx = tb.get_assertion(x);
            const auto &assy = tb.get_assertion(y);
            return assx.get_ess_hyps().size() < assy.get_ess_hyps().size() || (assx.get_ess_hyps().size() == assy.get_ess_hyps().size() && tb.get_sentence(x).size() > tb.get_sentence(y).size());
        });
    }
    return ret;
}

void UCTProver::compute_useful_assertions()
{
    this->root_useful_asses = filter_assertions(*this, tb.get_root_labels_to_theses());
    this->imp_con_useful_asses = filter_assertions(*this, tb.get_imp_con_labels_to_theses());
}

VisitResult SentenceNode::visit()
{
    auto strong_uct = this->uct.lock();
    auto &rand = strong_uct->get_rand();
    auto &tb = strong_uct->get_toolbox();
    assert(!this->exhausted);
    this->visit_num++;
#ifdef LOG_UCT
    VisitContext vc("visiting SentenceNode for " + tb.print_sentence(this->sentence, SentencePrinter::STYLE_ANSI_COLORS_SET_MM).to_string());
#endif

    // First visit: do some trivial checks, but do not create new children
    if (this->visit_num == 1) {
#ifdef LOG_UCT
        visit_log() << "First visit" << std::endl;
#endif
        auto &hyps = strong_uct->get_hypotheses();
        auto it = find(hyps.begin(), hyps.end(), this->sentence);
        if (it != hyps.end()) {
#ifdef LOG_UCT
            visit_log() << "Proved with an hypothesis!" << std::endl;
#endif
            this->exhausted = true;
            this->hyp_num = static_cast< size_t >(it - hyps.begin());
            return PROVED;
        } else {
#ifdef LOG_UCT
            //visit_log() << "Not proved with an hypothesis" << std::endl;
#endif
            return CONTINUE;
        }
    }

    // We might try to create a new child, if there are too few
    bool created_child = false;
#ifdef LOG_UCT
    //visit_log() << "Later visit" << std::endl;
#endif
    if (this->children.size() == 0 || this->children.size() < (this->visit_num / 3)) {
        while (this->ass_it != this->ass_range.end()) {
            const Assertion &ass = tb.get_assertion(*this->ass_it);
            this->ass_it++;
            ParsingTree2< SymTok, LabTok > thesis = tb.get_parsed_sent2(ass.get_thesis());
            UnilateralUnificator< SymTok, LabTok > unif(tb.get_standard_is_var());
            unif.add_parsing_trees2(thesis, this->sentence);
            bool unifiable;
            SubstMap2< SymTok, LabTok > subst_map;
            tie(unifiable, subst_map) = unif.unify2();
            if (!unifiable || !this->check_subst_map(subst_map, ass)) {
                continue;
            } else {
#ifdef LOG_UCT
                visit_log() << "Creating a new StepNode child" << std::endl;
#endif
                auto strong_parent = this->get_parent().lock();
                this->children.push_back(StepNode::create(this->uct, this->weak_from_this(), ass.get_thesis(), subst_map,
                                                          strong_parent ? strong_parent->get_open_vars() : std::map< LabTok, gio::safe_weak_ptr< StepNode > >{}));
                created_child = true;
                break;
            }
        }
    }

    // And now let us visit a child; if we just created one, we visit that one
    std::vector< std::shared_ptr< StepNode > >::iterator child_it;
    if (created_child) {
#ifdef LOG_UCT
        //visit_log() << "Visiting the child we just created" << std::endl;
#endif
        child_it = this->children.end() - 1;
    } else {
        // FIXME Implement a better policy
#ifdef LOG_UCT
        //visit_log() << "Visiting a random child" << std::endl;
#endif
        child_it = gio::random_choose(this->children.begin(), this->children.end(), rand);
    }
    auto child = *child_it;
    this->total_children_value -= child->get_value();
    VisitResult res = child->visit();
    this->total_children_value += child->get_value();

    if (res == DEAD) {
        // If the node is dead, we remove it from the children
#ifdef LOG_UCT
        visit_log() << "Child is dead, removing it" << std::endl;
#endif
        this->value -= child->get_value();
        this->children.erase(child_it);
    } else if (res == PROVED) {
        // If the visit succeeded, bingo! This node is proved, and we can evict all children exept for the one we just visited
#ifdef LOG_UCT
        visit_log() << "We found a proof!" << std::endl;
#endif
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

void SentenceNode::replay_proof(CheckpointedProofEngine &engine) const
{
    assert(this->exhausted);
    assert(this->children.size() <= 1);
    if (this->children.size() == 1) {
        this->children[0]->replay_proof(engine);
    } else {
        auto strong_uct = this->uct.lock();
        strong_uct->get_children_callback(this->hyp_num)();
    }
}

const std::vector< LabTok > empty_lab_vector;
const LabTok zero_label = {};

SentenceNode::SentenceNode(std::weak_ptr<UCTProver> uct, std::weak_ptr<StepNode> parent, const ParsingTree2<SymTok, LabTok> &sentence) : uct(uct), parent(parent), sentence(sentence), ass_range(boost::range::join(empty_lab_vector, empty_lab_vector)) {
#ifdef LOG_UCT
    //visit_log() << this << ": Constructing SentenceNode" << endl;
#endif
    auto strong_uct = this->uct.lock();
    auto &root_usefuls = strong_uct->get_root_useful_asses();
    auto &con_usefuls = strong_uct->get_imp_con_useful_asses();
    const auto &tb = strong_uct->get_toolbox();
    LabTok root_label = sentence.get_root().get_node().label;
    if (tb.get_standard_is_var()(root_label)) {
        root_label = zero_label;
    }
    const std::vector< LabTok > *f1 = &empty_lab_vector;
    const std::vector< LabTok > *f2 = &empty_lab_vector;
    if (root_label != tb.get_imp_label()) {
        auto it = root_usefuls.find(root_label);
        if (it != root_usefuls.end()) {
            f1 = &it->second;
        }
        if (root_label != zero_label) {
            it = root_usefuls.find(zero_label);
            if (it != root_usefuls.end()) {
                f2 = &it->second;
            }
        }
    } else {
        LabTok con_label = sentence.get_root().get_first_child().get_next_siebling().get_node().label;
        if (tb.get_standard_is_var()(con_label)) {
            con_label = zero_label;
        }
        auto it = con_usefuls.find(con_label);
        if (it != con_usefuls.end()) {
            f1 = &it->second;
        }
        if (con_label != zero_label) {
            it = con_usefuls.find(zero_label);
            if (it != con_usefuls.end()) {
                f2 = &it->second;
            }
        }
    }
    this->ass_range = boost::range::join(*f1, *f2);
    this->ass_it = this->ass_range.begin();
}

SentenceNode::~SentenceNode()
{
#ifdef LOG_UCT
    //visit_log() << this << ": Destructing SentenceNode" << endl;
#endif
}

bool SentenceNode::check_subst_map(const SubstMap2<SymTok, LabTok> &subst_map, const Assertion &ass)
{
    auto strong_uct = this->uct.lock();
    const auto &tb = strong_uct->get_toolbox();
    const auto &antidists = strong_uct->get_antidists();
    auto dists = propagate_dists< ParsingTree2< SymTok, LabTok > >(ass, subst_map, tb);
    if (!gio::has_no_diagonal(dists.begin(), dists.end())) {
        return false;
    }
    if (!gio::is_disjoint(dists.begin(), dists.end(), antidists.begin(), antidists.end())) {
        return false;
    }
    return true;
}

VisitResult StepNode::visit()
{
    auto strong_uct = this->uct.lock();
    auto &rand = strong_uct->get_rand();
    auto &tb = strong_uct->get_toolbox();
    assert(!this->exhausted);
#ifdef LOG_UCT
    VisitContext vc("visiting StepNode for label " + tb.resolve_label(this->label));
#else
    (void) tb;
#endif

    // If we have no children this must be the first visit, because it is illegal to visit a node that has already been proved
    if (this->children.empty()) {
#ifdef LOG_UCT
        visit_log() << "First visit, let us create children" << std::endl;
#endif
        return this->create_children();
    }

#ifdef LOG_UCT
    auto log = visit_log();
    log << "Currently open variables:";
    for (const auto &var : this->open_vars) {
        log << " " << tb.resolve_symbol(tb.get_var_lab_to_sym(var.first));
    }
    log << std::endl;
#endif

    // Then we visit a random child
#ifdef LOG_UCT
    //visit_log() << "Later visit, let us visit a random child" << std::endl;
#endif
    size_t i = static_cast< size_t >(gio::random_choose(this->active_children.begin(), this->active_children.end(), rand) - this->active_children.begin());
    return this->visit_child(i);
}

float StepNode::get_value() const {
    if (this->active_children.empty()) {
        return 0.0;
    }
    return this->active_children[this->worst_child]->get_value();
}

uint32_t StepNode::get_visit_num() const {
    if (this->active_children.empty()) {
        return 0;
    }
    return this->active_children[this->worst_child]->get_visit_num();
}

std::weak_ptr<SentenceNode> StepNode::get_parent() const
{
    return this->parent;
}

void StepNode::replay_proof(CheckpointedProofEngine &engine) const
{
    assert(this->exhausted);
    const auto &tb = this->uct.lock()->get_toolbox();

    // Push the substitution map (floating hypotheses)
    const Assertion &ass = tb.get_assertion(this->label);
    auto full_subst_map = update2(this->const_subst_map, this->unconst_subst_map, true);
    for (const auto &hyp_lab : ass.get_float_hyps()) {
        tb.build_type_prover(full_subst_map.at(hyp_lab))(engine);
    }

    // Push the essential hypotheses
    for (const auto &child : this->children) {
        child->replay_proof(engine);
    }

    // Invoke this step's theorem
    engine.process_label(this->label);
}

const std::map<LabTok, gio::safe_weak_ptr<StepNode> > &StepNode::get_open_vars() const
{
    return this->open_vars;
}

StepNode::StepNode(std::weak_ptr<UCTProver> uct, std::weak_ptr<SentenceNode> parent, LabTok label, const SubstMap2<SymTok, LabTok> &const_subst_map, const std::map<LabTok, gio::safe_weak_ptr<StepNode> > &open_vars)
    : uct(uct), parent(parent), label(label), const_subst_map(const_subst_map), open_vars(open_vars) {
#ifdef LOG_UCT
    //visit_log() << this << ": Constructing StepNode" << std::endl;
#endif
}

StepNode::~StepNode()
{
#ifdef LOG_UCT
    //visit_log() << this << ": Destructing StepNode" << std::endl;
#endif
}

VisitResult StepNode::create_child(const ParsingTree2<SymTok, LabTok> &sent)
{
#ifdef LOG_UCT
    visit_log() << "Spawning a child for " << this->uct.lock()->get_toolbox().print_sentence(sent, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << std::endl;
#endif
    // Check that we don't have the same sentence of an ancestor
    std::shared_ptr< SentenceNode > parent_sent = this->parent.lock();
    while (true) {
        if (!parent_sent) {
            break;
        }
        if (parent_sent->get_sentence() == sent) {
#ifdef LOG_UCT
            visit_log() << "New child coincides with one ancestor, dying..." << std::endl;
#endif
            return DEAD;
        }
        std::shared_ptr< StepNode > parent_step = parent_sent->get_parent().lock();
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

    std::set< LabTok > new_vars;
    std::tie(this->unconst_subst_map, new_vars) = tb.build_refreshing_full_subst_map2(tb.get_assertion_unconst_vars()[this->label.val()], strong_uct->get_temp_allocator());
    for (const auto &new_var : new_vars) {
        bool res;
        std::tie(std::ignore, res) = this->open_vars.insert(std::make_pair(new_var, this->weak_from_this()));
        assert(res);
#ifdef NDEBUG
        (void) res;
#endif
    }
    auto full_subst_map = update2(this->const_subst_map, this->unconst_subst_map, true);
    const Assertion &ass = tb.get_assertion(this->label);
    assert(ass.is_valid());
    for (auto hyp_tok : ass.get_ess_hyps()) {
        auto subst_hyp = substitute2(tb.get_parsed_sent2(hyp_tok), tb.get_standard_is_var(), full_subst_map);
        VisitResult res = this->create_child(subst_hyp);
        assert(res != PROVED);
        if (res == DEAD) {
            this->children.clear();
            this->exhausted = true;
            return DEAD;
        }
    }
    if (this->children.empty()) {
#ifdef LOG_UCT
        visit_log() << "No children, so nothing to prove!" << std::endl;
#endif
        this->exhausted = true;
        return PROVED;
    }
#ifdef LOG_UCT
    visit_log() << "Visiting each child for the first time" << std::endl;
#endif
    this->active_children = this->children;
    for (size_t i = 0; i < this->children.size(); i++) {
        // Do the first visit backwards, so that if some child is immediately evicted because it is trivial there is no problem
        VisitResult res = this->visit_child(this->children.size() - i - 1);
        if (res == DEAD || res == PROVED) {
            return res;
        }
    }
    return CONTINUE;
}

VisitResult StepNode::visit_child(size_t i)
{
    VisitResult res = this->active_children[i]->visit();
    if (res == PROVED) {
#ifdef LOG_UCT
        visit_log() << "We found a proof for a child!" << std::endl;
#endif
        this->active_children.erase(this->active_children.begin() + static_cast< std::iterator_traits<decltype(this->active_children)::iterator>::difference_type >(i));
        if (this->active_children.empty()) {
#ifdef LOG_UCT
            visit_log() << "All children finally proved!" << std::endl;
#endif
            this->exhausted = true;
            return PROVED;
        }
    } else if (res == DEAD) {
        this->exhausted = true;
        return DEAD;
    }
    this->worst_child = static_cast< size_t >(min_element(this->active_children.begin(), this->active_children.end(), [](auto &a, auto &b) { return a->get_value() < b->get_value(); }) - this->active_children.begin());
    return CONTINUE;
}

ParsingTree2< SymTok, LabTok > string_to_pt2(std::string sent_str, const LibraryToolbox &tb) {
    auto sent = tb.read_sentence(sent_str);
    auto thesis_pt = tb.parse_sentence(sent.begin()+1, sent.end(), tb.get_turnstile_alias());
    auto thesis_pt2 = pt_to_pt2(thesis_pt);
    return thesis_pt2;
}

std::vector< std::pair< ParsingTree2< SymTok, LabTok >, std::vector< ParsingTree2< SymTok, LabTok > > > > parse_tests(const LibraryToolbox &tb) {
    boost::filesystem::ifstream fin(platform_get_resources_base() / "tests.txt");
    std::string line;
    bool found_first = false;
    ParsingTree2< SymTok, LabTok > current_thesis;
    std::vector< ParsingTree2< SymTok, LabTok > > current_hyps;
    std::vector< std::pair< ParsingTree2< SymTok, LabTok >, std::vector< ParsingTree2< SymTok, LabTok > > > > problems;
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
                problems.push_back(std::make_pair(current_thesis, current_hyps));
            }
            found_first = true;
            current_hyps.clear();
            current_thesis = string_to_pt2(line, tb);
        }
    }
    if (found_first) {
        problems.push_back(std::make_pair(current_thesis, current_hyps));
    }
    return problems;
}

int uct_main(int argc, char *argv[]) {
    auto &data = get_set_mm();
    //auto &lib = data.lib;
    auto &tb = data.tb;

    size_t pb_idx = 0;
    if (argc == 2) {
        pb_idx = static_cast< size_t >(atoi(argv[1]));
    }

    auto problems = parse_tests(tb);
    auto problem = problems.at(pb_idx);

    std::cout << "Trying to prove thesis: " << tb.print_sentence(problem.first, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << std::endl;
    std::cout << "with hypotheses:" << std::endl;
    for (const auto &hyp : problem.second) {
        std::cout << " * " << tb.print_sentence(hyp, SentencePrinter::STYLE_ANSI_COLORS_SET_MM) << std::endl;
    }
    std::cout << std::endl;

    auto prover = UCTProver::create(tb, problem.first, problem.second);
    for (int i = 0; i < 50000; i++) {
        if (i % 2500 == 0) {
            std::cout << i << " visits done" << std::endl;
        }
        VisitResult res = prover->visit();
#ifdef LOG_UCT
        visit_log() << std::endl;
#endif
        if (res == PROVED) {
            std::cout << "Found proof after " << i+1 << " visits:";
            CreativeProofEngineImpl< Sentence > engine(tb, false);
            std::vector< std::function< void() > > children_cb;
            for (const auto &hyp : problem.second) {
                LabTok hyp_lab = engine.create_new_hypothesis(tb.reconstruct_sentence(pt2_to_pt(hyp), tb.get_turnstile()));
                children_cb.emplace_back([hyp_lab,&engine]() {
                    engine.process_label(hyp_lab);
                });
            }
            prover->set_children_callbacks(std::move(children_cb));
            try {
                prover->replay_proof(engine);
            } catch (ProofException< Sentence > &pe) {
                std::cout << "Failed with exception:" << std::endl;
                tb.dump_proof_exception(pe, std::cout);
            }
            const auto &labels = engine.get_proof_labels();
            for (const auto &label : labels) {
                if (label != LabTok{}) {
                    std::cout << " " << tb.resolve_label(label);
                } else {
                    std::cout << " *";
                }
            }
            std::cout << std::endl;
            break;
        } else if (res == DEAD) {
#ifdef LOG_UCT
            visit_log() << "The node is dead, the search has failed..." << std::endl;
#endif
            break;
        }
        //while (std::cin.get() != '\n') {}
    }

    return 0;
}
gio_static_block {
    gio::register_main_function("uct", uct_main);
}
