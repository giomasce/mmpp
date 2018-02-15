
#include "wff.h"
#include "mm/ptengine.h"

using namespace std;

Wff::~Wff()
{
}

// FIXME Terribly inefficient, but to_sentence() is inefficient too
/*Sentence Wff::to_asserted_sentence(const Library &lib) const
{
    auto ret = this->to_sentence(lib);
    ret.insert(ret.begin(), lib.get_symbol("|-"));
    return ret;
}

// FIXME Terribly inefficient, but to_sentence() is inefficient too
Sentence Wff::to_wff_sentence(const Library &lib) const
{
    auto ret = this->to_sentence(lib);
    ret.insert(ret.begin(), lib.get_symbol("wff"));
    return ret;
}*/

ParsingTree2<SymTok, LabTok> Wff::to_parsing_tree(const LibraryToolbox &tb) const
{
    auto type_prover = this->get_type_prover(tb);
    ExtendedProofEngine< ParsingTree2< SymTok, LabTok > > engine(tb, false);
    bool res = type_prover(engine);
    assert(res);
    return engine.get_stack().back();
}

Prover<AbstractCheckpointedProofEngine> Wff::get_truth_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

bool Wff::is_true() const
{
    return false;
}

Prover<AbstractCheckpointedProofEngine> Wff::get_falsity_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

bool Wff::is_false() const
{
    return false;
}

Prover<AbstractCheckpointedProofEngine> Wff::get_type_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

Prover<AbstractCheckpointedProofEngine> Wff::get_imp_not_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

Prover< AbstractCheckpointedProofEngine > Wff::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    (void) var;
    (void) positive;
    (void) tb;
    return null_prover;
}

RegisteredProver Wff::adv_truth_1_rp = LibraryToolbox::register_prover({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )");
RegisteredProver Wff::adv_truth_2_rp = LibraryToolbox::register_prover({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )");
RegisteredProver Wff::adv_truth_3_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( -. ph -> ps )"}, "|- ps");
Prover<AbstractCheckpointedProofEngine> Wff::adv_truth_internal(pvar_set::iterator cur_var, pvar_set::iterator end_var, const LibraryToolbox &tb) const {
    if (cur_var == end_var) {
        return this->get_truth_prover(tb);
    } else {
        const auto &var = *cur_var++;
        pwff pos_wff = this->subst(var, true);
        pwff neg_wff = this->subst(var, false);
        pwff pos_antecent = var;
        pwff neg_antecent = Not::create(var);
        auto rec_pos_prover = pos_wff->adv_truth_internal(cur_var, end_var, tb);
        auto rec_neg_prover = neg_wff->adv_truth_internal(cur_var, end_var, tb);
        auto pos_prover = this->get_subst_prover(var, true, tb);
        auto neg_prover = this->get_subst_prover(var, false, tb);
        auto pos_mp_prover = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Wff::adv_truth_1_rp,
            {{"ph", pos_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}, {"ch", pos_wff->get_type_prover(tb)}},
            {rec_pos_prover, pos_prover});
        auto neg_mp_prover = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Wff::adv_truth_2_rp,
            {{"ph", neg_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}, {"ch", neg_wff->get_type_prover(tb)}},
            {rec_neg_prover, neg_prover});
        auto final = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Wff::adv_truth_3_rp,
            {{"ph", pos_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}},
            {pos_mp_prover, neg_mp_prover});
        return final;
    }
}

RegisteredProver Wff::adv_truth_4_rp = LibraryToolbox::register_prover({"|- ps", "|- ( ph <-> ps )"}, "|- ph");
Prover<AbstractCheckpointedProofEngine> Wff::get_adv_truth_prover(const LibraryToolbox &tb) const
{
    pwff not_imp = this->imp_not_form();
    pvar_set vars;
    this->get_variables(vars);
    auto real = not_imp->adv_truth_internal(vars.begin(), vars.end(), tb);
    auto equiv = this->get_imp_not_prover(tb);
    auto final = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Wff::adv_truth_4_rp, {{"ph", this->get_type_prover(tb)}, {"ps", not_imp->get_type_prover(tb)}}, {real, equiv});
    return final;
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

std::vector<SymTok> True::to_sentence(const Library &lib) const
{
    return { lib.get_symbol("T.") };
}

void True::get_variables(pvar_set &vars) const
{
    (void) vars;
}

