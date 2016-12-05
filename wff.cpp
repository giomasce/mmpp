
#include "wff.h"

using namespace std;

Wff::~Wff()
{
}

Prover Wff::get_truth_prover() const
{
    return null_prover;
}

Prover Wff::get_falsity_prover() const
{
    return null_prover;
}

Prover Wff::get_type_prover() const
{
    return null_prover;
}

Prover Wff::get_imp_not_prover() const
{
    return null_prover;
}

Prover Wff::get_subst_prover(string var, bool positive) const
{
    (void) var;
    (void) positive;
    return null_prover;
}

static Prover adv_truth_internal(pwff wff, set< string >::iterator cur_var, set< string >::iterator end_var) {
    if (cur_var == end_var) {
        return wff->get_truth_prover();
    } else {
        string var = *cur_var++;
        pwff pos_wff = wff->subst(var, true);
        pwff neg_wff = wff->subst(var, false);
        pwff pos_antecent = pwff(new Var(var));
        pwff neg_antecent = pwff(new Not(pwff(new Var(var))));
        Prover rec_pos_prover = adv_truth_internal(pos_wff, cur_var, end_var);
        Prover rec_neg_prover = adv_truth_internal(neg_wff, cur_var, end_var);
        Prover pos_prover = wff->get_subst_prover(var, true);
        Prover neg_prover = wff->get_subst_prover(var, false);
        Prover pos_mp_prover = LibraryToolbox::build_prover4({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )",
            {{"ph", pos_antecent->get_type_prover()}, {"ps", wff->get_type_prover()}, {"ch", pos_wff->get_type_prover()}},
            {rec_pos_prover, pos_prover});
        Prover neg_mp_prover = LibraryToolbox::build_prover4({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )",
            {{"ph", neg_antecent->get_type_prover()}, {"ps", wff->get_type_prover()}, {"ch", neg_wff->get_type_prover()}},
            {rec_neg_prover, neg_prover});
        Prover final = LibraryToolbox::build_prover4({"|- ( ph -> ps )", "|- ( -. ph -> ps )"}, "|- ps",
            {{"ph", pos_antecent->get_type_prover()}, {"ps", wff->get_type_prover()}},
            {pos_mp_prover, neg_mp_prover});
        return final;
    }
}

