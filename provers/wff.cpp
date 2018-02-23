
#include "wff.h"
#include "mm/ptengine.h"

using namespace std;

#define LOG_WFF

Wff::~Wff()
{
}

ParsingTree2<SymTok, LabTok> Wff::to_parsing_tree(const LibraryToolbox &tb) const
{
    auto type_prover = this->get_type_prover(tb);
    CreativeProofEngineImpl< ParsingTree2< SymTok, LabTok > > engine(tb, false);
    bool res = type_prover(engine);
    assert(res);
    return engine.get_stack().back();
}

Prover<CheckpointedProofEngine> Wff::get_truth_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

bool Wff::is_true() const
{
    return false;
}

Prover<CheckpointedProofEngine> Wff::get_falsity_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

bool Wff::is_false() const
{
    return false;
}

Prover<CheckpointedProofEngine> Wff::get_type_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

Prover<CheckpointedProofEngine> Wff::get_imp_not_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

Prover< CheckpointedProofEngine > Wff::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    (void) var;
    (void) positive;
    (void) tb;
    return null_prover;
}

const RegisteredProver Wff::adv_truth_1_rp = LibraryToolbox::register_prover({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )");
const RegisteredProver Wff::adv_truth_2_rp = LibraryToolbox::register_prover({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )");
const RegisteredProver Wff::adv_truth_3_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( -. ph -> ps )"}, "|- ps");
std::pair<bool, Prover<CheckpointedProofEngine> > Wff::adv_truth_internal(pvar_set::iterator cur_var, pvar_set::iterator end_var, const LibraryToolbox &tb) const {
    if (cur_var == end_var) {
        return make_pair(this->is_true(), this->get_truth_prover(tb));
    } else {
        const auto &var = *cur_var++;
        pwff pos_wff = this->subst(var, true);
        pwff neg_wff = this->subst(var, false);
        pwff pos_antecent = var;
        pwff neg_antecent = Not::create(var);
        auto rec_pos_prover = pos_wff->adv_truth_internal(cur_var, end_var, tb);
        auto rec_neg_prover = neg_wff->adv_truth_internal(cur_var, end_var, tb);
        if (!rec_pos_prover.first || !rec_neg_prover.first) {
            return make_pair(false, null_prover);
        }
        auto pos_prover = this->get_subst_prover(var, true, tb);
        auto neg_prover = this->get_subst_prover(var, false, tb);
        auto pos_mp_prover = tb.build_registered_prover< CheckpointedProofEngine >(Wff::adv_truth_1_rp,
            {{"ph", pos_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}, {"ch", pos_wff->get_type_prover(tb)}},
            {rec_pos_prover.second, pos_prover});
        auto neg_mp_prover = tb.build_registered_prover< CheckpointedProofEngine >(Wff::adv_truth_2_rp,
            {{"ph", neg_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}, {"ch", neg_wff->get_type_prover(tb)}},
            {rec_neg_prover.second, neg_prover});
        auto final = tb.build_registered_prover< CheckpointedProofEngine >(Wff::adv_truth_3_rp,
            {{"ph", pos_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}},
            {pos_mp_prover, neg_mp_prover});
        return make_pair(true, final);
    }
}

const RegisteredProver Wff::adv_truth_4_rp = LibraryToolbox::register_prover({"|- ps", "|- ( ph <-> ps )"}, "|- ph");
std::pair<bool, Prover<CheckpointedProofEngine> > Wff::get_adv_truth_prover(const LibraryToolbox &tb) const
{
#ifdef LOG_WFF
    cerr << "WFF proving: " << this->to_string() << endl;
#endif
    pwff not_imp = this->imp_not_form();
#ifdef LOG_WFF
    cerr << "not_imp form: " << not_imp->to_string() << endl;
#endif
    pvar_set vars;
    this->get_variables(vars);
    auto real = not_imp->adv_truth_internal(vars.begin(), vars.end(), tb);
    if (!real.first) {
        return make_pair(false, null_prover);
    }
    auto equiv = this->get_imp_not_prover(tb);
    auto final = tb.build_registered_prover< CheckpointedProofEngine >(Wff::adv_truth_4_rp, {{"ph", this->get_type_prover(tb)}, {"ps", not_imp->get_type_prover(tb)}}, {real.second, equiv});
    return make_pair(true, final);
}

pvar Wff::get_tseitin_var(const LibraryToolbox &tb) const
{
    return Var::create(this->to_parsing_tree(tb), tb);
}

std::pair<CNFProblem, pvar_map<uint32_t> > Wff::get_tseitin_cnf_problem(const LibraryToolbox &tb) const
{
    CNForm cnf;
    this->get_tseitin_form(cnf, tb);
    cnf.insert({{true, this->get_tseitin_var(tb)}});
    auto vars = collect_tseitin_vars(cnf);
    auto map = build_tseitin_map(vars);
    auto dimacs = build_cnf_problem(cnf, map);
    return make_pair(dimacs, map);
}

True::True() {
}

string True::to_string() const {
    return "T.";
}

pwff True::imp_not_form() const {
    return True::create();
}

pwff True::subst(pvar var, bool positive) const
{
    (void) var;
    (void) positive;
    return this->shared_from_this();
}

void True::get_variables(pvar_set &vars) const
{
    (void) vars;
}

const RegisteredProver True::truth_rp = LibraryToolbox::register_prover({}, "|- T.");
Prover<CheckpointedProofEngine> True::get_truth_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(True::truth_rp, {}, {});
}

bool True::is_true() const
{
    return true;
}

const RegisteredProver True::type_rp = LibraryToolbox::register_prover({}, "wff T.");
Prover<CheckpointedProofEngine> True::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(True::type_rp, {}, {});
}

