
#include "wff.h"

using namespace std;

Wff::~Wff()
{
}

Prover Wff::get_truth_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

Prover Wff::get_falsity_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

Prover Wff::get_type_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

Prover Wff::get_imp_not_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

Prover Wff::get_subst_prover(string var, bool positive, const LibraryToolbox &tb) const
{
    (void) var;
    (void) positive;
    (void) tb;
    return null_prover;
}

RegisteredProver Wff::adv_truth_1_rp = LibraryToolbox::register_prover({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )");
RegisteredProver Wff::adv_truth_2_rp = LibraryToolbox::register_prover({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )");
RegisteredProver Wff::adv_truth_3_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( -. ph -> ps )"}, "|- ps");
Prover Wff::adv_truth_internal(set< string >::iterator cur_var, set< string >::iterator end_var, const LibraryToolbox &tb) const {
    if (cur_var == end_var) {
        return this->get_truth_prover(tb);
    } else {
        string var = *cur_var++;
        pwff pos_wff = this->subst(var, true);
        pwff neg_wff = this->subst(var, false);
        pwff pos_antecent = pwff(new Var(var));
        pwff neg_antecent = pwff(new Not(pwff(new Var(var))));
        Prover rec_pos_prover = pos_wff->adv_truth_internal(cur_var, end_var, tb);
        Prover rec_neg_prover = neg_wff->adv_truth_internal(cur_var, end_var, tb);
        Prover pos_prover = this->get_subst_prover(var, true, tb);
        Prover neg_prover = this->get_subst_prover(var, false, tb);
        Prover pos_mp_prover = tb.build_registered_prover(Wff::adv_truth_1_rp,
            {{"ph", pos_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}, {"ch", pos_wff->get_type_prover(tb)}},
            {rec_pos_prover, pos_prover});
        Prover neg_mp_prover = tb.build_registered_prover(Wff::adv_truth_2_rp,
            {{"ph", neg_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}, {"ch", neg_wff->get_type_prover(tb)}},
            {rec_neg_prover, neg_prover});
        Prover final = tb.build_registered_prover(Wff::adv_truth_3_rp,
            {{"ph", pos_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}},
            {pos_mp_prover, neg_mp_prover});
        return final;
    }
}

RegisteredProver Wff::adv_truth_4_rp = LibraryToolbox::register_prover({"|- ps", "|- ( ph <-> ps )"}, "|- ph");
Prover Wff::get_adv_truth_prover(const LibraryToolbox &tb) const
{
    pwff not_imp = this->imp_not_form();
    set< string > vars;
    this->get_variables(vars);
    Prover real = not_imp->adv_truth_internal(vars.begin(), vars.end(), tb);
    Prover equiv = this->get_imp_not_prover(tb);
    Prover final = tb.build_registered_prover(Wff::adv_truth_4_rp, {{"ph", this->get_type_prover(tb)}, {"ps", not_imp->get_type_prover(tb)}}, {real, equiv});
    return final;
}

True::True() {
}

string True::to_string() const {
    return "T.";
}

pwff True::imp_not_form() const {
    return pwff(new True());
}

pwff True::subst(string var, bool positive) const
{
    (void) var;
    (void) positive;
    return pwff(new True());
}

std::vector<SymTok> True::to_sentence(const Library &lib) const
{
    return { lib.get_symbol("T.") };
}

void True::get_variables(std::set<string> &vars) const
{
    (void) vars;
}

RegisteredProver True::truth_rp = LibraryToolbox::register_prover({}, "|- T.");
Prover True::get_truth_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(True::truth_rp, {}, {});
}

RegisteredProver True::type_rp = LibraryToolbox::register_prover({}, "wff T.");
Prover True::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(True::type_rp, {}, {});
}

RegisteredProver True::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( T. <-> T. )");
Prover True::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(True::imp_not_rp, {}, {});
}

RegisteredProver True::subst_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( T. <-> T. ) )");
Prover True::get_subst_prover(string var, bool positive, const LibraryToolbox &tb) const
{
    pwff subst = pwff(new Var(var));
    if (!positive) {
        subst = pwff(new Not(subst));
    }
    return tb.build_registered_prover(True::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
}

False::False() {
}

string False::to_string() const {
    return "F.";
}

pwff False::imp_not_form() const {
    return pwff(new False());
}

pwff False::subst(string var, bool positive) const
{
    (void) var;
    (void) positive;
    return pwff(new False());
}

std::vector<SymTok> False::to_sentence(const Library &lib) const
{
    return { lib.get_symbol("F.") };
}

void False::get_variables(std::set<string> &vars) const
{
    (void) vars;
}

RegisteredProver False::falsity_rp = LibraryToolbox::register_prover({}, "|- -. F.");
Prover False::get_falsity_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(False::falsity_rp, {}, {});
}