Prover Wff::get_adv_truth_prover() const
{
    pwff not_imp = this->imp_not_form();
    set< string > vars;
    this->get_variables(vars);
    Prover real = adv_truth_internal(not_imp, vars.begin(), vars.end());
    Prover equiv = this->get_imp_not_prover();
    Prover final = LibraryToolbox::build_prover4({"|- ps", "|- ( ph <-> ps )"}, "|- ph", {{"ph", this->get_type_prover()}, {"ps", not_imp->get_type_prover()}}, {real, equiv});
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

std::vector<SymTok> True::to_sentence(const LibraryInterface &lib) const
{
    return { lib.get_symbol("T.") };
}

void True::get_variables(std::set<string> &vars) const
{
    (void) vars;
}

std::function<bool (const LibraryInterface &, ProofEngine &)> True::get_truth_prover() const
{
    return LibraryToolbox::build_prover4({}, "|- T.", {}, {});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> True::get_type_prover() const
{
    return LibraryToolbox::build_prover4({}, "wff T.", {}, {});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> True::get_imp_not_prover() const
{
    return LibraryToolbox::build_prover4({}, "|- ( T. <-> T. )", {}, {});
}

Prover True::get_subst_prover(string var, bool positive) const
{
    return LibraryToolbox::build_prover4({}, string("|- ( ") + (positive ? "" : "-. ") + var + " -> ( T. <-> T. ) )", {}, {});
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

std::vector<SymTok> False::to_sentence(const LibraryInterface &lib) const
{
    return { lib.get_symbol("F.") };
}

void False::get_variables(std::set<string> &vars) const
{
    (void) vars;
}

std::function<bool (const LibraryInterface &, ProofEngine &)> False::get_falsity_prover() const
{
    return LibraryToolbox::build_prover4({}, "|- -. F.", {}, {});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> False::get_type_prover() const
{
    return LibraryToolbox::build_prover4({}, "wff F.", {}, {});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> False::get_imp_not_prover() const
{
    return LibraryToolbox::build_prover4({}, "|- ( F. <-> F. )", {}, {});
}

Prover False::get_subst_prover(string var, bool positive) const
{
    return LibraryToolbox::build_prover4({}, string("|- ( ") + (positive ? "" : "-. ") + var + " -> ( F. <-> F. ) )", {}, {});
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

std::vector<SymTok> Var::to_sentence(const LibraryInterface &lib) const
{
    return { lib.get_symbol(this->name) };
}

void Var::get_variables(std::set<string> &vars) const
{
    vars.insert(this->name);
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Var::get_type_prover() const
{
    return LibraryToolbox::build_type_prover2("wff " + this->name);
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Var::get_imp_not_prover() const
{
    return LibraryToolbox::build_prover4({}, "|- ( " + this->name + " <-> " + this->name + " )", {{"ph", this->get_type_prover()}}, {});
}

Prover Var::get_subst_prover(string var, bool positive) const
{
    if (var == this->name) {
        if (positive) {
            string first_sent = "|- ( T. -> ( " + var + " -> ( T. <-> " + var + " ) ) )";
            string second_sent = "|- ( " + var + " -> ( T. <-> " + var + " ) )";
            string third_sent = "|- ( " + var + " -> ( " + var + " <-> T. ) )";
            Prover first = LibraryToolbox::build_prover4({}, first_sent, {{var, this->get_type_prover()}}, {});
            Prover truth = LibraryToolbox::build_prover4({}, "|- T.", {}, {});
            Prover second = LibraryToolbox::build_prover4({"|- T.", first_sent}, second_sent, {{var, this->get_type_prover()}}, {truth, first});
            Prover third = LibraryToolbox::build_prover4({second_sent}, third_sent, {{var, this->get_type_prover()}}, {second});
            return third;
        } else {
            string first_sent = "|- ( -. F. -> ( -. " + var + " -> ( F. <-> " + var + " ) ) )";
            string second_sent = "|- ( -. " + var + " -> ( F. <-> " + var + " ) )";
            string third_sent = "|- ( -. " + var + " -> ( " + var + " <-> F. ) )";
            Prover first = LibraryToolbox::build_prover4({}, first_sent, {{var, this->get_type_prover()}}, {});
            Prover truth = LibraryToolbox::build_prover4({}, "|- -. F.", {}, {});
            Prover second = LibraryToolbox::build_prover4({"|- -. F.", first_sent}, second_sent, {{var, this->get_type_prover()}}, {truth, first});
            Prover third = LibraryToolbox::build_prover4({second_sent}, third_sent, {{var, this->get_type_prover()}}, {second});
            return third;
        }
        //return LibraryToolbox::build_prover4({}, string("|- ( ") + (positive ? "" : "-. ") + var + " -> ( " + this->name + " <-> " + (positive ? "T." : "F.") + " ) )", {}, {});
    } else {
        return LibraryToolbox::build_prover4({}, string("|- ( ") + (positive ? "" : "-. ") + var + " -> ( " + this->name + " <-> " + this->name + " ) )", {}, {});
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

std::vector<SymTok> Not::to_sentence(const LibraryInterface &lib) const
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

std::function<bool (const LibraryInterface &, ProofEngine &)> Not::get_truth_prover() const
{
    return this->a->get_falsity_prover();
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Not::get_falsity_prover() const
{
    return LibraryToolbox::build_prover4({ "|- ph" }, "|- -. -. ph", {{ "ph", this->a->get_type_prover() }}, { this->a->get_truth_prover() });
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Not::get_type_prover() const
{
    return LibraryToolbox::build_prover4({}, "wff -. ph", {{ "ph", this->a->get_type_prover() }}, {});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Not::get_imp_not_prover() const
{
    return LibraryToolbox::build_prover4({"|- ( ph <-> ps )"}, "|- ( -. ph <-> -. ps )", {{"ph", this->a->get_type_prover()}, {"ps", this->a->imp_not_form()->get_type_prover() }}, { this->a->get_imp_not_prover() });
}

Prover Not::get_subst_prover(string var, bool positive) const
{
    pwff antecent;
    if (positive) {
        antecent = pwff(new Var(var));
    } else {
        antecent = pwff(new Not(pwff(new Var(var))));
    }
    return LibraryToolbox::build_prover4({"|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ( -. ps <-> -. ch ) )",
        {{"ph", antecent->get_type_prover()}, {"ps", this->a->get_type_prover()}, {"ch", this->a->subst(var, positive)->get_type_prover()}}, {this->a->get_subst_prover(var, positive)});
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

std::vector<SymTok> Imp::to_sentence(const LibraryInterface &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->a->to_sentence(lib);
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

std::function<bool (const LibraryInterface &, ProofEngine &)> Imp::get_truth_prover() const
{
    Prover first_prover = LibraryToolbox::build_prover4({ "|- ps" }, "|- ( ph -> ps )", {{ "ph", this->a->get_type_prover() }, { "ps", this->b->get_type_prover() }}, { this->b->get_truth_prover() });
    Prover second_prover = LibraryToolbox::build_prover4({ "|- -. ph" }, "|- ( ph -> ps )", {{ "ph", this->a->get_type_prover() }, { "ps", this->b->get_type_prover() }}, { this->a->get_falsity_prover() });
    return LibraryToolbox::compose_provers(first_prover, second_prover);
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Imp::get_falsity_prover() const
{
    Prover theorem_prover = LibraryToolbox::build_prover4({}, "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )", {{"ph", this->a->get_type_prover()}, {"ps", this->b->get_type_prover()}}, {});
    Prover mp_prover1 = LibraryToolbox::build_prover4({ "|- ph", "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )"}, "|- ( -. ps -> -. ( ph -> ps ) )",
        {{"ph", this->a->get_type_prover()}, {"ps", this->b->get_type_prover()}}, { this->a->get_truth_prover(), theorem_prover });
    Prover mp_prover2 = LibraryToolbox::build_prover4({ "|- ( -. ps -> -. ( ph -> ps ) )", "|- -. ps"}, "|- -. ( ph -> ps )",
        {{"ph", this->a->get_type_prover()}, {"ps", this->b->get_type_prover()}}, { mp_prover1, this->b->get_falsity_prover() });
    return mp_prover2;
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Imp::get_type_prover() const
{
    return LibraryToolbox::build_prover4({}, "wff ( ph -> ps )", {{ "ph", this->a->get_type_prover() }, { "ps", this->b->get_type_prover() }}, {});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Imp::get_imp_not_prover() const
{
    return LibraryToolbox::build_prover4({"|- ( ph <-> ps )", "|- ( ch <-> th )"}, "|- ( ( ph -> ch ) <-> ( ps -> th ) )",
        {{"ph", this->a->get_type_prover()}, {"ps", this->a->imp_not_form()->get_type_prover() }, {"ch", this->b->get_type_prover()}, {"th", this->b->imp_not_form()->get_type_prover()}},
    { this->a->get_imp_not_prover(), this->b->get_imp_not_prover() });
}

Prover Imp::get_subst_prover(string var, bool positive) const
{
    pwff antecent;
    if (positive) {
        antecent = pwff(new Var(var));
    } else {
        antecent = pwff(new Not(pwff(new Var(var))));
    }
    return LibraryToolbox::build_prover4({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps -> th ) <-> ( ch -> ta ) ) )",
        {{"ph", antecent->get_type_prover()}, {"ps", this->a->get_type_prover()}, {"ch", this->a->subst(var, positive)->get_type_prover()}, {"th", this->b->get_type_prover()}, {"ta", this->b->subst(var, positive)->get_type_prover()}},
        {this->a->get_subst_prover(var, positive), this->b->get_subst_prover(var, positive)});
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

std::vector<SymTok> Biimp::to_sentence(const LibraryInterface &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->a->to_sentence(lib);
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

std::function<bool (const LibraryInterface &, ProofEngine &)> Biimp::get_type_prover() const
{
    return LibraryToolbox::build_prover4({}, "wff ( ph <-> ps )", {{ "ph", this->a->get_type_prover() }, { "ps", this->b->get_type_prover() }}, {});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Biimp::get_imp_not_prover() const
{
    Prover first = LibraryToolbox::build_prover4({}, "|- ( ( ph <-> ps ) <-> -. ( ( ph -> ps ) -> -. ( ps -> ph ) ) )", {{"ph", this->a->get_type_prover()}, {"ps", this->b->get_type_prover()}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover();
    Prover compose = LibraryToolbox::build_prover4({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )",
        {{"ph", this->get_type_prover()}, {"ps", this->half_imp_not_form()->get_type_prover()}, {"ch", this->imp_not_form()->get_type_prover()}}, {first, second});
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

std::function<bool (const LibraryInterface &, ProofEngine &)> Xor::get_type_prover() const
{
    return LibraryToolbox::build_prover4({}, "wff ( ph \\/_ ps )", {{ "ph", this->a->get_type_prover() }, { "ps", this->b->get_type_prover() }}, {});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Xor::get_imp_not_prover() const
{
    Prover first = LibraryToolbox::build_prover4({}, "|- ( ( ph \\/_ ps ) <-> -. ( ph <-> ps ) )", {{"ph", this->a->get_type_prover()}, {"ps", this->b->get_type_prover()}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover();
    Prover compose = LibraryToolbox::build_prover4({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )",
        {{"ph", this->get_type_prover()}, {"ps", this->half_imp_not_form()->get_type_prover()}, {"ch", this->imp_not_form()->get_type_prover()}}, {first, second});
    return compose;
}

std::vector<SymTok> Xor::to_sentence(const LibraryInterface &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->a->to_sentence(lib);
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

std::function<bool (const LibraryInterface &, ProofEngine &)> Nand::get_type_prover() const
{
    return LibraryToolbox::build_prover4({}, "wff ( ph -/\\ ps )", {{ "ph", this->a->get_type_prover() }, { "ps", this->b->get_type_prover() }}, {});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Nand::get_imp_not_prover() const
{
    Prover first = LibraryToolbox::build_prover4({}, "|- ( ( ph -/\\ ps ) <-> -. ( ph /\\ ps ) )", {{"ph", this->a->get_type_prover()}, {"ps", this->b->get_type_prover()}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover();
    Prover compose = LibraryToolbox::build_prover4({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )",
        {{"ph", this->get_type_prover()}, {"ps", this->half_imp_not_form()->get_type_prover()}, {"ch", this->imp_not_form()->get_type_prover()}}, {first, second});
    return compose;
}

std::vector<SymTok> Nand::to_sentence(const LibraryInterface &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->a->to_sentence(lib);
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

std::function<bool (const LibraryInterface &, ProofEngine &)> Or::get_type_prover() const
{
    return LibraryToolbox::build_prover4({}, "wff ( ph \\/ ps )", {{ "ph", this->a->get_type_prover() }, { "ps", this->b->get_type_prover() }}, {});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Or::get_imp_not_prover() const
{
    Prover first = LibraryToolbox::build_prover4({}, "|- ( ( ph \\/ ps ) <-> ( -. ph -> ps ) )", {{"ph", this->a->get_type_prover()}, {"ps", this->b->get_type_prover()}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover();
    Prover compose = LibraryToolbox::build_prover4({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )",
        {{"ph", this->get_type_prover()}, {"ps", this->half_imp_not_form()->get_type_prover()}, {"ch", this->imp_not_form()->get_type_prover()}}, {first, second});
    return compose;
}

std::vector<SymTok> Or::to_sentence(const LibraryInterface &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->a->to_sentence(lib);
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

std::vector<SymTok> And::to_sentence(const LibraryInterface &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("("));
    vector< SymTok > senta = this->a->to_sentence(lib);
    vector< SymTok > sentb = this->a->to_sentence(lib);
    copy(senta.begin(), senta.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol("/\\"));
    copy(sentb.begin(), sentb.end(), back_inserter(ret));
    ret.push_back(lib.get_symbol(")"));
    return ret;
}

std::function<bool (const LibraryInterface &, ProofEngine &)> And::get_type_prover() const
{
    return LibraryToolbox::build_prover4({}, "wff ( ph /\\ ps )", {{ "ph", this->a->get_type_prover() }, { "ps", this->b->get_type_prover() }}, {});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> And::get_imp_not_prover() const
{
    Prover first = LibraryToolbox::build_prover4({}, "|- ( ( ph /\\ ps ) <-> -. ( ph -> -. ps ) )", {{"ph", this->a->get_type_prover()}, {"ps", this->b->get_type_prover()}}, {});
    Prover second = this->half_imp_not_form()->get_imp_not_prover();
    Prover compose = LibraryToolbox::build_prover4({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )",
        {{"ph", this->get_type_prover()}, {"ps", this->half_imp_not_form()->get_type_prover()}, {"ch", this->imp_not_form()->get_type_prover()}}, {first, second});
    return compose;
}

pwff ConvertibleWff::subst(string var, bool positive) const
{
    return this->imp_not_form()->subst(var, positive);
}

std::function<bool (const LibraryInterface &, ProofEngine &)> ConvertibleWff::get_truth_prover() const
{
    //return null_prover;
    return LibraryToolbox::build_prover4({"|- ( ps <-> ph )", "|- ph"}, "|- ps", {{"ph", this->imp_not_form()->get_type_prover()}, {"ps", this->get_type_prover()}}, {this->get_imp_not_prover(), this->imp_not_form()->get_truth_prover()});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> ConvertibleWff::get_falsity_prover() const
{
    //return null_prover;
    return LibraryToolbox::build_prover4({"|- ( ps <-> ph )", "|- -. ph"}, "|- -. ps", {{"ph", this->imp_not_form()->get_type_prover()}, {"ps", this->get_type_prover()}}, {this->get_imp_not_prover(), this->imp_not_form()->get_falsity_prover()});
}

std::function<bool (const LibraryInterface &, ProofEngine &)> ConvertibleWff::get_type_prover() const
{
    // Disabled, because ordinarily I do not want to use this generic and probably inefficient method
    return null_prover;
    return LibraryToolbox::build_type_prover2("wff " + this->to_string());
}