const RegisteredProver True::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( T. <-> T. )");
Prover<CheckpointedProofEngine> True::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(True::imp_not_rp, {}, {});
}

const RegisteredProver True::subst_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( T. <-> T. ) )");
Prover<CheckpointedProofEngine> True::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    pwff subst = var;
    if (!positive) {
        subst = Not::create(subst);
    }
    return tb.build_registered_prover< CheckpointedProofEngine >(True::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
}

bool True::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const True* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return true;
    }
}

void True::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    cnf.insert({{true, this->get_tseitin_var(tb)}});
}

False::False() {
}

string False::to_string() const {
    return "F.";
}

pwff False::imp_not_form() const {
    return False::create();
}

pwff False::subst(pvar var, bool positive) const
{
    (void) var;
    (void) positive;
    return this->shared_from_this();
}

void False::get_variables(pvar_set &vars) const
{
    (void) vars;
}

const RegisteredProver False::falsity_rp = LibraryToolbox::register_prover({}, "|- -. F.");
Prover<CheckpointedProofEngine> False::get_falsity_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(False::falsity_rp, {}, {});
}

bool False::is_false() const
{
    return true;
}

const RegisteredProver False::type_rp = LibraryToolbox::register_prover({}, "wff F.");
Prover<CheckpointedProofEngine> False::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(False::type_rp, {}, {});
}

const RegisteredProver False::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( F. <-> F. )");
Prover<CheckpointedProofEngine> False::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(False::imp_not_rp, {}, {});
}

const RegisteredProver False::subst_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( F. <-> F. ) )");
Prover<CheckpointedProofEngine> False::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    pwff subst = var;
    if (!positive) {
        subst = Not::create(subst);
    }
    return tb.build_registered_prover< CheckpointedProofEngine >(False::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
}

bool False::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const False* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return true;
    }
}

void False::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    cnf.insert({{false, this->get_tseitin_var(tb)}});
}

Var::Var(NameType name, string string_repr) :
    name(name), string_repr(string_repr) {
}

static auto var_cons_helper(const string &string_repr, const LibraryToolbox &tb) {
    auto sent = tb.read_sentence(string_repr);
    auto pt = tb.parse_sentence(sent.begin(), sent.end(), tb.get_turnstile_alias());
    return pt_to_pt2(pt);
}

Var::Var(const string &string_repr, const LibraryToolbox &tb) :
    name(var_cons_helper(string_repr, tb)), string_repr(string_repr) {
}

Var::Var(const Var::NameType &name, const LibraryToolbox &tb) :
    name(name), string_repr(tb.print_sentence(tb.reconstruct_sentence(pt2_to_pt(name))).to_string()) {
}

string Var::to_string() const {
    return this->string_repr;
}

pwff Var::imp_not_form() const {
    return this->shared_from_this();
}