RegisteredProver True::truth_rp = LibraryToolbox::register_prover({}, "|- T.");
Prover<AbstractCheckpointedProofEngine> True::get_truth_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(True::truth_rp, {}, {});
}

bool True::is_true() const
{
    return true;
}

RegisteredProver True::type_rp = LibraryToolbox::register_prover({}, "wff T.");
Prover<AbstractCheckpointedProofEngine> True::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(True::type_rp, {}, {});
}

RegisteredProver True::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( T. <-> T. )");
Prover<AbstractCheckpointedProofEngine> True::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(True::imp_not_rp, {}, {});
}

RegisteredProver True::subst_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( T. <-> T. ) )");
Prover<AbstractCheckpointedProofEngine> True::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    pwff subst = var;
    if (!positive) {
        subst = Not::create(subst);
    }
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(True::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
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

/*std::vector<SymTok> False::to_sentence(const Library &lib) const
{
    return { lib.get_symbol("F.") };
}*/

void False::get_variables(pvar_set &vars) const
{
    (void) vars;
}

RegisteredProver False::falsity_rp = LibraryToolbox::register_prover({}, "|- -. F.");
Prover<AbstractCheckpointedProofEngine> False::get_falsity_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(False::falsity_rp, {}, {});
}

bool False::is_false() const
{
    return true;
}

RegisteredProver False::type_rp = LibraryToolbox::register_prover({}, "wff F.");
Prover<AbstractCheckpointedProofEngine> False::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(False::type_rp, {}, {});
}

RegisteredProver False::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( F. <-> F. )");
Prover<AbstractCheckpointedProofEngine> False::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(False::imp_not_rp, {}, {});
}

RegisteredProver False::subst_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( F. <-> F. ) )");
Prover<AbstractCheckpointedProofEngine> False::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    pwff subst = var;
    if (!positive) {
        subst = Not::create(subst);
    }
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(False::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
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

Var::Var(NameType name) :
    name(name), string_repr(name) {
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

/*std::vector<SymTok> Var::to_sentence(const Library &lib) const
{
    return { lib.get_symbol(this->name) };
}*/

void Var::get_variables(pvar_set &vars) const
{
    vars.insert(this->shared_from_this());
}

Prover<AbstractCheckpointedProofEngine> Var::get_type_prover(const LibraryToolbox &tb) const
{
    //return tb.build_type_prover< AbstractCheckpointedProofEngine >(this->to_wff_sentence(tb));
    auto label = tb.get_var_sym_to_lab(tb.get_symbol(this->name));
    return [label](AbstractCheckpointedProofEngine &engine) {
        engine.process_label(label);
        return true;
    };
}

RegisteredProver Var::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( ph <-> ph )");
Prover<AbstractCheckpointedProofEngine> Var::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Var::imp_not_rp, {{"ph", this->get_type_prover(tb)}}, {});
}

RegisteredProver Var::subst_pos_1_rp = LibraryToolbox::register_prover({}, "|- ( T. -> ( ph -> ( T. <-> ph ) ) )");
RegisteredProver Var::subst_pos_2_rp = LibraryToolbox::register_prover({"|- T.", "|- ( T. -> ( ph -> ( T. <-> ph ) ) )"}, "|- ( ph -> ( T. <-> ph ) )");
RegisteredProver Var::subst_pos_3_rp = LibraryToolbox::register_prover({"|- ( ph -> ( T. <-> ph ) )"}, "|- ( ph -> ( ph <-> T. ) )");
RegisteredProver Var::subst_pos_truth_rp = LibraryToolbox::register_prover({}, "|- T.");

RegisteredProver Var::subst_neg_1_rp = LibraryToolbox::register_prover({}, "|- ( -. F. -> ( -. ph -> ( F. <-> ph ) ) )");
RegisteredProver Var::subst_neg_2_rp = LibraryToolbox::register_prover({"|- -. F.", "|- ( -. F. -> ( -. ph -> ( F. <-> ph ) ) )"}, "|- ( -. ph -> ( F. <-> ph ) )");
RegisteredProver Var::subst_neg_3_rp = LibraryToolbox::register_prover({"|- ( -. ph -> ( F. <-> ph ) )"}, "|- ( -. ph -> ( ph <-> F. ) )");
RegisteredProver Var::subst_neg_falsity_rp = LibraryToolbox::register_prover({}, "|- -. F.");

