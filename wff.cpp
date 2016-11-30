
#include "wff.h"

using namespace std;

std::vector<LabTok> Wff::prove_true(const LibraryInterface &lib) {
    (void) lib;
    return {};
}

std::vector<LabTok> Wff::prove_false(const LibraryInterface &lib) {
    (void) lib;
    return {};
}

True::True() {
}

string True::to_string() {
    return "T.";
}

pwff True::imp_not_form() {
    // Already fundamental
    return pwff(new True());
}

std::vector<LabTok> True::prove_true(const LibraryInterface &lib) {
    auto res = lib.unify_assertion({}, parse_sentence("|- T.", lib));
    assert(!res.empty());
    return { res.begin()->first };
}

False::False() {
}

string False::to_string() {
    return "F.";
}

pwff False::imp_not_form() {
    // Already fundamental
    return pwff(new False());
}

std::vector<LabTok> False::prove_false(const LibraryInterface &lib) {
    auto res = lib.unify_assertion({}, parse_sentence("|- -. F.", lib));
    assert(!res.empty());
    return { res.begin()->first };
}

Var::Var(string name) :
    name(name) {
}

string Var::to_string() {
    return this->name;
}

pwff Var::imp_not_form() {
    // Already fundamental
    return pwff(new Var(this->name));
}

Not::Not(pwff a) :
    a(a) {
}

string Not::to_string() {
    return "-. " + this->a->to_string();
}

pwff Not::imp_not_form() {
    // Already fundamental
    return pwff(new Not(this->a->imp_not_form()));
}

std::vector<LabTok> Not::prove_true(const LibraryInterface &lib) {
    std::vector< LabTok > rec = this->a->prove_false(lib);
    if (rec.empty()) {
        return {};
    }
    return rec;
}

static void proving_helper() {

}

std::vector<LabTok> Not::prove_false(const LibraryInterface &lib) {
    std::vector< LabTok > rec = this->a->prove_true(lib);
    if (rec.empty()) {
        return {};
    }
    auto res = lib.unify_assertion({parse_sentence("|- ph", lib)}, parse_sentence("|- -. -. ph", lib));
    assert(!res.empty());
    rec.push_back(res.begin()->first);
    return rec;
}

Imp::Imp(pwff a, pwff b) :
    a(a), b(b) {
}

string Imp::to_string() {
    return "( " + this->a->to_string() + " -> " + this->b->to_string() + " )";
}

pwff Imp::imp_not_form() {
    // Already fundamental
    return pwff(new Imp(this->a->imp_not_form(), this->b->imp_not_form()));
}

std::vector<LabTok> Imp::prove_true(const LibraryInterface &lib) {
    auto rec = this->b->prove_true(lib);
    if (rec.empty()) {
        rec = this->a->prove_false(lib);
        if (rec.empty()) {
            return {};
        } else {
            auto res = lib.unify_assertion({parse_sentence("|- -. ph", lib)}, parse_sentence("|- ( ph -> ps )", lib));
            assert(!res.empty());
            rec.push_back(res.begin()->first);
            return rec;
        }
    } else {
        auto res = lib.unify_assertion({parse_sentence("|- ph", lib)}, parse_sentence("|- ( ps -> ph )", lib));
        assert(!res.empty());
        rec.push_back(res.begin()->first);
        return rec;
    }
}

Biimp::Biimp(pwff a, pwff b) :
    a(a), b(b) {
}

string Biimp::to_string() {
    return "( " + this->a->to_string() + " <-> " + this->b->to_string() + " )";
}

pwff Biimp::imp_not_form() {
    // Using dfbi1
    auto ain = this->a->imp_not_form();
    auto bin = this->b->imp_not_form();
    return pwff(new Not(pwff(new Imp(pwff(new Imp(ain, bin)), pwff(new Not(pwff(new Imp(bin, ain))))))));
}

Xor::Xor(pwff a, pwff b) :
    a(a), b(b) {
}

string Xor::to_string() {
    return "( " + this->a->to_string() + " \\/_ " + this->b->to_string() + " )";
}

pwff Xor::imp_not_form() {
    // Using df-xor and recurring
    return pwff(new Not(pwff(new Biimp(this->a->imp_not_form(), this->b->imp_not_form()))))->imp_not_form();
}

Nand::Nand(pwff a, pwff b) :
    a(a), b(b) {
}

string Nand::to_string() {
    return "( " + this->a->to_string() + " -/\\ " + this->b->to_string() + " )";
}

pwff Nand::imp_not_form() {
    // Using df-nan and recurring
    return pwff(new Not(pwff(new Or(this->a->imp_not_form(), this->b->imp_not_form()))))->imp_not_form();
}

Or::Or(pwff a, pwff b) :
    a(a), b(b) {
}

string Or::to_string() {
    return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " )";
}

pwff Or::imp_not_form() {
    // Using df-or
    return pwff(new Imp(pwff(new Not(this->a->imp_not_form())), this->b->imp_not_form()));
}

And::And(pwff a, pwff b) :
    a(a), b(b) {
}

string And::to_string() {
    return "( " + this->a->to_string() + " /\\ " + this->b->to_string() + " )";
}

pwff And::imp_not_form() {
    // Using df-an
    return pwff(new Not(pwff(new Imp(this->a->imp_not_form(), pwff(new Not(this->b->imp_not_form()))))));
}