pwff Var::subst(pvar var, bool positive) const
{
    if (*this == *var) {
        if (positive) {
            return True::create();
        } else {
            return False::create();
        }
    } else {
        return this->shared_from_this();
    }
}

void Var::get_variables(pvar_set &vars) const
{
    vars.insert(this->shared_from_this());
}

Prover<CheckpointedProofEngine> Var::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_type_prover(this->name);
    /*auto label = tb.get_var_sym_to_lab(tb.get_symbol(this->name));
    return [label](AbstractCheckpointedProofEngine &engine) {
        engine.process_label(label);
        return true;
    };*/
}

const RegisteredProver Var::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( ph <-> ph )");
Prover<CheckpointedProofEngine> Var::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Var::imp_not_rp, {{"ph", this->get_type_prover(tb)}}, {});
}

const RegisteredProver Var::subst_pos_1_rp = LibraryToolbox::register_prover({}, "|- ( T. -> ( ph -> ( T. <-> ph ) ) )");
const RegisteredProver Var::subst_pos_2_rp = LibraryToolbox::register_prover({"|- T.", "|- ( T. -> ( ph -> ( T. <-> ph ) ) )"}, "|- ( ph -> ( T. <-> ph ) )");
const RegisteredProver Var::subst_pos_3_rp = LibraryToolbox::register_prover({"|- ( ph -> ( T. <-> ph ) )"}, "|- ( ph -> ( ph <-> T. ) )");
const RegisteredProver Var::subst_pos_truth_rp = LibraryToolbox::register_prover({}, "|- T.");

const RegisteredProver Var::subst_neg_1_rp = LibraryToolbox::register_prover({}, "|- ( -. F. -> ( -. ph -> ( F. <-> ph ) ) )");
const RegisteredProver Var::subst_neg_2_rp = LibraryToolbox::register_prover({"|- -. F.", "|- ( -. F. -> ( -. ph -> ( F. <-> ph ) ) )"}, "|- ( -. ph -> ( F. <-> ph ) )");
const RegisteredProver Var::subst_neg_3_rp = LibraryToolbox::register_prover({"|- ( -. ph -> ( F. <-> ph ) )"}, "|- ( -. ph -> ( ph <-> F. ) )");
const RegisteredProver Var::subst_neg_falsity_rp = LibraryToolbox::register_prover({}, "|- -. F.");

const RegisteredProver Var::subst_indep_rp = LibraryToolbox::register_prover({}, "|- ( ps -> ( ph <-> ph ) )");
Prover<CheckpointedProofEngine> Var::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    if (*this == *var) {
        if (positive) {
            auto first = tb.build_registered_prover< CheckpointedProofEngine >(Var::subst_pos_1_rp, {{"ph", this->get_type_prover(tb)}}, {});
            auto truth = tb.build_registered_prover< CheckpointedProofEngine >(Var::subst_pos_truth_rp, {}, {});
            auto second = tb.build_registered_prover< CheckpointedProofEngine >(Var::subst_pos_2_rp, {{"ph", this->get_type_prover(tb)}}, {truth, first});
            auto third = tb.build_registered_prover< CheckpointedProofEngine >(Var::subst_pos_3_rp, {{"ph", this->get_type_prover(tb)}}, {second});
            return third;
        } else {
            auto first = tb.build_registered_prover< CheckpointedProofEngine >(Var::subst_neg_1_rp, {{"ph", this->get_type_prover(tb)}}, {});
            auto falsity = tb.build_registered_prover< CheckpointedProofEngine >(Var::subst_neg_falsity_rp, {}, {});
            auto second = tb.build_registered_prover< CheckpointedProofEngine >(Var::subst_neg_2_rp, {{"ph", this->get_type_prover(tb)}}, {falsity, first});
            auto third = tb.build_registered_prover< CheckpointedProofEngine >(Var::subst_neg_3_rp, {{"ph", this->get_type_prover(tb)}}, {second});
            return third;
        }
    } else {
        pwff ant = var;
        if (!positive) {
            ant = Not::create(ant);
        }
        return tb.build_registered_prover< CheckpointedProofEngine >(Var::subst_indep_rp, {{"ps", ant->get_type_prover(tb)}, {"ph", this->get_type_prover(tb)}}, {});
    }
}

