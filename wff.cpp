
#include "toolbox.h"
#include "wff.h"

using namespace std;

/*bool Wff::prove_type(const LibraryInterface &lib, ProofEngine &engine) const
{
    vector< SymTok > type_sent;
    type_sent.push_back(lib.get_symbol("wff"));
    auto sent = this->to_sentence(lib);
    copy(sent.begin(), sent.end(), back_inserter(type_sent));
    auto proof = lib.prove_type(type_sent);
    if (proof.empty()) {
        return false;
    }
    for (auto &label : proof) {
        engine.process_label(label);
    }
    return true;
}

bool Wff::prove_true(const LibraryInterface &lib, ProofEngine &engine) const
{
    (void) lib;
    (void) engine;
    return false;
}

bool Wff::prove_false(const LibraryInterface &lib, ProofEngine &engine) const
{
    (void) lib;
    (void) engine;
    return false;
}

bool Wff::prove_imp_not(const LibraryInterface &lib, ProofEngine &engine) const
{
    (void) lib;
    (void) engine;
    return false;
}*/

Wff::~Wff()
{
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Wff::get_truth_prover() const
{
    return [](const LibraryInterface&, ProofEngine &){ return false; };
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Wff::get_falsity_prover() const
{
    return [this](const LibraryInterface &, ProofEngine &){ return false; };
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Wff::get_type_prover() const
{
    return [this](const LibraryInterface &, ProofEngine &){ return false; };
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Wff::get_imp_not_prover() const
{
    return [this](const LibraryInterface &, ProofEngine &){ return false; };
}

True::True() {
}

string True::to_string() const {
    return "T.";
}

pwff True::imp_not_form() const {
    // Already fundamental
    return pwff(new True());
}

std::vector<SymTok> True::to_sentence(const LibraryInterface &lib) const
{
    return { lib.get_symbol("T.") };
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

False::False() {
}

string False::to_string() const {
    return "F.";
}

pwff False::imp_not_form() const {
    // Already fundamental
    return pwff(new False());
}

std::vector<SymTok> False::to_sentence(const LibraryInterface &lib) const
{
    return { lib.get_symbol("F.") };
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

Var::Var(string name) :
    name(name) {
}

string Var::to_string() const {
    return this->name;
}

pwff Var::imp_not_form() const {
    // Already fundamental
    return pwff(new Var(this->name));
}

std::vector<SymTok> Var::to_sentence(const LibraryInterface &lib) const
{
    return { lib.get_symbol(this->name) };
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Var::get_type_prover() const
{
    return LibraryToolbox::build_type_prover2("wff " + this->name);
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Var::get_imp_not_prover() const
{
    return LibraryToolbox::build_prover4({}, "|- ( " + this->name + " <-> " + this->name + " )", {{"ph", this->get_type_prover()}}, {});
}

Not::Not(pwff a) :
    a(a) {
}

string Not::to_string() const {
    return "-. " + this->a->to_string();
}

pwff Not::imp_not_form() const {
    // Already fundamental
    return pwff(new Not(this->a->imp_not_form()));
}

std::vector<SymTok> Not::to_sentence(const LibraryInterface &lib) const
{
    vector< SymTok > ret;
    ret.push_back(lib.get_symbol("-."));
    vector< SymTok > sent = this->a->to_sentence(lib);
    copy(sent.begin(), sent.end(), back_inserter(ret));
    return ret;
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

Imp::Imp(pwff a, pwff b) :
    a(a), b(b) {
}

string Imp::to_string() const {
    return "( " + this->a->to_string() + " -> " + this->b->to_string() + " )";
}

pwff Imp::imp_not_form() const {
    // Already fundamental
    return pwff(new Imp(this->a->imp_not_form(), this->b->imp_not_form()));
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
    // Using df-or
    return pwff(new Imp(pwff(new Not(this->a->imp_not_form())), this->b->imp_not_form()));
}

pwff Or::half_imp_not_form() const
{
    return pwff(new Imp(pwff(new Not(this->a)), this->b));
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