RegisteredProver False::type_rp = LibraryToolbox::register_prover({}, "wff F.");
Prover False::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(False::type_rp, {}, {});
}

RegisteredProver False::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( F. <-> F. )");
Prover False::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(False::imp_not_rp, {}, {});
}

RegisteredProver False::subst_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( F. <-> F. ) )");
Prover False::get_subst_prover(string var, bool positive, const LibraryToolbox &tb) const
{
    pwff subst = pwff(new Var(var));
    if (!positive) {
        subst = pwff(new Not(subst));
    }
    return tb.build_registered_prover(False::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
}

Var::Var(string name) :
    name(name) {
}

string Var::to_string() const {
    return this->name;
}

pwff Var::imp_not_form() const {
    return pwff(new Var(this->name));
}

pwff Var::subst(string var, bool positive) const
{
    if (this->name == var) {
        if (positive) {
            return pwff(new True());
        } else {
            return pwff(new False());
        }
    } else {
        return pwff(new Var(this->name));
    }
}

std::vector<SymTok> Var::to_sentence(const Library &lib) const
{
    return { lib.get_symbol(this->name) };
}

void Var::get_variables(std::set<string> &vars) const
{
    vars.insert(this->name);
}

Prover Var::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_type_prover2("wff " + this->name);
}

RegisteredProver Var::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( ph <-> ph )");
Prover Var::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(Var::imp_not_rp, {{"ph", this->get_type_prover(tb)}}, {});
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
Prover Var::get_subst_prover(string var, bool positive, const LibraryToolbox &tb) const
{
    if (var == this->name) {
        if (positive) {
            Prover first = tb.build_registered_prover(Var::subst_pos_1_rp, {{"ph", this->get_type_prover(tb)}}, {});
            Prover truth = tb.build_registered_prover(Var::subst_pos_truth_rp, {}, {});
            Prover second = tb.build_registered_prover(Var::subst_pos_2_rp, {{"ph", this->get_type_prover(tb)}}, {truth, first});
            Prover third = tb.build_registered_prover(Var::subst_pos_3_rp, {{"ph", this->get_type_prover(tb)}}, {second});
            return third;
        } else {
            Prover first = tb.build_registered_prover(Var::subst_neg_1_rp, {{"ph", this->get_type_prover(tb)}}, {});
            Prover falsity = tb.build_registered_prover(Var::subst_neg_falsity_rp, {}, {});
            Prover second = tb.build_registered_prover(Var::subst_neg_2_rp, {{"ph", this->get_type_prover(tb)}}, {falsity, first});
            Prover third = tb.build_registered_prover(Var::subst_neg_3_rp, {{"ph", this->get_type_prover(tb)}}, {second});
            return third;
        }
    } else {
        pwff ant = pwff(new Var(var));
        if (!positive) {
            ant = pwff(new Not(ant));
        }
        return tb.build_registered_prover(Var::subst_indep_rp, {{"ps", ant->get_type_prover(tb)}, {"ph", this->get_type_prover(tb)}}, {});
    }
}

Not::Not(pwff a) :
    a(a) {
}

string Not::to_string() const {
    return "-. " + this->a->to_string();
}

pwff Not::imp_not_form() const {
    return pwff(new Not(this->a->imp_not_form()));
}

pwff Not::subst(string var, bool positive) const
{
    return pwff(new Not(this->a->subst(var, positive)));
}

std::vector<SymTok> Not::to_sentence(const Library &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("-."));
    vector< SymTok > sent = this->a->to_sentence(lib);
    copy(sent.begin(), sent.end(), back_inserter(ret));
    return ret;
}

void Not::get_variables(std::set<string> &vars) const
{
    this->a->get_variables(vars);
}

Prover Not::get_truth_prover(const LibraryToolbox &tb) const
{
    return this->a->get_falsity_prover(tb);
}

RegisteredProver Not::falsity_rp = LibraryToolbox::register_prover({ "|- ph" }, "|- -. -. ph");
Prover Not::get_falsity_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(Not::falsity_rp, {{ "ph", this->a->get_type_prover(tb) }}, { this->a->get_truth_prover(tb) });
}