bool Var::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const Var* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return this->get_name() == px->get_name();
    }
}

bool Var::operator<(const Var &x) const
{
    return this->get_name() < x.get_name();
}

void Var::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    (void) cnf;
    (void) tb;
}

Not::Not(pwff a) :
    a(a) {
}

string Not::to_string() const {
    return "-. " + this->a->to_string();
}

pwff Not::imp_not_form() const {
    return Not::create(this->a->imp_not_form());
}

pwff Not::subst(pvar var, bool positive) const
{
    return Not::create(this->a->subst(var, positive));
}

void Not::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
}

Prover<CheckpointedProofEngine> Not::get_truth_prover(const LibraryToolbox &tb) const
{
    return this->a->get_falsity_prover(tb);
}

bool Not::is_true() const
{
    return this->a->is_false();
}

const RegisteredProver Not::falsity_rp = LibraryToolbox::register_prover({ "|- ph" }, "|- -. -. ph");
Prover<CheckpointedProofEngine> Not::get_falsity_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Not::falsity_rp, {{ "ph", this->a->get_type_prover(tb) }}, { this->a->get_truth_prover(tb) });
}

bool Not::is_false() const
{
    return this->a->is_true();
}

const RegisteredProver Not::type_rp = LibraryToolbox::register_prover({}, "wff -. ph");
Prover<CheckpointedProofEngine> Not::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Not::type_rp, {{ "ph", this->a->get_type_prover(tb) }}, {});
}

const RegisteredProver Not::imp_not_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )"}, "|- ( -. ph <-> -. ps )");
Prover<CheckpointedProofEngine> Not::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Not::imp_not_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }}, { this->a->get_imp_not_prover(tb) });
}

const RegisteredProver Not::subst_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ( -. ps <-> -. ch ) )");
Prover<CheckpointedProofEngine> Not::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    pwff ant;
    if (positive) {
        ant = var;
    } else {
        ant = Not::create(var);
    }
    return tb.build_registered_prover< CheckpointedProofEngine >(Not::subst_rp, {{"ph", ant->get_type_prover(tb)}, {"ps", this->a->get_type_prover(tb)}, {"ch", this->a->subst(var, positive)->get_type_prover(tb)}}, {this->a->get_subst_prover(var, positive, tb)});
}

bool Not::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const Not* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return *this->get_a() == *px->get_a();
    }
}

void Not::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    this->get_a()->get_tseitin_form(cnf, tb);
    cnf.insert({{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
}

Imp::Imp(pwff a, pwff b) :
    a(a), b(b) {
}

string Imp::to_string() const {
    return "( " + this->a->to_string() + " -> " + this->b->to_string() + " )";
}

pwff Imp::imp_not_form() const {
    return Imp::create(this->a->imp_not_form(), this->b->imp_not_form());
}

pwff Imp::subst(pvar var, bool positive) const
{
    return Imp::create(this->a->subst(var, positive), this->b->subst(var, positive));
}

void Imp::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

const RegisteredProver Imp::truth_1_rp = LibraryToolbox::register_prover({ "|- ps" }, "|- ( ph -> ps )");
const RegisteredProver Imp::truth_2_rp = LibraryToolbox::register_prover({ "|- -. ph" }, "|- ( ph -> ps )");
Prover<CheckpointedProofEngine> Imp::get_truth_prover(const LibraryToolbox &tb) const
{
    if (this->b->is_true()) {
        return tb.build_registered_prover< CheckpointedProofEngine >(Imp::truth_1_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, { this->b->get_truth_prover(tb) });
    }
    if (this->a->is_false()) {
        return tb.build_registered_prover< CheckpointedProofEngine >(Imp::truth_2_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, { this->a->get_falsity_prover(tb) });
    }
    return null_prover;
    //return cascade_provers(first_prover, second_prover);
}

bool Imp::is_true() const
{
    return this->b->is_true() || this->a->is_false();
}

const RegisteredProver Imp::falsity_1_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )");
const RegisteredProver Imp::falsity_2_rp = LibraryToolbox::register_prover({ "|- ph", "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )"}, "|- ( -. ps -> -. ( ph -> ps ) )");
const RegisteredProver Imp::falsity_3_rp = LibraryToolbox::register_prover({ "|- ( -. ps -> -. ( ph -> ps ) )", "|- -. ps"}, "|- -. ( ph -> ps )");
Prover<CheckpointedProofEngine> Imp::get_falsity_prover(const LibraryToolbox &tb) const
{
    auto theorem_prover = tb.build_registered_prover< CheckpointedProofEngine >(Imp::falsity_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto mp_prover1 = tb.build_registered_prover< CheckpointedProofEngine >(Imp::falsity_2_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { this->a->get_truth_prover(tb), theorem_prover });
    auto mp_prover2 = tb.build_registered_prover< CheckpointedProofEngine >(Imp::falsity_3_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { mp_prover1, this->b->get_falsity_prover(tb) });
    return mp_prover2;
}

bool Imp::is_false() const
{
    return this->a->is_true() && this->b->is_false();
}

const RegisteredProver Imp::type_rp = LibraryToolbox::register_prover({}, "wff ( ph -> ps )");
Prover<CheckpointedProofEngine> Imp::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Imp::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

const RegisteredProver Imp::imp_not_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ch <-> th )"}, "|- ( ( ph -> ch ) <-> ( ps -> th ) )");
Prover<CheckpointedProofEngine> Imp::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Imp::imp_not_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }, {"ch", this->b->get_type_prover(tb)}, {"th", this->b->imp_not_form()->get_type_prover(tb)}},
    { this->a->get_imp_not_prover(tb), this->b->get_imp_not_prover(tb) });
}