RegisteredProver Var::subst_indep_rp = LibraryToolbox::register_prover({}, "|- ( ps -> ( ph <-> ph ) )");
Prover<AbstractCheckpointedProofEngine> Var::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    if (*this == *var) {
        if (positive) {
            auto first = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Var::subst_pos_1_rp, {{"ph", this->get_type_prover(tb)}}, {});
            auto truth = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Var::subst_pos_truth_rp, {}, {});
            auto second = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Var::subst_pos_2_rp, {{"ph", this->get_type_prover(tb)}}, {truth, first});
            auto third = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Var::subst_pos_3_rp, {{"ph", this->get_type_prover(tb)}}, {second});
            return third;
        } else {
            auto first = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Var::subst_neg_1_rp, {{"ph", this->get_type_prover(tb)}}, {});
            auto falsity = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Var::subst_neg_falsity_rp, {}, {});
            auto second = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Var::subst_neg_2_rp, {{"ph", this->get_type_prover(tb)}}, {falsity, first});
            auto third = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Var::subst_neg_3_rp, {{"ph", this->get_type_prover(tb)}}, {second});
            return third;
        }
    } else {
        pwff ant = var;
        if (!positive) {
            ant = Not::create(ant);
        }
        return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Var::subst_indep_rp, {{"ps", ant->get_type_prover(tb)}, {"ph", this->get_type_prover(tb)}}, {});
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

/*std::vector<SymTok> Not::to_sentence(const Library &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("-."));
    vector< SymTok > sent = this->a->to_sentence(lib);
    copy(sent.begin(), sent.end(), back_inserter(ret));
    return ret;
}*/

void Not::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
}

Prover<AbstractCheckpointedProofEngine> Not::get_truth_prover(const LibraryToolbox &tb) const
{
    return this->a->get_falsity_prover(tb);
}

bool Not::is_true() const
{
    return this->a->is_false();
}

RegisteredProver Not::falsity_rp = LibraryToolbox::register_prover({ "|- ph" }, "|- -. -. ph");
Prover<AbstractCheckpointedProofEngine> Not::get_falsity_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Not::falsity_rp, {{ "ph", this->a->get_type_prover(tb) }}, { this->a->get_truth_prover(tb) });
}

bool Not::is_false() const
{
    return this->a->is_true();
}

RegisteredProver Not::type_rp = LibraryToolbox::register_prover({}, "wff -. ph");
Prover<AbstractCheckpointedProofEngine> Not::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Not::type_rp, {{ "ph", this->a->get_type_prover(tb) }}, {});
}

RegisteredProver Not::imp_not_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )"}, "|- ( -. ph <-> -. ps )");
Prover<AbstractCheckpointedProofEngine> Not::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Not::imp_not_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }}, { this->a->get_imp_not_prover(tb) });
}

RegisteredProver Not::subst_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ( -. ps <-> -. ch ) )");
Prover<AbstractCheckpointedProofEngine> Not::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    pwff ant;
    if (positive) {
        ant = var;
    } else {
        ant = Not::create(var);
    }
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Not::subst_rp, {{"ph", ant->get_type_prover(tb)}, {"ps", this->a->get_type_prover(tb)}, {"ch", this->a->subst(var, positive)->get_type_prover(tb)}}, {this->a->get_subst_prover(var, positive, tb)});
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

/*std::vector<SymTok> Imp::to_sentence(const Library &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->b->to_sentence(lib);
    copy(senta.begin(), senta.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol("->"));
    copy(sentb.begin(), sentb.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol(")"));
    return ret;
}*/

void Imp::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

RegisteredProver Imp::truth_1_rp = LibraryToolbox::register_prover({ "|- ps" }, "|- ( ph -> ps )");
RegisteredProver Imp::truth_2_rp = LibraryToolbox::register_prover({ "|- -. ph" }, "|- ( ph -> ps )");
Prover<AbstractCheckpointedProofEngine> Imp::get_truth_prover(const LibraryToolbox &tb) const
{
    if (this->b->is_true()) {
        return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Imp::truth_1_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, { this->b->get_truth_prover(tb) });
    }
    if (this->a->is_false()) {
        return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Imp::truth_2_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, { this->a->get_falsity_prover(tb) });
    }
    return null_prover;
    //return cascade_provers(first_prover, second_prover);
}

bool Imp::is_true() const
{
    return this->b->is_true() || this->a->is_false();
}