RegisteredProver Not::type_rp = LibraryToolbox::register_prover({}, "wff -. ph");
Prover Not::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(Not::type_rp, {{ "ph", this->a->get_type_prover(tb) }}, {});
}

RegisteredProver Not::imp_not_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )"}, "|- ( -. ph <-> -. ps )");
Prover Not::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(Not::imp_not_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }}, { this->a->get_imp_not_prover(tb) });
}

RegisteredProver Not::subst_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ( -. ps <-> -. ch ) )");
Prover Not::get_subst_prover(string var, bool positive, const LibraryToolbox &tb) const
{
    pwff ant;
    if (positive) {
        ant = pwff(new Var(var));
    } else {
        ant = pwff(new Not(pwff(new Var(var))));
    }
    return tb.build_registered_prover(Not::subst_rp,
        {{"ph", ant->get_type_prover(tb)}, {"ps", this->a->get_type_prover(tb)}, {"ch", this->a->subst(var, positive)->get_type_prover(tb)}}, {this->a->get_subst_prover(var, positive, tb)});
}

Imp::Imp(pwff a, pwff b) :
    a(a), b(b) {
}

string Imp::to_string() const {
    return "( " + this->a->to_string() + " -> " + this->b->to_string() + " )";
}

pwff Imp::imp_not_form() const {
    return pwff(new Imp(this->a->imp_not_form(), this->b->imp_not_form()));
}

pwff Imp::subst(string var, bool positive) const
{
    return pwff(new Imp(this->a->subst(var, positive), this->b->subst(var, positive)));
}

std::vector<SymTok> Imp::to_sentence(const Library &lib) const
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
}

void Imp::get_variables(std::set<string> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

RegisteredProver Imp::truth_1_rp = LibraryToolbox::register_prover({ "|- ps" }, "|- ( ph -> ps )");
RegisteredProver Imp::truth_2_rp = LibraryToolbox::register_prover({ "|- -. ph" }, "|- ( ph -> ps )");
Prover Imp::get_truth_prover(const LibraryToolbox &tb) const
{
    Prover first_prover = tb.build_registered_prover(Imp::truth_1_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, { this->b->get_truth_prover(tb) });
    Prover second_prover = tb.build_registered_prover(Imp::truth_2_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, { this->a->get_falsity_prover(tb) });
    return cascade_provers(first_prover, second_prover);
}

RegisteredProver Imp::falsity_1_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )");
RegisteredProver Imp::falsity_2_rp = LibraryToolbox::register_prover({ "|- ph", "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )"}, "|- ( -. ps -> -. ( ph -> ps ) )");
RegisteredProver Imp::falsity_3_rp = LibraryToolbox::register_prover({ "|- ( -. ps -> -. ( ph -> ps ) )", "|- -. ps"}, "|- -. ( ph -> ps )");
Prover Imp::get_falsity_prover(const LibraryToolbox &tb) const
{
    Prover theorem_prover = tb.build_registered_prover(Imp::falsity_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    Prover mp_prover1 = tb.build_registered_prover(Imp::falsity_2_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { this->a->get_truth_prover(tb), theorem_prover });
    Prover mp_prover2 = tb.build_registered_prover(Imp::falsity_3_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { mp_prover1, this->b->get_falsity_prover(tb) });
    return mp_prover2;
}

RegisteredProver Imp::type_rp = LibraryToolbox::register_prover({}, "wff ( ph -> ps )");
Prover Imp::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(Imp::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver Imp::imp_not_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ch <-> th )"}, "|- ( ( ph -> ch ) <-> ( ps -> th ) )");
Prover Imp::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(Imp::imp_not_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }, {"ch", this->b->get_type_prover(tb)}, {"th", this->b->imp_not_form()->get_type_prover(tb)}},
    { this->a->get_imp_not_prover(tb), this->b->get_imp_not_prover(tb) });
}

RegisteredProver Imp::subst_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps -> th ) <-> ( ch -> ta ) ) )");
Prover Imp::get_subst_prover(string var, bool positive, const LibraryToolbox &tb) const
{
    pwff ant;
    if (positive) {
        ant = pwff(new Var(var));
    } else {
        ant = pwff(new Not(pwff(new Var(var))));
    }
    return tb.build_registered_prover(Imp::subst_rp,
        {{"ph", ant->get_type_prover(tb)}, {"ps", this->a->get_type_prover(tb)}, {"ch", this->a->subst(var, positive)->get_type_prover(tb)}, {"th", this->b->get_type_prover(tb)}, {"ta", this->b->subst(var, positive)->get_type_prover(tb)}},
        {this->a->get_subst_prover(var, positive, tb), this->b->get_subst_prover(var, positive, tb)});
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
    return pwff(new Not(pwff(new Imp(pwff(new Imp(ain, bin)), pwff(new Not(pwff(new Imp(bin, ain))))))));
}

