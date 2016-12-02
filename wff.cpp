
#include "toolbox.h"
#include "wff.h"

using namespace std;

void Wff::prove_type(const LibraryInterface &lib, ProofEngine &engine) const
{
    vector< SymTok > type_sent;
    type_sent.push_back(lib.get_symbol("wff"));
    auto sent = this->to_sentence(lib);
    copy(sent.begin(), sent.end(), back_inserter(type_sent));
    for (auto &label : lib.prove_type(type_sent)) {
        engine.process_label(label);
    }
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

std::function<bool (const LibraryInterface &, ProofEngine &)> Wff::get_truth_prover() const
{
    return [this](const LibraryInterface & lib, ProofEngine &engine){ return this->prove_true(lib, engine); };
}

std::function<bool (const LibraryInterface &, ProofEngine &)> Wff::get_falsehood_prover() const
{
    return [this](const LibraryInterface & lib, ProofEngine &engine){ return this->prove_false(lib, engine); };
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

void True::prove_type(const LibraryInterface &lib, ProofEngine &engine) const
{
    auto res = lib.unify_assertion({}, lib.parse_sentence("wff T."), true);
    engine.process_label(get<0>(res.at(0)));
}

bool True::prove_true(const LibraryInterface &lib, ProofEngine &engine) const
{
    LibraryToolbox tb(lib);
    return tb.proving_helper2({}, lib.parse_sentence("|- T."), {}, {}, engine);
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

bool False::prove_false(const LibraryInterface &lib, ProofEngine &engine) const
{
    LibraryToolbox tb(lib);
    return tb.proving_helper2({}, lib.parse_sentence("|- -. F."), {}, {}, engine);
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

bool Not::prove_true(const LibraryInterface &lib, ProofEngine &engine) const
{
    return this->a->prove_false(lib, engine);
}

bool Not::prove_false(const LibraryInterface &lib, ProofEngine &engine) const
{

    LibraryToolbox tb(lib);
    return tb.proving_helper2({ lib.parse_sentence("|- ph") }, lib.parse_sentence("|- -. -. ph"), { this->a->get_truth_prover() }, {{lib.get_symbol("ph"), this->a->to_sentence(lib) }}, engine);
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

bool Imp::prove_true(const LibraryInterface &lib, ProofEngine &engine) const
{
    LibraryToolbox tb(lib);
    bool res = tb.proving_helper2({ lib.parse_sentence("|- ps") }, lib.parse_sentence("|- ( ph -> ps )"),
        { this->b->get_truth_prover() },
        {{lib.get_symbol("ph"), this->a->to_sentence(lib) },
         {lib.get_symbol("ps"), this->b->to_sentence(lib)}},
        engine);
    if (res) {
        return true;
    }
    return tb.proving_helper2({ lib.parse_sentence("|- -. ph") }, lib.parse_sentence("|- ( ph -> ps )"),
        { this->a->get_falsehood_prover() },
        {{lib.get_symbol("ph"), this->a->to_sentence(lib) },
         {lib.get_symbol("ps"), this->b->to_sentence(lib)}},
                              engine);
}

bool Imp::prove_false(const LibraryInterface &lib, ProofEngine &engine) const
{
    LibraryToolbox tb(lib);
    return false;
}

Biimp::Biimp(pwff a, pwff b) :
    a(a), b(b) {
}

string Biimp::to_string() const {
    return "( " + this->a->to_string() + " <-> " + this->b->to_string() + " )";
}

pwff Biimp::imp_not_form() const {
    // Using dfbi1
    auto ain = this->a->imp_not_form();
    auto bin = this->b->imp_not_form();
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

Xor::Xor(pwff a, pwff b) :
    a(a), b(b) {
}

string Xor::to_string() const {
    return "( " + this->a->to_string() + " \\/_ " + this->b->to_string() + " )";
}

pwff Xor::imp_not_form() const {
    // Using df-xor and recurring
    return pwff(new Not(pwff(new Biimp(this->a->imp_not_form(), this->b->imp_not_form()))))->imp_not_form();
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
    // Using df-nan and recurring
    return pwff(new Not(pwff(new Or(this->a->imp_not_form(), this->b->imp_not_form()))))->imp_not_form();
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

pwff And::imp_not_form() const {
    // Using df-an
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