RegisteredProver Imp::falsity_1_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )");
RegisteredProver Imp::falsity_2_rp = LibraryToolbox::register_prover({ "|- ph", "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )"}, "|- ( -. ps -> -. ( ph -> ps ) )");
RegisteredProver Imp::falsity_3_rp = LibraryToolbox::register_prover({ "|- ( -. ps -> -. ( ph -> ps ) )", "|- -. ps"}, "|- -. ( ph -> ps )");
Prover<AbstractCheckpointedProofEngine> Imp::get_falsity_prover(const LibraryToolbox &tb) const
{
    auto theorem_prover = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Imp::falsity_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto mp_prover1 = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Imp::falsity_2_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { this->a->get_truth_prover(tb), theorem_prover });
    auto mp_prover2 = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Imp::falsity_3_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { mp_prover1, this->b->get_falsity_prover(tb) });
    return mp_prover2;
}

bool Imp::is_false() const
{
    return this->a->is_true() && this->b->is_false();
}

RegisteredProver Imp::type_rp = LibraryToolbox::register_prover({}, "wff ( ph -> ps )");
Prover<AbstractCheckpointedProofEngine> Imp::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Imp::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver Imp::imp_not_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ch <-> th )"}, "|- ( ( ph -> ch ) <-> ( ps -> th ) )");
Prover<AbstractCheckpointedProofEngine> Imp::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Imp::imp_not_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }, {"ch", this->b->get_type_prover(tb)}, {"th", this->b->imp_not_form()->get_type_prover(tb)}},
    { this->a->get_imp_not_prover(tb), this->b->get_imp_not_prover(tb) });
}

RegisteredProver Imp::subst_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps -> th ) <-> ( ch -> ta ) ) )");
Prover<AbstractCheckpointedProofEngine> Imp::get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const
{
    pwff ant;
    if (positive) {
        ant = var;
    } else {
        ant = Not::create(var);
    }
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Imp::subst_rp,
        {{"ph", ant->get_type_prover(tb)}, {"ps", this->a->get_type_prover(tb)}, {"ch", this->a->subst(var, positive)->get_type_prover(tb)}, {"th", this->b->get_type_prover(tb)}, {"ta", this->b->subst(var, positive)->get_type_prover(tb)}},
    {this->a->get_subst_prover(var, positive, tb), this->b->get_subst_prover(var, positive, tb)});
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

/*std::vector<SymTok> Biimp::to_sentence(const Library &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->b->to_sentence(lib);
    copy(senta.begin(), senta.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol("<->"));
    copy(sentb.begin(), sentb.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol(")"));
    return ret;
}*/

void Biimp::get_variables(pvar_set &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

RegisteredProver Biimp::type_rp = LibraryToolbox::register_prover({}, "wff ( ph <-> ps )");
Prover<AbstractCheckpointedProofEngine> Biimp::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Biimp::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver Biimp::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph <-> ps ) <-> -. ( ( ph -> ps ) -> -. ( ps -> ph ) ) )");
RegisteredProver Biimp::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<AbstractCheckpointedProofEngine> Biimp::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Biimp::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Biimp::imp_not_2_rp,
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

RegisteredProver Xor::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/_ ps )");
Prover<AbstractCheckpointedProofEngine> Xor::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Xor::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver Xor::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/_ ps ) <-> -. ( ph <-> ps ) )");
RegisteredProver Xor::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<AbstractCheckpointedProofEngine> Xor::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Xor::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Xor::imp_not_2_rp,
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

/*std::vector<SymTok> Xor::to_sentence(const Library &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->b->to_sentence(lib);
    copy(senta.begin(), senta.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol("\\/_"));
    copy(sentb.begin(), sentb.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol(")"));
    return ret;
}*/

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

RegisteredProver Nand::type_rp = LibraryToolbox::register_prover({}, "wff ( ph -/\\ ps )");
Prover<AbstractCheckpointedProofEngine> Nand::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Nand::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver Nand::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph -/\\ ps ) <-> -. ( ph /\\ ps ) )");
RegisteredProver Nand::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<AbstractCheckpointedProofEngine> Nand::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Nand::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Nand::imp_not_2_rp,
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

/*std::vector<SymTok> Nand::to_sentence(const Library &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->b->to_sentence(lib);
    copy(senta.begin(), senta.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol("-/\\"));
    copy(sentb.begin(), sentb.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol(")"));
    return ret;
}*/

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