const RegisteredProver Imp::subst_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps -> th ) <-> ( ch -> ta ) ) )");
Prover<CheckpointedProofEngine> Imp::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    pwff ant;
    if (positive) {
        ant = var;
    } else {
        ant = Not::create(var);
    }
    return tb.build_registered_prover< CheckpointedProofEngine >(Imp::subst_rp,
        {{"ph", ant->get_type_prover(tb)}, {"ps", this->a->get_type_prover(tb)}, {"ch", this->a->subst(var, positive)->get_type_prover(tb)}, {"th", this->b->get_type_prover(tb)}, {"ta", this->b->subst(var, positive)->get_type_prover(tb)}},
    {this->a->get_subst_prover(var, positive, tb), this->b->get_subst_prover(var, positive, tb)});
}

const RegisteredProver Imp::mp_rp = LibraryToolbox::register_prover({"|- ph", "|- ( ph -> ps )"}, "|- ps");
Prover<CheckpointedProofEngine> Imp::get_mp_prover(Prover<CheckpointedProofEngine> ant_prover, Prover<CheckpointedProofEngine> this_prover, const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Imp::mp_rp, { {"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { ant_prover, this_prover });
}

bool Imp::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const Imp* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

void Imp::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    this->get_a()->get_tseitin_form(cnf, tb);
    this->get_b()->get_tseitin_form(cnf, tb);
    cnf.insert({{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
    cnf.insert({{false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
}

Biimp::Biimp(pwff a, pwff b) :
    a(a), b(b) {
}

string Biimp::to_string() const {
    return "( " + this->a->to_string() + " <-> " + this->b->to_string() + " )";
}

pwff Biimp::imp_not_form() const {
    auto ain = this->a->imp_not_form();
    auto bin = this->b->imp_not_form();
    return Not::create(Imp::create(Imp::create(ain, bin), Not::create(Imp::create(bin, ain))));
}

pwff Biimp::half_imp_not_form() const
{
    auto ain = this->a;
    auto bin = this->b;
    return Not::create(Imp::create(Imp::create(ain, bin), Not::create(Imp::create(bin, ain))));
}

void Biimp::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

const RegisteredProver Biimp::type_rp = LibraryToolbox::register_prover({}, "wff ( ph <-> ps )");
Prover<CheckpointedProofEngine> Biimp::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Biimp::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

const RegisteredProver Biimp::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph <-> ps ) <-> -. ( ( ph -> ps ) -> -. ( ps -> ph ) ) )");
const RegisteredProver Biimp::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<CheckpointedProofEngine> Biimp::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(Biimp::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(Biimp::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

bool Biimp::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const Biimp* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

void Biimp::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    this->get_a()->get_tseitin_form(cnf, tb);
    this->get_b()->get_tseitin_form(cnf, tb);
    cnf.insert({{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
    cnf.insert({{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
}

Xor::Xor(pwff a, pwff b) :
    a(a), b(b) {
}

string Xor::to_string() const {
    return "( " + this->a->to_string() + " \\/_ " + this->b->to_string() + " )";
}

pwff Xor::imp_not_form() const {
    return Not::create(Biimp::create(this->a->imp_not_form(), this->b->imp_not_form()))->imp_not_form();
}

pwff Xor::half_imp_not_form() const
{
    return Not::create(Biimp::create(this->a, this->b));
}

void Xor::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

const RegisteredProver Xor::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/_ ps )");
Prover<CheckpointedProofEngine> Xor::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Xor::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

const RegisteredProver Xor::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/_ ps ) <-> -. ( ph <-> ps ) )");
const RegisteredProver Xor::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<CheckpointedProofEngine> Xor::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(Xor::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(Xor::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

bool Xor::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const Xor* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

void Xor::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    this->get_a()->get_tseitin_form(cnf, tb);
    this->get_b()->get_tseitin_form(cnf, tb);
    cnf.insert({{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
    cnf.insert({{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
}

Nand::Nand(pwff a, pwff b) :
    a(a), b(b) {
}

string Nand::to_string() const {
    return "( " + this->a->to_string() + " -/\\ " + this->b->to_string() + " )";
}

pwff Nand::imp_not_form() const {
    return Not::create(And::create(this->a->imp_not_form(), this->b->imp_not_form()))->imp_not_form();
}

pwff Nand::half_imp_not_form() const
{
    return Not::create(And::create(this->a, this->b));
}

void Nand::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

const RegisteredProver Nand::type_rp = LibraryToolbox::register_prover({}, "wff ( ph -/\\ ps )");
Prover<CheckpointedProofEngine> Nand::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Nand::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

const RegisteredProver Nand::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph -/\\ ps ) <-> -. ( ph /\\ ps ) )");
const RegisteredProver Nand::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<CheckpointedProofEngine> Nand::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(Nand::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(Nand::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

bool Nand::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const Nand* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

void Nand::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    this->get_a()->get_tseitin_form(cnf, tb);
    this->get_b()->get_tseitin_form(cnf, tb);
    cnf.insert({{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
}

Or::Or(pwff a, pwff b) :
    a(a), b(b) {
}

string Or::to_string() const {
    return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " )";
}

pwff Or::imp_not_form() const {
    return Imp::create(Not::create(this->a->imp_not_form()), this->b->imp_not_form());
}

pwff Or::half_imp_not_form() const
{
    return Imp::create(Not::create(this->a), this->b);
}

void Or::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

const RegisteredProver Or::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/ ps )");
Prover<CheckpointedProofEngine> Or::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Or::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

const RegisteredProver Or::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/ ps ) <-> ( -. ph -> ps ) )");
const RegisteredProver Or::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<CheckpointedProofEngine> Or::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(Or::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(Or::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

bool Or::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const Or* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

void Or::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    this->get_a()->get_tseitin_form(cnf, tb);
    this->get_b()->get_tseitin_form(cnf, tb);
    cnf.insert({{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
    cnf.insert({{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
    cnf.insert({{false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
}

And::And(pwff a, pwff b) :
    a(a), b(b) {
}

string And::to_string() const {
    return "( " + this->a->to_string() + " /\\ " + this->b->to_string() + " )";
}

pwff And::half_imp_not_form() const {
    return Not::create(Imp::create(this->a, Not::create(this->b)));
}

void And::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

pwff And::imp_not_form() const
{
    return Not::create(Imp::create(this->a->imp_not_form(), Not::create(this->b->imp_not_form())));
}

const RegisteredProver And::type_rp = LibraryToolbox::register_prover({}, "wff ( ph /\\ ps )");
Prover<CheckpointedProofEngine> And::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(And::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

const RegisteredProver And::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph /\\ ps ) <-> -. ( ph -> -. ps ) )");
const RegisteredProver And::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<CheckpointedProofEngine> And::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(And::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(And::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

bool And::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const And* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

void And::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    this->get_a()->get_tseitin_form(cnf, tb);
    this->get_b()->get_tseitin_form(cnf, tb);
    cnf.insert({{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
}

pwff ConvertibleWff::subst(pvar var, bool positive) const
{
    return this->imp_not_form()->subst(var, positive);
}

const RegisteredProver ConvertibleWff::truth_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- ph"}, "|- ps");
Prover<CheckpointedProofEngine> ConvertibleWff::get_truth_prover(const LibraryToolbox &tb) const
{
    //return null_prover;
    return tb.build_registered_prover< CheckpointedProofEngine >(ConvertibleWff::truth_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_truth_prover(tb)});
}

bool ConvertibleWff::is_true() const
{
    return this->imp_not_form()->is_true();
}

const RegisteredProver ConvertibleWff::falsity_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- -. ph"}, "|- -. ps");
Prover<CheckpointedProofEngine> ConvertibleWff::get_falsity_prover(const LibraryToolbox &tb) const
{
    //return null_prover;
    return tb.build_registered_prover< CheckpointedProofEngine >(ConvertibleWff::falsity_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_falsity_prover(tb)});
}

bool ConvertibleWff::is_false() const
{
    return this->imp_not_form()->is_false();
}

/*Prover<AbstractCheckpointedProofEngine> ConvertibleWff::get_type_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    // Disabled, because ordinarily I do not want to use this generic and probably inefficient method
    return null_prover;
    //return tb.build_type_prover< AbstractCheckpointedProofEngine >(this->to_wff_sentence(tb));
}*/

And3::And3(pwff a, pwff b, pwff c) :
    a(a), b(b), c(c)
{
}

string And3::to_string() const
{
    return "( " + this->a->to_string() + " /\\ " + this->b->to_string() + " /\\ " + this->c->to_string() + " )";
}

pwff And3::imp_not_form() const
{
    return And::create(And::create(this->a, this->b), this->c)->imp_not_form();
}

pwff And3::half_imp_not_form() const
{
    return And::create(And::create(this->a, this->b), this->c);
}

void And3::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
    this->c->get_variables(vars);
}

const RegisteredProver And3::type_rp = LibraryToolbox::register_prover({}, "wff ( ph /\\ ps /\\ ch )");
Prover<CheckpointedProofEngine> And3::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(And3::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
}

const RegisteredProver And3::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph /\\ ps /\\ ch ) <-> ( ( ph /\\ ps ) /\\ ch ) )");
const RegisteredProver And3::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<CheckpointedProofEngine> And3::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(And3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(And3::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

bool And3::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const And3* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b() && *this->get_c() == *px->get_c();
    }
}

void And3::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    this->get_a()->get_tseitin_form(cnf, tb);
    this->get_b()->get_tseitin_form(cnf, tb);
    this->get_c()->get_tseitin_form(cnf, tb);

    auto intermediate = And::create(this->get_a(), this->get_b());

    // intermediate = ( a /\ b )
    cnf.insert({{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, intermediate->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_a()->get_tseitin_var(tb)}, {false, intermediate->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_b()->get_tseitin_var(tb)}, {false, intermediate->get_tseitin_var(tb)}});

    // this = ( intermediate /\ c )
    cnf.insert({{false, intermediate->get_tseitin_var(tb)}, {false, this->get_c()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
    cnf.insert({{true, intermediate->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
    cnf.insert({{true, this->get_c()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
}

Or3::Or3(pwff a, pwff b, pwff c) :
    a(a), b(b), c(c)
{
}

string Or3::to_string() const
{
    return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " \\/ " + this->c->to_string() + " )";
}

pwff Or3::imp_not_form() const
{
    return Or::create(Or::create(this->a, this->b), this->c)->imp_not_form();
}

pwff Or3::half_imp_not_form() const
{
    return Or::create(Or::create(this->a, this->b), this->c);
}

void Or3::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
    this->c->get_variables(vars);
}

const RegisteredProver Or3::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/ ps \\/ ch )");
Prover<CheckpointedProofEngine> Or3::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(Or3::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
}

const RegisteredProver Or3::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/ ps \\/ ch ) <-> ( ( ph \\/ ps ) \\/ ch ) )");
const RegisteredProver Or3::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<CheckpointedProofEngine> Or3::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(Or3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(Or3::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

void Or3::get_tseitin_form(CNForm &cnf, const LibraryToolbox &tb) const
{
    this->get_a()->get_tseitin_form(cnf, tb);
    this->get_b()->get_tseitin_form(cnf, tb);
    this->get_c()->get_tseitin_form(cnf, tb);

    auto intermediate = Or::create(this->get_a(), this->get_b());

    // intermediate = ( a \/ b )
    cnf.insert({{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, intermediate->get_tseitin_var(tb)}});
    cnf.insert({{false, this->get_a()->get_tseitin_var(tb)}, {true, intermediate->get_tseitin_var(tb)}});
    cnf.insert({{false, this->get_b()->get_tseitin_var(tb)}, {true, intermediate->get_tseitin_var(tb)}});

    // this = ( intermediate \/ c )
    cnf.insert({{true, intermediate->get_tseitin_var(tb)}, {true, this->get_c()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}});
    cnf.insert({{false, intermediate->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
    cnf.insert({{false, this->get_c()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}});
}

bool Or3::operator==(const Wff &x) const
{
    auto px = dynamic_cast< const Or3* >(&x);
    if (px == NULL) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b() && *this->get_c() == *px->get_c();
    }
}

bool pvar_comp::operator()(const pvar &x, const pvar &y) const {
    return *x < *y;
}

pwff wff_from_pt(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb)
{
    assert(tb.resolve_symbol(pt.type) == "wff");
    if (pt.label == tb.get_registered_prover_label(True::type_rp)) {
        assert(pt.children.size() == 0);
        return True::create();
    } else if (pt.label == tb.get_registered_prover_label(False::type_rp)) {
        assert(pt.children.size() == 0);
        return False::create();
    } else if (pt.label == tb.get_registered_prover_label(Not::type_rp)) {
        assert(pt.children.size() == 1);
        return Not::create(wff_from_pt(pt.children[0], tb));
    } else if (pt.label == tb.get_registered_prover_label(Imp::type_rp)) {
        assert(pt.children.size() == 2);
        return Imp::create(wff_from_pt(pt.children[0], tb), wff_from_pt(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(Biimp::type_rp)) {
        assert(pt.children.size() == 2);
        return Biimp::create(wff_from_pt(pt.children[0], tb), wff_from_pt(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(And::type_rp)) {
        assert(pt.children.size() == 2);
        return And::create(wff_from_pt(pt.children[0], tb), wff_from_pt(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(Or::type_rp)) {
        assert(pt.children.size() == 2);
        return Or::create(wff_from_pt(pt.children[0], tb), wff_from_pt(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(Nand::type_rp)) {
        assert(pt.children.size() == 2);
        return Nand::create(wff_from_pt(pt.children[0], tb), wff_from_pt(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(Xor::type_rp)) {
        assert(pt.children.size() == 2);
        return Xor::create(wff_from_pt(pt.children[0], tb), wff_from_pt(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(And3::type_rp)) {
        assert(pt.children.size() == 3);
        return And3::create(wff_from_pt(pt.children[0], tb), wff_from_pt(pt.children[1], tb), wff_from_pt(pt.children[2], tb));
    } else if (pt.label == tb.get_registered_prover_label(Or3::type_rp)) {
        assert(pt.children.size() == 3);
        return Or3::create(wff_from_pt(pt.children[0], tb), wff_from_pt(pt.children[1], tb), wff_from_pt(pt.children[2], tb));
    } else {
        return Var::create(pt_to_pt2(pt), tb);
    }
}

pvar_set collect_tseitin_vars(const CNForm &cnf)
{
    pvar_set ret;
    for (const auto &x : cnf) {
        for (const auto &y : x) {
            ret.insert(y.second);
        }
    }
    return ret;
}

pvar_map<uint32_t> build_tseitin_map(const pvar_set &vars)
{
    pvar_map< uint32_t > ret;
    uint32_t idx = 0;
    for (const auto &var : vars) {
        ret[var] = idx;
        idx++;
    }
    return ret;
}

CNFProblem build_cnf_problem(const CNForm &cnf, const pvar_map<uint32_t> &var_map)
{
    CNFProblem ret{var_map.size(), {}};
    for (const auto &clause : cnf) {
        ret.clauses.emplace_back();
        for (const auto &term : clause) {
            ret.clauses.back().push_back(make_pair(term.first, var_map.at(term.second)));
        }
    }
    return ret;
}