pwff Biimp::half_imp_not_form() const
{
    auto ain = this->a;
    auto bin = this->b;
    return pwff(new Not(pwff(new Imp(pwff(new Imp(ain, bin)), pwff(new Not(pwff(new Imp(bin, ain))))))));
}

std::vector<SymTok> Biimp::to_sentence(const Library &lib) const
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
}

void Biimp::get_variables(std::set<string> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

RegisteredProver Biimp::type_rp = LibraryToolbox::register_prover({}, "wff ( ph <-> ps )");
Prover Biimp::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(Biimp::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver Biimp::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph <-> ps ) <-> -. ( ( ph -> ps ) -> -. ( ps -> ph ) ) )");
RegisteredProver Biimp::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover Biimp::get_imp_not_prover(const LibraryToolbox &tb) const
{
    Prover first = tb.build_registered_prover(Biimp::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover(tb);
    Prover compose = tb.build_registered_prover(Biimp::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

Xor::Xor(pwff a, pwff b) :
    a(a), b(b) {
}

string Xor::to_string() const {
    return "( " + this->a->to_string() + " \\/_ " + this->b->to_string() + " )";
}

pwff Xor::imp_not_form() const {
    return pwff(new Not(pwff(new Biimp(this->a->imp_not_form(), this->b->imp_not_form()))))->imp_not_form();
}

pwff Xor::half_imp_not_form() const
{
    return pwff(new Not(pwff(new Biimp(this->a, this->b))));
}

void Xor::get_variables(std::set<string> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

RegisteredProver Xor::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/_ ps )");
Prover Xor::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(Xor::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver Xor::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/_ ps ) <-> -. ( ph <-> ps ) )");
RegisteredProver Xor::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover Xor::get_imp_not_prover(const LibraryToolbox &tb) const
{
    Prover first = tb.build_registered_prover(Xor::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover(tb);
    Prover compose = tb.build_registered_prover(Xor::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

std::vector<SymTok> Xor::to_sentence(const Library &lib) const
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
}

Nand::Nand(pwff a, pwff b) :
    a(a), b(b) {
}

string Nand::to_string() const {
    return "( " + this->a->to_string() + " -/\\ " + this->b->to_string() + " )";
}

pwff Nand::imp_not_form() const {
    return pwff(new Not(pwff(new And(this->a->imp_not_form(), this->b->imp_not_form()))))->imp_not_form();
}

pwff Nand::half_imp_not_form() const
{
    return pwff(new Not(pwff(new And(this->a, this->b))));
}

void Nand::get_variables(std::set<string> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

RegisteredProver Nand::type_rp = LibraryToolbox::register_prover({}, "wff ( ph -/\\ ps )");
Prover Nand::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(Nand::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver Nand::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph -/\\ ps ) <-> -. ( ph /\\ ps ) )");
RegisteredProver Nand::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover Nand::get_imp_not_prover(const LibraryToolbox &tb) const
{
    Prover first = tb.build_registered_prover(Nand::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover(tb);
    Prover compose = tb.build_registered_prover(Nand::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

std::vector<SymTok> Nand::to_sentence(const Library &lib) const
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
}

Or::Or(pwff a, pwff b) :
    a(a), b(b) {
}

string Or::to_string() const {
    return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " )";
}

pwff Or::imp_not_form() const {
    return pwff(new Imp(pwff(new Not(this->a->imp_not_form())), this->b->imp_not_form()));
}

pwff Or::half_imp_not_form() const
{
    return pwff(new Imp(pwff(new Not(this->a)), this->b));
}

void Or::get_variables(std::set<string> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

RegisteredProver Or::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/ ps )");
Prover Or::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(Or::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver Or::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/ ps ) <-> ( -. ph -> ps ) )");
RegisteredProver Or::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover Or::get_imp_not_prover(const LibraryToolbox &tb) const
{
    Prover first = tb.build_registered_prover(Or::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover(tb);
    Prover compose = tb.build_registered_prover(Or::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

std::vector<SymTok> Or::to_sentence(const Library &lib) const
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
}

And::And(pwff a, pwff b) :
    a(a), b(b) {
}

string And::to_string() const {
    return "( " + this->a->to_string() + " /\\ " + this->b->to_string() + " )";
}

pwff And::half_imp_not_form() const {
    return pwff(new Not(pwff(new Imp(this->a, pwff(new Not(this->b))))));
}

void And::get_variables(std::set<string> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

pwff And::imp_not_form() const
{
    return pwff(new Not(pwff(new Imp(this->a->imp_not_form(), pwff(new Not(this->b->imp_not_form()))))));
}

std::vector<SymTok> And::to_sentence(const Library &lib) const
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
}

RegisteredProver And::type_rp = LibraryToolbox::register_prover({}, "wff ( ph /\\ ps )");
Prover And::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(And::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

RegisteredProver And::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph /\\ ps ) <-> -. ( ph -> -. ps ) )");
RegisteredProver And::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover And::get_imp_not_prover(const LibraryToolbox &tb) const
{
    Prover first = tb.build_registered_prover(And::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover(tb);
    Prover compose = tb.build_registered_prover(And::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

pwff ConvertibleWff::subst(string var, bool positive) const
{
    return this->imp_not_form()->subst(var, positive);
}

RegisteredProver ConvertibleWff::truth_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- ph"}, "|- ps");
Prover ConvertibleWff::get_truth_prover(const LibraryToolbox &tb) const
{
    //return null_prover;
    return tb.build_registered_prover(ConvertibleWff::truth_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_truth_prover(tb)});
}

RegisteredProver ConvertibleWff::falsity_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- -. ph"}, "|- -. ps");
Prover ConvertibleWff::get_falsity_prover(const LibraryToolbox &tb) const
{
    //return null_prover;
    return tb.build_registered_prover(ConvertibleWff::falsity_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_falsity_prover(tb)});
}

Prover ConvertibleWff::get_type_prover(const LibraryToolbox &tb) const
{
    // Disabled, because ordinarily I do not want to use this generic and probably inefficient method
    return null_prover;
    return tb.build_type_prover2("wff " + this->to_string());
}

And3::And3(pwff a, pwff b, pwff c) :
    a(a), b(b), c(c)
{
}

string And3::to_string() const
{
    return "( " + this->a->to_string() + " /\\ " + this->b->to_string() + " /\\ " + this->c->to_string() + " )";
}

std::vector<SymTok> And3::to_sentence(const Library &lib) const
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
}

pwff And3::imp_not_form() const
{
    return pwff(new And(pwff(new And(this->a, this->b)), this->c))->imp_not_form();
}

pwff And3::half_imp_not_form() const
{
    return pwff(new And(pwff(new And(this->a, this->b)), this->c));
}

void And3::get_variables(std::set<string> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
    this->c->get_variables(vars);
}

RegisteredProver And3::type_rp = LibraryToolbox::register_prover({}, "wff ( ph /\\ ps /\\ ch )");
Prover And3::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(And3::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
}

RegisteredProver And3::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph /\\ ps /\\ ch ) <-> ( ( ph /\\ ps ) /\\ ch ) )");
RegisteredProver And3::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover And3::get_imp_not_prover(const LibraryToolbox &tb) const
{
    Prover first = tb.build_registered_prover(And3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover(tb);
    Prover compose = tb.build_registered_prover(And3::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

Or3::Or3(pwff a, pwff b, pwff c) :
    a(a), b(b), c(c)
{
}

string Or3::to_string() const
{
    return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " \\/ " + this->c->to_string() + " )";
}

std::vector<SymTok> Or3::to_sentence(const Library &lib) const
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
}

pwff Or3::imp_not_form() const
{
    return pwff(new Or(pwff(new Or(this->a, this->b)), this->c))->imp_not_form();
}

pwff Or3::half_imp_not_form() const
{
    return pwff(new Or(pwff(new Or(this->a, this->b)), this->c));
}

void Or3::get_variables(std::set<string> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
    this->c->get_variables(vars);
}

RegisteredProver Or3::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/ ps \\/ ch )");
Prover Or3::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover(Or3::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
}

RegisteredProver Or3::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/ ps \\/ ch ) <-> ( ( ph \\/ ps ) \\/ ch ) )");
RegisteredProver Or3::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
Prover Or3::get_imp_not_prover(const LibraryToolbox &tb) const
{
    Prover first = tb.build_registered_prover(Or3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover(tb);
    Prover compose = tb.build_registered_prover(Or3::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}