RegisteredProver Or::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/ ps )");
Prover<AbstractCheckpointedProofEngine> Or::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Or::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver Or::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/ ps ) <-> ( -. ph -> ps ) )");
RegisteredProver Or::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<AbstractCheckpointedProofEngine> Or::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Or::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Or::imp_not_2_rp,
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

/*std::vector<SymTok> Or::to_sentence(const Library &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->b->to_sentence(lib);
    copy(senta.begin(), senta.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol("\\/"));
    copy(sentb.begin(), sentb.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol(")"));
    return ret;
}*/

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

/*std::vector<SymTok> And::to_sentence(const Library &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->b->to_sentence(lib);
    copy(senta.begin(), senta.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol("/\\"));
    copy(sentb.begin(), sentb.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol(")"));
    return ret;
}*/

RegisteredProver And::type_rp = LibraryToolbox::register_prover({}, "wff ( ph /\\ ps )");
Prover<AbstractCheckpointedProofEngine> And::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(And::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver And::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph /\\ ps ) <-> -. ( ph -> -. ps ) )");
RegisteredProver And::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<AbstractCheckpointedProofEngine> And::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< AbstractCheckpointedProofEngine >(And::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< AbstractCheckpointedProofEngine >(And::imp_not_2_rp,
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

pwff ConvertibleWff::subst(pvar var, bool positive) const
{
    return this->imp_not_form()->subst(var, positive);
}

RegisteredProver ConvertibleWff::truth_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- ph"}, "|- ps");
Prover<AbstractCheckpointedProofEngine> ConvertibleWff::get_truth_prover(const LibraryToolbox &tb) const
{
    //return null_prover;
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(ConvertibleWff::truth_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_truth_prover(tb)});
}

bool ConvertibleWff::is_true() const
{
    return this->imp_not_form()->is_true();
}

RegisteredProver ConvertibleWff::falsity_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- -. ph"}, "|- -. ps");
Prover<AbstractCheckpointedProofEngine> ConvertibleWff::get_falsity_prover(const LibraryToolbox &tb) const
{
    //return null_prover;
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(ConvertibleWff::falsity_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_falsity_prover(tb)});
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

/*std::vector<SymTok> And3::to_sentence(const Library &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->b->to_sentence(lib);
    vector< SymTok > sentc = this->c->to_sentence(lib);
    copy(senta.begin(), senta.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol("/\\"));
    copy(sentb.begin(), sentb.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol("/\\"));
    copy(sentc.begin(), sentc.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol(")"));
    return ret;
}*/

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

RegisteredProver And3::type_rp = LibraryToolbox::register_prover({}, "wff ( ph /\\ ps /\\ ch )");
Prover<AbstractCheckpointedProofEngine> And3::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(And3::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
}

RegisteredProver And3::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph /\\ ps /\\ ch ) <-> ( ( ph /\\ ps ) /\\ ch ) )");
RegisteredProver And3::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<AbstractCheckpointedProofEngine> And3::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< AbstractCheckpointedProofEngine >(And3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< AbstractCheckpointedProofEngine >(And3::imp_not_2_rp,
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

Or3::Or3(pwff a, pwff b, pwff c) :
    a(a), b(b), c(c)
{
}

string Or3::to_string() const
{
    return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " \\/ " + this->c->to_string() + " )";
}

/*std::vector<SymTok> Or3::to_sentence(const Library &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->b->to_sentence(lib);
    vector< SymTok > sentc = this->c->to_sentence(lib);
    copy(senta.begin(), senta.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol("\\/"));
    copy(sentb.begin(), sentb.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol("\\/"));
    copy(sentc.begin(), sentc.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol(")"));
    return ret;
}*/

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

RegisteredProver Or3::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/ ps \\/ ch )");
Prover<AbstractCheckpointedProofEngine> Or3::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< AbstractCheckpointedProofEngine >(Or3::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
}

RegisteredProver Or3::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/ ps \\/ ch ) <-> ( ( ph \\/ ps ) \\/ ch ) )");
RegisteredProver Or3::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover<AbstractCheckpointedProofEngine> Or3::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Or3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< AbstractCheckpointedProofEngine >(Or3::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
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

bool pvar_comp::operator()(const pvar x, const pvar y) {
    return *x < *y;
}
