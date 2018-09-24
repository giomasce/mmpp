
#include "wff.h"
#include "mm/ptengine.h"

//#define LOG_WFF

template<typename Tag>
TWffBase<Tag>::~TWffBase() {}

template<typename Tag>
ParsingTree2<SymTok, LabTok> TWffBase<Tag>::to_parsing_tree(const LibraryToolbox &tb) const
{
    auto type_prover = this->get_type_prover(tb);
    CreativeProofEngineImpl< ParsingTree2< SymTok, LabTok > > engine(tb, false);
    bool res = type_prover(engine);
    assert(res);
#ifdef NDEBUG
    (void) res;
#endif
    return engine.get_stack().back();
}

template<typename Tag>
Prover<CheckpointedProofEngine> TWffBase<Tag>::get_truth_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

template<typename Tag>
bool TWffBase<Tag>::is_true() const
{
    return false;
}

template<typename Tag>
Prover<CheckpointedProofEngine> TWffBase<Tag>::get_falsity_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

template<typename Tag>
bool TWffBase<Tag>::is_false() const
{
    return false;
}

template<typename Tag>
Prover<CheckpointedProofEngine> TWffBase<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

Prover<CheckpointedProofEngine> TWff<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

Prover<CheckpointedProofEngine> TWff<PropTag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
{
    (void) var;
    (void) positive;
    (void) tb;
    return null_prover;
}

ptvar<PropTag> TWff<PropTag>::get_tseitin_var(const LibraryToolbox &tb) const {
    return TVar<Tag>::create(this->to_parsing_tree(tb), tb);
}

std::tuple<CNFProblem, ptvar_map<uint32_t, PropTag>, std::vector<Prover<CheckpointedProofEngine> > > TWff<PropTag>::get_tseitin_cnf_problem(const LibraryToolbox &tb) const {
    CNForm<Tag> cnf;
    this->get_tseitin_form(cnf, tb, *this);
    cnf[{{true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TWff::id_rp, {{"ph", this->get_type_prover(tb)}}, {});
    auto vars = collect_tseitin_vars(cnf);
    auto map = build_tseitin_map(vars);
    auto problem = build_cnf_problem(cnf, map);
    return make_tuple(problem.first, map, problem.second);
}

void TWff<PropTag>::set_library_toolbox(const LibraryToolbox &tb) const
{
    for (auto child : this->get_children()) {
        child->set_library_toolbox(tb);
    }
}

std::pair<bool, Prover<CheckpointedProofEngine> > TWff<PropTag>::get_adv_truth_prover(const LibraryToolbox &tb) const
{
#ifdef LOG_WFF
    cerr << "WFF proving: " << this->to_string() << endl;
#endif
    ptwff<Tag> not_imp = this->imp_not_form();
#ifdef LOG_WFF
    cerr << "not_imp form: " << not_imp->to_string() << endl;
#endif
    ptvar_set<Tag> vars;
    this->get_variables(vars);
    auto real = not_imp->adv_truth_internal(vars.begin(), vars.end(), tb);
    if (!real.first) {
        return make_pair(false, null_prover);
    }
    auto equiv = this->get_imp_not_prover(tb);
    auto final = tb.build_registered_prover< CheckpointedProofEngine >(TWff::adv_truth_4_rp, {{"ph", this->get_type_prover(tb)}, {"ps", not_imp->get_type_prover(tb)}}, {real.second, equiv});
    return make_pair(true, final);
}

std::pair<bool, Prover<CheckpointedProofEngine> > TWff<PropTag>::adv_truth_internal(typename ptvar_set<Tag>::iterator cur_var, typename ptvar_set<Tag>::iterator end_var, const LibraryToolbox &tb) const {
    if (cur_var == end_var) {
        return make_pair(this->is_true(), this->get_truth_prover(tb));
    } else {
        const auto &var = *cur_var++;
        ptwff<Tag> pos_wff = this->subst(var, true);
        ptwff<Tag> neg_wff = this->subst(var, false);
        ptwff<Tag> pos_antecent = var;
        ptwff<Tag> neg_antecent = TNot<Tag>::create(var);
        auto rec_pos_prover = pos_wff->adv_truth_internal(cur_var, end_var, tb);
        auto rec_neg_prover = neg_wff->adv_truth_internal(cur_var, end_var, tb);
        if (!rec_pos_prover.first || !rec_neg_prover.first) {
            return make_pair(false, null_prover);
        }
        auto pos_prover = this->get_subst_prover(var, true, tb);
        auto neg_prover = this->get_subst_prover(var, false, tb);
        auto pos_mp_prover = tb.build_registered_prover< CheckpointedProofEngine >(TWff::adv_truth_1_rp,
        {{"ph", pos_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}, {"ch", pos_wff->get_type_prover(tb)}},
        {rec_pos_prover.second, pos_prover});
        auto neg_mp_prover = tb.build_registered_prover< CheckpointedProofEngine >(TWff::adv_truth_2_rp,
        {{"ph", neg_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}, {"ch", neg_wff->get_type_prover(tb)}},
        {rec_neg_prover.second, neg_prover});
        auto final = tb.build_registered_prover< CheckpointedProofEngine >(TWff::adv_truth_3_rp,
        {{"ph", pos_antecent->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}},
        {pos_mp_prover, neg_mp_prover});
        return make_pair(true, final);
    }
}

const RegisteredProver TWff<PropTag>::adv_truth_1_rp = LibraryToolbox::register_prover({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )");
const RegisteredProver TWff<PropTag>::adv_truth_2_rp = LibraryToolbox::register_prover({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )");
const RegisteredProver TWff<PropTag>::adv_truth_3_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( -. ph -> ps )"}, "|- ps");
const RegisteredProver TWff<PropTag>::adv_truth_4_rp = LibraryToolbox::register_prover({"|- ps", "|- ( ph <-> ps )"}, "|- ph");
const RegisteredProver TWff<PropTag>::id_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ph )");

ptwff<PropTag> ConvertibleTWff<PropTag>::subst(ptvar<Tag> var, bool positive) const
{
    return this->imp_not_form()->subst(var, positive);
}

Prover<CheckpointedProofEngine> ConvertibleTWff<PropTag>::get_truth_prover(const LibraryToolbox &tb) const
{
    //return null_prover;
    return tb.build_registered_prover< CheckpointedProofEngine >(ConvertibleTWff::truth_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_truth_prover(tb)});
}

bool ConvertibleTWff<PropTag>::is_true() const
{
    return this->imp_not_form()->is_true();
}

Prover<CheckpointedProofEngine> ConvertibleTWff<PropTag>::get_falsity_prover(const LibraryToolbox &tb) const
{
    //return null_prover;
    return tb.build_registered_prover< CheckpointedProofEngine >(ConvertibleTWff::falsity_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_falsity_prover(tb)});
}

bool ConvertibleTWff<PropTag>::is_false() const
{
    return this->imp_not_form()->is_false();
}

const RegisteredProver ConvertibleTWff<PropTag>::truth_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- ph"}, "|- ps");
const RegisteredProver ConvertibleTWff<PropTag>::falsity_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- -. ph"}, "|- -. ps");

template<typename Tag>
std::string TTrueBase<Tag>::to_string() const {
    return "T.";
}

template<typename Tag>
Prover<CheckpointedProofEngine> TTrueBase<Tag>::get_truth_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TTrueBase::truth_rp, {}, {});
}

template<typename Tag>
bool TTrueBase<Tag>::is_true() const
{
    return true;
}

template<typename Tag>
Prover<CheckpointedProofEngine> TTrueBase<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TTrueBase::type_rp, {}, {});
}

template<typename Tag>
bool TTrueBase<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TTrueBase* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return true;
    }
}

template<typename Tag>
const RegisteredProver TTrueBase<Tag>::truth_rp = LibraryToolbox::register_prover({}, "|- T.");
template<typename Tag>
const RegisteredProver TTrueBase<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff T.");

void TTrue<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    cnf[{{true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TTrue::tseitin1_rp, {{"th", glob_ctx.get_type_prover(tb)}}, {});
}

ptwff<PropTag> TTrue<PropTag>::imp_not_form() const {
    return TTrue<Tag>::create();
}

ptwff<PropTag> TTrue<PropTag>::subst(ptvar<Tag> var, bool positive) const
{
    (void) var;
    (void) positive;
    return this->shared_from_this();
}

void TTrue<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    (void) vars;
}

Prover<CheckpointedProofEngine> TTrue<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TTrue::imp_not_rp, {}, {});
}

Prover<CheckpointedProofEngine> TTrue<PropTag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
{
    ptwff<Tag> subst = var;
    if (!positive) {
        subst = TNot<Tag>::create(subst);
    }
    return tb.build_registered_prover< CheckpointedProofEngine >(TTrue::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
}

const RegisteredProver TTrue<PropTag>::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( T. <-> T. )");
const RegisteredProver TTrue<PropTag>::subst_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( T. <-> T. ) )");
const RegisteredProver TTrue<PropTag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> T. )");

template<typename Tag>
std::string TFalseBase<Tag>::to_string() const {
    return "F.";
}

template<typename Tag>
Prover<CheckpointedProofEngine> TFalseBase<Tag>::get_falsity_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TFalseBase::falsity_rp, {}, {});
}

template<typename Tag>
bool TFalseBase<Tag>::is_false() const
{
    return true;
}

template<typename Tag>
Prover<CheckpointedProofEngine> TFalseBase<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TFalseBase::type_rp, {}, {});
}

template<typename Tag>
bool TFalseBase<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TFalseBase* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return true;
    }
}

template<typename Tag>
const RegisteredProver TFalseBase<Tag>::falsity_rp = LibraryToolbox::register_prover({}, "|- -. F.");
template<typename Tag>
const RegisteredProver TFalseBase<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff F.");

void TFalse<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    cnf[{{false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TFalse::tseitin1_rp, {{"th", glob_ctx.get_type_prover(tb)}}, {});
}

ptwff<PropTag> TFalse<PropTag>::imp_not_form() const {
    return this->shared_from_this();
}

ptwff<PropTag> TFalse<PropTag>::subst(ptvar<Tag> var, bool positive) const
{
    (void) var;
    (void) positive;
    return this->shared_from_this();
}

void TFalse<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    (void) vars;
}

Prover<CheckpointedProofEngine> TFalse<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TFalse::imp_not_rp, {}, {});
}

Prover<CheckpointedProofEngine> TFalse<PropTag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
{
    ptwff<Tag> subst = var;
    if (!positive) {
        subst = TNot<Tag>::create(subst);
    }
    return tb.build_registered_prover< CheckpointedProofEngine >(TFalse::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
}

const RegisteredProver TFalse<PropTag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> -. F. )");
const RegisteredProver TFalse<PropTag>::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( F. <-> F. )");
const RegisteredProver TFalse<PropTag>::subst_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( F. <-> F. ) )");

std::string TVar<PropTag>::to_string() const {
    return this->string_repr;
}

ptwff<PropTag> TVar<PropTag>::imp_not_form() const {
    return this->shared_from_this();
}

ptwff<PropTag> TVar<PropTag>::subst(ptvar<Tag> var, bool positive) const
{
    if (*this == *var) {
        if (positive) {
            return TTrue<Tag>::create();
        } else {
            return TFalse<Tag>::create();
        }
    } else {
        return this->shared_from_this();
    }
}

void TVar<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    vars.insert(this->shared_from_this());
}

Prover<CheckpointedProofEngine> TVar<PropTag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_type_prover(this->get_name());
    /*auto label = tb.get_var_sym_to_lab(tb.get_symbol(this->name));
    return [label](AbstractCheckpointedProofEngine &engine) {
        engine.process_label(label);
        return true;
    };*/
}

Prover<CheckpointedProofEngine> TVar<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TVar::imp_not_rp, {{"ph", this->get_type_prover(tb)}}, {});
}

Prover<CheckpointedProofEngine> TVar<PropTag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
{
    if (*this == *var) {
        if (positive) {
            auto first = tb.build_registered_prover< CheckpointedProofEngine >(TVar::subst_pos_1_rp, {{"ph", this->get_type_prover(tb)}}, {});
            auto truth = tb.build_registered_prover< CheckpointedProofEngine >(TVar::subst_pos_truth_rp, {}, {});
            auto second = tb.build_registered_prover< CheckpointedProofEngine >(TVar::subst_pos_2_rp, {{"ph", this->get_type_prover(tb)}}, {truth, first});
            auto third = tb.build_registered_prover< CheckpointedProofEngine >(TVar::subst_pos_3_rp, {{"ph", this->get_type_prover(tb)}}, {second});
            return third;
        } else {
            auto first = tb.build_registered_prover< CheckpointedProofEngine >(TVar::subst_neg_1_rp, {{"ph", this->get_type_prover(tb)}}, {});
            auto falsity = tb.build_registered_prover< CheckpointedProofEngine >(TVar::subst_neg_falsity_rp, {}, {});
            auto second = tb.build_registered_prover< CheckpointedProofEngine >(TVar::subst_neg_2_rp, {{"ph", this->get_type_prover(tb)}}, {falsity, first});
            auto third = tb.build_registered_prover< CheckpointedProofEngine >(TVar::subst_neg_3_rp, {{"ph", this->get_type_prover(tb)}}, {second});
            return third;
        }
    } else {
        ptwff<Tag> ant = var;
        if (!positive) {
            ant = TNot<Tag>::create(ant);
        }
        return tb.build_registered_prover< CheckpointedProofEngine >(TVar::subst_indep_rp, {{"ps", ant->get_type_prover(tb)}, {"ph", this->get_type_prover(tb)}}, {});
    }
}

bool TVar<PropTag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TVar* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return this->get_name() == px->get_name();
    }
}

bool TVar<PropTag>::operator<(const TVar &x) const {
    return this->get_name() < x.get_name();
}

void TVar<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    (void) cnf;
    (void) tb;
    (void) glob_ctx;
}

static decltype(auto) var_cons_helper(const std::string &string_repr, const LibraryToolbox &tb) {
    auto sent = tb.read_sentence(string_repr);
    auto pt = tb.parse_sentence(sent.begin(), sent.end(), tb.get_turnstile_alias());
    return pt_to_pt2(pt);
}

void TVar<PropTag>::set_library_toolbox(const LibraryToolbox &tb) const
{
    if (!this->name.quick_is_valid()) {
        this->name = var_cons_helper(string_repr, tb);
    }
}

const typename TVar<PropTag>::NameType &TVar<PropTag>::get_name() const {
    assert(this->name.quick_is_valid());
    return this->name;
}

TVar<PropTag>::TVar(const std::string &string_repr) : string_repr(string_repr)
{
}

TVar<PropTag>::TVar(TVar::NameType name, std::string string_repr) :
    name(name), string_repr(string_repr) {
}

TVar<PropTag>::TVar(const std::string &string_repr, const LibraryToolbox &tb) :
    name(var_cons_helper(string_repr, tb)), string_repr(string_repr) {
}

TVar<PropTag>::TVar(const TVar::NameType &name, const LibraryToolbox &tb) :
    name(name), string_repr(tb.print_sentence(tb.reconstruct_sentence(pt2_to_pt(name))).to_string()) {
}

const RegisteredProver TVar<PropTag>::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( ph <-> ph )");
const RegisteredProver TVar<PropTag>::subst_pos_1_rp = LibraryToolbox::register_prover({}, "|- ( T. -> ( ph -> ( T. <-> ph ) ) )");
const RegisteredProver TVar<PropTag>::subst_pos_2_rp = LibraryToolbox::register_prover({"|- T.", "|- ( T. -> ( ph -> ( T. <-> ph ) ) )"}, "|- ( ph -> ( T. <-> ph ) )");
const RegisteredProver TVar<PropTag>::subst_pos_3_rp = LibraryToolbox::register_prover({"|- ( ph -> ( T. <-> ph ) )"}, "|- ( ph -> ( ph <-> T. ) )");
const RegisteredProver TVar<PropTag>::subst_pos_truth_rp = LibraryToolbox::register_prover({}, "|- T.");
const RegisteredProver TVar<PropTag>::subst_neg_1_rp = LibraryToolbox::register_prover({}, "|- ( -. F. -> ( -. ph -> ( F. <-> ph ) ) )");
const RegisteredProver TVar<PropTag>::subst_neg_2_rp = LibraryToolbox::register_prover({"|- -. F.", "|- ( -. F. -> ( -. ph -> ( F. <-> ph ) ) )"}, "|- ( -. ph -> ( F. <-> ph ) )");
const RegisteredProver TVar<PropTag>::subst_neg_3_rp = LibraryToolbox::register_prover({"|- ( -. ph -> ( F. <-> ph ) )"}, "|- ( -. ph -> ( ph <-> F. ) )");
const RegisteredProver TVar<PropTag>::subst_neg_falsity_rp = LibraryToolbox::register_prover({}, "|- -. F.");
const RegisteredProver TVar<PropTag>::subst_indep_rp = LibraryToolbox::register_prover({}, "|- ( ps -> ( ph <-> ph ) )");

template<typename Tag>
std::string TNotBase<Tag>::to_string() const {
    return "-. " + this->a->to_string();
}

template<typename Tag>
Prover<CheckpointedProofEngine> TNotBase<Tag>::get_truth_prover(const LibraryToolbox &tb) const
{
    return this->a->get_falsity_prover(tb);
}

template<typename Tag>
bool TNotBase<Tag>::is_true() const
{
    return this->a->is_false();
}

template<typename Tag>
Prover<CheckpointedProofEngine> TNotBase<Tag>::get_falsity_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TNotBase::falsity_rp, {{ "ph", this->a->get_type_prover(tb) }}, { this->a->get_truth_prover(tb) });
}

template<typename Tag>
bool TNotBase<Tag>::is_false() const
{
    return this->a->is_true();
}

template<typename Tag>
Prover<CheckpointedProofEngine> TNotBase<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TNotBase::type_rp, {{ "ph", this->a->get_type_prover(tb) }}, {});
}

template<typename Tag>
bool TNotBase<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TNotBase* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a();
    }
}

template<typename Tag>
const RegisteredProver TNotBase<Tag>::falsity_rp = LibraryToolbox::register_prover({ "|- ph" }, "|- -. -. ph");
template<typename Tag>
const RegisteredProver TNotBase<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff -. ph");

void TNot<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNot::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNot::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

ptwff<PropTag> TNot<PropTag>::imp_not_form() const {
    return TNot<Tag>::create(this->a->imp_not_form());
}

ptwff<PropTag> TNot<PropTag>::subst(ptvar<Tag> var, bool positive) const
{
    return TNot<Tag>::create(this->a->subst(var, positive));
}

void TNot<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
}

Prover<CheckpointedProofEngine> TNot<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TNot::imp_not_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }}, { this->a->get_imp_not_prover(tb) });
}

Prover<CheckpointedProofEngine> TNot<PropTag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
{
    ptwff<Tag> ant;
    if (positive) {
        ant = var;
    } else {
        ant = TNot<Tag>::create(var);
    }
    return tb.build_registered_prover< CheckpointedProofEngine >(TNot::subst_rp, {{"ph", ant->get_type_prover(tb)}, {"ps", this->a->get_type_prover(tb)}, {"ch", this->a->subst(var, positive)->get_type_prover(tb)}}, {this->a->get_subst_prover(var, positive, tb)});
}

const RegisteredProver TNot<PropTag>::imp_not_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )"}, "|- ( -. ph <-> -. ps )");
const RegisteredProver TNot<PropTag>::subst_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ( -. ps <-> -. ch ) )");
const RegisteredProver TNot<PropTag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ph \\/ -. -. ph ) )");
const RegisteredProver TNot<PropTag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ph \\/ -. ph ) )");

template<typename Tag>
std::string TImpBase<Tag>::to_string() const {
    return "( " + this->a->to_string() + " -> " + this->b->to_string() + " )";
}

template<typename Tag>
Prover<CheckpointedProofEngine> TImpBase<Tag>::get_truth_prover(const LibraryToolbox &tb) const
{
    if (this->b->is_true()) {
        return tb.build_registered_prover< CheckpointedProofEngine >(TImpBase::truth_1_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, { this->b->get_truth_prover(tb) });
    }
    if (this->a->is_false()) {
        return tb.build_registered_prover< CheckpointedProofEngine >(TImpBase::truth_2_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, { this->a->get_falsity_prover(tb) });
    }
    return null_prover;
    //return cascade_provers(first_prover, second_prover);
}

template<typename Tag>
bool TImpBase<Tag>::is_true() const
{
    return this->b->is_true() || this->a->is_false();
}

template<typename Tag>
Prover<CheckpointedProofEngine> TImpBase<Tag>::get_falsity_prover(const LibraryToolbox &tb) const
{
    auto theorem_prover = tb.build_registered_prover< CheckpointedProofEngine >(TImpBase::falsity_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto mp_prover1 = tb.build_registered_prover< CheckpointedProofEngine >(TImpBase::falsity_2_rp,
    {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { this->a->get_truth_prover(tb), theorem_prover });
    auto mp_prover2 = tb.build_registered_prover< CheckpointedProofEngine >(TImpBase::falsity_3_rp,
    {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { mp_prover1, this->b->get_falsity_prover(tb) });
    return mp_prover2;
}

template<typename Tag>
bool TImpBase<Tag>::is_false() const
{
    return this->a->is_true() && this->b->is_false();
}

template<typename Tag>
Prover<CheckpointedProofEngine> TImpBase<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TImpBase::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
bool TImpBase<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TImpBase* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
const RegisteredProver TImpBase<Tag>::truth_1_rp = LibraryToolbox::register_prover({ "|- ps" }, "|- ( ph -> ps )");
template<typename Tag>
const RegisteredProver TImpBase<Tag>::truth_2_rp = LibraryToolbox::register_prover({ "|- -. ph" }, "|- ( ph -> ps )");
template<typename Tag>
const RegisteredProver TImpBase<Tag>::falsity_1_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )");
template<typename Tag>
const RegisteredProver TImpBase<Tag>::falsity_2_rp = LibraryToolbox::register_prover({ "|- ph", "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )"}, "|- ( -. ps -> -. ( ph -> ps ) )");
template<typename Tag>
const RegisteredProver TImpBase<Tag>::falsity_3_rp = LibraryToolbox::register_prover({ "|- ( -. ps -> -. ( ph -> ps ) )", "|- -. ps"}, "|- -. ( ph -> ps )");
template<typename Tag>
const RegisteredProver TImpBase<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph -> ps )");

void TImp<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TImp::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TImp::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TImp::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

ptwff<PropTag> TImp<PropTag>::imp_not_form() const {
    return TImp<Tag>::create(this->a->imp_not_form(), this->b->imp_not_form());
}

ptwff<PropTag> TImp<PropTag>::subst(ptvar<Tag> var, bool positive) const
{
    return TImp<Tag>::create(this->a->subst(var, positive), this->b->subst(var, positive));
}

void TImp<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

Prover<CheckpointedProofEngine> TImp<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TImp::imp_not_rp,
    {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }, {"ch", this->b->get_type_prover(tb)}, {"th", this->b->imp_not_form()->get_type_prover(tb)}},
    { this->a->get_imp_not_prover(tb), this->b->get_imp_not_prover(tb) });
}

Prover<CheckpointedProofEngine> TImp<PropTag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
{
    ptwff<Tag> ant;
    if (positive) {
        ant = var;
    } else {
        ant = TNot<Tag>::create(var);
    }
    return tb.build_registered_prover< CheckpointedProofEngine >(TImp::subst_rp,
    {{"ph", ant->get_type_prover(tb)}, {"ps", this->a->get_type_prover(tb)}, {"ch", this->a->subst(var, positive)->get_type_prover(tb)}, {"th", this->b->get_type_prover(tb)}, {"ta", this->b->subst(var, positive)->get_type_prover(tb)}},
    {this->a->get_subst_prover(var, positive, tb), this->b->get_subst_prover(var, positive, tb)});
}

const RegisteredProver TImp<PropTag>::imp_not_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ch <-> th )"}, "|- ( ( ph -> ch ) <-> ( ps -> th ) )");
const RegisteredProver TImp<PropTag>::subst_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps -> th ) <-> ( ch -> ta ) ) )");
const RegisteredProver TImp<PropTag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ ps ) \\/ -. ( ph -> ps ) ) )");
const RegisteredProver TImp<PropTag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ph \\/ ( ph -> ps ) ) )");
const RegisteredProver TImp<PropTag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ps \\/ ( ph -> ps ) ) )");

template<typename Tag>
std::string TBiimpBase<Tag>::to_string() const {
    return "( " + this->a->to_string() + " <-> " + this->b->to_string() + " )";
}

template<typename Tag>
Prover<CheckpointedProofEngine> TBiimpBase<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TBiimpBase::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
bool TBiimpBase<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TBiimpBase* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
const RegisteredProver TBiimpBase<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph <-> ps )");

void TBiimp<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin4_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

ptwff<PropTag> TBiimp<PropTag>::imp_not_form() const {
    auto ain = this->a->imp_not_form();
    auto bin = this->b->imp_not_form();
    return TNot<Tag>::create(TImp<Tag>::create(TImp<Tag>::create(ain, bin), TNot<Tag>::create(TImp<Tag>::create(bin, ain))));
}

ptwff<PropTag> TBiimp<PropTag>::half_imp_not_form() const
{
    auto ain = this->a;
    auto bin = this->b;
    return TNot<Tag>::create(TImp<Tag>::create(TImp<Tag>::create(ain, bin), TNot<Tag>::create(TImp<Tag>::create(bin, ain))));
}

void TBiimp<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

Prover<CheckpointedProofEngine> TBiimp<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TBiimp::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TBiimp::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

const RegisteredProver TBiimp<PropTag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph <-> ps ) <-> -. ( ( ph -> ps ) -> -. ( ps -> ph ) ) )");
const RegisteredProver TBiimp<PropTag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
const RegisteredProver TBiimp<PropTag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ -. ps ) \\/ ( ph <-> ps ) ) )");
const RegisteredProver TBiimp<PropTag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ ps ) \\/ ( ph <-> ps ) ) )");
const RegisteredProver TBiimp<PropTag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ -. ps ) \\/ -. ( ph <-> ps ) ) )");
const RegisteredProver TBiimp<PropTag>::tseitin4_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ ps ) \\/ -. ( ph <-> ps ) ) )");

template<typename Tag>
std::string TAndBase<Tag>::to_string() const {
    return "( " + this->a->to_string() + " /\\ " + this->b->to_string() + " )";
}

template<typename Tag>
Prover<CheckpointedProofEngine> TAndBase<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TAndBase::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
bool TAndBase<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TAndBase* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
const RegisteredProver TAndBase<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph /\\ ps )");

void TAnd<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

ptwff<PropTag> TAnd<PropTag>::imp_not_form() const
{
    return TNot<Tag>::create(TImp<Tag>::create(this->a->imp_not_form(), TNot<Tag>::create(this->b->imp_not_form())));
}

ptwff<PropTag> TAnd<PropTag>::half_imp_not_form() const {
    return TNot<Tag>::create(TImp<Tag>::create(this->a, TNot<Tag>::create(this->b)));
}

void TAnd<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

Prover<CheckpointedProofEngine> TAnd<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TAnd::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TAnd::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

const RegisteredProver TAnd<PropTag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph /\\ ps ) <-> -. ( ph -> -. ps ) )");
const RegisteredProver TAnd<PropTag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
const RegisteredProver TAnd<PropTag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ -. ps ) \\/ ( ph /\\ ps ) ) )");
const RegisteredProver TAnd<PropTag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ph \\/ -. ( ph /\\ ps ) ) )");
const RegisteredProver TAnd<PropTag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ps \\/ -. ( ph /\\ ps ) ) )");

template<typename Tag>
std::string TOrBase<Tag>::to_string() const {
    return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " )";
}

template<typename Tag>
Prover<CheckpointedProofEngine> TOrBase<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TOrBase::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
bool TOrBase<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TOrBase* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
const RegisteredProver TOrBase<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/ ps )");

void TOr<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

ptwff<PropTag> TOr<PropTag>::imp_not_form() const {
    return TImp<Tag>::create(TNot<Tag>::create(this->a->imp_not_form()), this->b->imp_not_form());
}

ptwff<PropTag> TOr<PropTag>::half_imp_not_form() const
{
    return TImp<Tag>::create(TNot<Tag>::create(this->a), this->b);
}

void TOr<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

Prover<CheckpointedProofEngine> TOr<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TOr::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TOr::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

const RegisteredProver TOr<PropTag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/ ps ) <-> ( -. ph -> ps ) )");
const RegisteredProver TOr<PropTag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
const RegisteredProver TOr<PropTag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ ps ) \\/ -. ( ph \\/ ps ) ) )");
const RegisteredProver TOr<PropTag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ph \\/ ( ph \\/ ps ) ) )");
const RegisteredProver TOr<PropTag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ps \\/ ( ph \\/ ps ) ) )");

template<typename Tag>
std::string TNandBase<Tag>::to_string() const {
    return "( " + this->a->to_string() + " -/\\ " + this->b->to_string() + " )";
}

template<typename Tag>
Prover<CheckpointedProofEngine> TNandBase<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TNandBase::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
bool TNandBase<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TNandBase* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
const RegisteredProver TNandBase<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph -/\\ ps )");

void TNand<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNand::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNand::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNand::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

ptwff<PropTag> TNand<PropTag>::imp_not_form() const {
    return TNot<Tag>::create(TAnd<Tag>::create(this->a->imp_not_form(), this->b->imp_not_form()))->imp_not_form();
}

ptwff<PropTag> TNand<PropTag>::half_imp_not_form() const
{
    return TNot<Tag>::create(TAnd<Tag>::create(this->a, this->b));
}

void TNand<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

Prover<CheckpointedProofEngine> TNand<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TNand::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TNand::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

const RegisteredProver TNand<PropTag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph -/\\ ps ) <-> -. ( ph /\\ ps ) )");
const RegisteredProver TNand<PropTag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
const RegisteredProver TNand<PropTag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ -. ps ) \\/ -. ( ph -/\\ ps ) ) )");
const RegisteredProver TNand<PropTag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ph \\/ ( ph -/\\ ps ) ) )");
const RegisteredProver TNand<PropTag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ps \\/ ( ph -/\\ ps ) ) )");

template<typename Tag>
std::string TXorBase<Tag>::to_string() const {
    return "( " + this->a->to_string() + " \\/_ " + this->b->to_string() + " )";
}

template<typename Tag>
Prover<CheckpointedProofEngine> TXorBase<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TXorBase::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
bool TXorBase<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TXorBase* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
const RegisteredProver TXorBase<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/_ ps )");

void TXor<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin4_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

ptwff<PropTag> TXor<PropTag>::imp_not_form() const {
    return TNot<Tag>::create(TBiimp<Tag>::create(this->a->imp_not_form(), this->b->imp_not_form()))->imp_not_form();
}

ptwff<PropTag> TXor<PropTag>::half_imp_not_form() const
{
    return TNot<Tag>::create(TBiimp<Tag>::create(this->a, this->b));
}

void TXor<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

Prover<CheckpointedProofEngine> TXor<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TXor::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TXor::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

const RegisteredProver TXor<PropTag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/_ ps ) <-> -. ( ph <-> ps ) )");
const RegisteredProver TXor<PropTag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
const RegisteredProver TXor<PropTag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ -. ps ) \\/ -. ( ph \\/_ ps ) ) )");
const RegisteredProver TXor<PropTag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ ps ) \\/ -. ( ph \\/_ ps ) ) )");
const RegisteredProver TXor<PropTag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ -. ps ) \\/ ( ph \\/_ ps ) ) )");
const RegisteredProver TXor<PropTag>::tseitin4_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ ps ) \\/ ( ph \\/_ ps ) ) )");

template<typename Tag>
std::string TAnd3Base<Tag>::to_string() const
{
    return "( " + this->a->to_string() + " /\\ " + this->b->to_string() + " /\\ " + this->c->to_string() + " )";
}

template<typename Tag>
Prover<CheckpointedProofEngine> TAnd3Base<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TAnd3Base::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
}

template<typename Tag>
bool TAnd3Base<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TAnd3Base* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b() && *this->get_c() == *px->get_c();
    }
}

template<typename Tag>
const RegisteredProver TAnd3Base<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph /\\ ps /\\ ch )");

void TAnd3<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_c()->get_tseitin_form(cnf, tb, glob_ctx);

    auto intermediate = TAnd<Tag>::create(this->get_a(), this->get_b());

    // intermediate = ( a /\ b )
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, intermediate->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd3::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {false, intermediate->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd3::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_b()->get_tseitin_var(tb)}, {false, intermediate->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd3::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});

    // this = ( intermediate /\ c )
    cnf[{{false, intermediate->get_tseitin_var(tb)}, {false, this->get_c()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd3::tseitin4_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, intermediate->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd3::tseitin5_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_c()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd3::tseitin6_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

ptwff<PropTag> TAnd3<PropTag>::imp_not_form() const
{
    return TAnd<Tag>::create(TAnd<Tag>::create(this->a, this->b), this->c)->imp_not_form();
}

ptwff<PropTag> TAnd3<PropTag>::half_imp_not_form() const
{
    return TAnd<Tag>::create(TAnd<Tag>::create(this->a, this->b), this->c);
}

void TAnd3<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
    this->c->get_variables(vars);
}

Prover<CheckpointedProofEngine> TAnd3<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TAnd3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TAnd3::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

const RegisteredProver TAnd3<PropTag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph /\\ ps /\\ ch ) <-> ( ( ph /\\ ps ) /\\ ch ) )");
const RegisteredProver TAnd3<PropTag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
const RegisteredProver TAnd3<PropTag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ -. ps ) \\/ ( ph /\\ ps ) ) )");
const RegisteredProver TAnd3<PropTag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ph \\/ -. ( ph /\\ ps ) ) )");
const RegisteredProver TAnd3<PropTag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ps \\/ -. ( ph /\\ ps ) ) )");
const RegisteredProver TAnd3<PropTag>::tseitin4_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ( ph /\\ ps ) \\/ -. ch ) \\/ ( ph /\\ ps /\\ ch ) ) )");
const RegisteredProver TAnd3<PropTag>::tseitin5_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph /\\ ps ) \\/ -. ( ph /\\ ps /\\ ch ) ) )");
const RegisteredProver TAnd3<PropTag>::tseitin6_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ch \\/ -. ( ph /\\ ps /\\ ch ) ) )");

template<typename Tag>
std::string TOr3Base<Tag>::to_string() const
{
    return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " \\/ " + this->c->to_string() + " )";
}

template<typename Tag>
Prover<CheckpointedProofEngine> TOr3Base<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TOr3Base::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
}

template<typename Tag>
bool TOr3Base<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TOr3Base* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b() && *this->get_c() == *px->get_c();
    }
}

template<typename Tag>
const RegisteredProver TOr3Base<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/ ps \\/ ch )");

void TOr3<PropTag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_c()->get_tseitin_form(cnf, tb, glob_ctx);

    auto intermediate = TOr<Tag>::create(this->get_a(), this->get_b());

    // intermediate = ( a \/ b )
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, intermediate->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr3::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, intermediate->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr3::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_b()->get_tseitin_var(tb)}, {true, intermediate->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr3::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});

    // this = ( intermediate \/ c )
    cnf[{{true, intermediate->get_tseitin_var(tb)}, {true, this->get_c()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr3::tseitin4_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, intermediate->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr3::tseitin5_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_c()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr3::tseitin6_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"ch", this->get_c()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

ptwff<PropTag> TOr3<PropTag>::imp_not_form() const
{
    return TOr<Tag>::create(TOr<Tag>::create(this->a, this->b), this->c)->imp_not_form();
}

ptwff<PropTag> TOr3<PropTag>::half_imp_not_form() const
{
    return TOr<Tag>::create(TOr<Tag>::create(this->a, this->b), this->c);
}

void TOr3<PropTag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
    this->c->get_variables(vars);
}

Prover<CheckpointedProofEngine> TOr3<PropTag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TOr3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TOr3::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

const RegisteredProver TOr3<PropTag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/ ps \\/ ch ) <-> ( ( ph \\/ ps ) \\/ ch ) )");
const RegisteredProver TOr3<PropTag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
const RegisteredProver TOr3<PropTag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ ps ) \\/ -. ( ph \\/ ps ) ) )");
const RegisteredProver TOr3<PropTag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ph \\/ ( ph \\/ ps ) ) )");
const RegisteredProver TOr3<PropTag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ps \\/ ( ph \\/ ps ) ) )");
const RegisteredProver TOr3<PropTag>::tseitin4_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ( ph \\/ ps ) \\/ ch ) \\/ -. ( ph \\/ ps \\/ ch ) ) )");
const RegisteredProver TOr3<PropTag>::tseitin5_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ( ph \\/ ps ) \\/ ( ph \\/ ps \\/ ch ) ) )");
const RegisteredProver TOr3<PropTag>::tseitin6_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ch \\/ ( ph \\/ ps \\/ ch ) ) )");

std::string TForall<PredTag>::to_string() const
{
    return "A. " + this->var_string + " " + this->get_a()->to_string();
}

bool TForall<PredTag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TForall* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return this->var == px->var && *this->get_a() == *px->get_a();
    }
}

const RegisteredProver TForall<PredTag>::type_rp = LibraryToolbox::register_prover({}, "wff A. x ph");

std::string TExists<PredTag>::to_string() const
{
    return "A. " + this->var_string + " " + this->get_a()->to_string();
}

bool TExists<PredTag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TExists* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return this->var == px->var && *this->get_a() == *px->get_a();
    }
}
const RegisteredProver TExists<PredTag>::type_rp = LibraryToolbox::register_prover({}, "wff E. x ph");

template class TWffBase<PropTag>;
template class TWffBase<PredTag>;
template class TWff<PropTag>;
template class TWff<PredTag>;
template class TVar<PropTag>;
template class TTrue<PropTag>;
template class TTrue<PredTag>;
template class TFalse<PropTag>;
template class TFalse<PredTag>;
template class TNot<PropTag>;
template class TNot<PredTag>;
template class TImp<PropTag>;
template class TImp<PredTag>;
template class TBiimp<PropTag>;
template class TBiimp<PredTag>;
template class TAnd<PropTag>;
template class TAnd<PredTag>;
template class TOr<PropTag>;
template class TOr<PredTag>;
template class TNand<PropTag>;
template class TNand<PredTag>;
template class TXor<PropTag>;
template class TXor<PredTag>;
template class TAnd3<PropTag>;
template class TAnd3<PredTag>;
template class TOr3<PropTag>;
template class TOr3<PredTag>;
template class TForall<PredTag>;

template<typename Tag>
bool wff_from_pt_int(ptwff<Tag> &ret, const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);

template<>
bool wff_from_pt_int<PropTag>(ptwff<PropTag> &ret, const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb) {
    ret = TVar<PropTag>::create(pt_to_pt2(pt), tb);
    return true;
}

template<>
bool wff_from_pt_int<PredTag>(ptwff<PredTag> &ret, const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb) {
    if (pt.label == tb.get_registered_prover_label(TForall<PredTag>::type_rp)) {
        assert(pt.children.size() == 2);
        assert(pt.children[0].children.empty());
        ret = TForall<PredTag>::create(pt.children[0].label, wff_from_pt<PredTag>(pt.children[1], tb), tb);
        return true;
    } else if (pt.label == tb.get_registered_prover_label(TExists<PredTag>::type_rp)) {
        assert(pt.children.size() == 2);
        assert(pt.children[0].children.empty());
        ret = TExists<PredTag>::create(pt.children[0].label, wff_from_pt<PredTag>(pt.children[1], tb), tb);
        return true;
    } else {
        return false;
    }
}

template<typename Tag>
ptwff<Tag> wff_from_pt(const ParsingTree< SymTok, LabTok > &pt, const LibraryToolbox &tb)
{
    assert(tb.resolve_symbol(pt.type) == "wff");
    if (pt.label == tb.get_registered_prover_label(TTrue<Tag>::type_rp)) {
        assert(pt.children.size() == 0);
        return TTrue<Tag>::create();
    } else if (pt.label == tb.get_registered_prover_label(TFalse<Tag>::type_rp)) {
        assert(pt.children.size() == 0);
        return TFalse<Tag>::create();
    } else if (pt.label == tb.get_registered_prover_label(TNot<Tag>::type_rp)) {
        assert(pt.children.size() == 1);
        return TNot<Tag>::create(wff_from_pt<Tag>(pt.children[0], tb));
    } else if (pt.label == tb.get_registered_prover_label(TImp<Tag>::type_rp)) {
        assert(pt.children.size() == 2);
        return TImp<Tag>::create(wff_from_pt<Tag>(pt.children[0], tb), wff_from_pt<Tag>(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(TBiimp<Tag>::type_rp)) {
        assert(pt.children.size() == 2);
        return TBiimp<Tag>::create(wff_from_pt<Tag>(pt.children[0], tb), wff_from_pt<Tag>(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(TAnd<Tag>::type_rp)) {
        assert(pt.children.size() == 2);
        return TAnd<Tag>::create(wff_from_pt<Tag>(pt.children[0], tb), wff_from_pt<Tag>(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(TOr<Tag>::type_rp)) {
        assert(pt.children.size() == 2);
        return TOr<Tag>::create(wff_from_pt<Tag>(pt.children[0], tb), wff_from_pt<Tag>(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(TNand<Tag>::type_rp)) {
        assert(pt.children.size() == 2);
        return TNand<Tag>::create(wff_from_pt<Tag>(pt.children[0], tb), wff_from_pt<Tag>(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(TXor<Tag>::type_rp)) {
        assert(pt.children.size() == 2);
        return TXor<Tag>::create(wff_from_pt<Tag>(pt.children[0], tb), wff_from_pt<Tag>(pt.children[1], tb));
    } else if (pt.label == tb.get_registered_prover_label(TAnd3<Tag>::type_rp)) {
        assert(pt.children.size() == 3);
        return TAnd3<Tag>::create(wff_from_pt<Tag>(pt.children[0], tb), wff_from_pt<Tag>(pt.children[1], tb), wff_from_pt<Tag>(pt.children[2], tb));
    } else if (pt.label == tb.get_registered_prover_label(TOr3<Tag>::type_rp)) {
        assert(pt.children.size() == 3);
        return TOr3<Tag>::create(wff_from_pt<Tag>(pt.children[0], tb), wff_from_pt<Tag>(pt.children[1], tb), wff_from_pt<Tag>(pt.children[2], tb));
    } else {
        ptwff<Tag> ret;
        if (wff_from_pt_int<Tag>(ret, pt, tb)) {
            return ret;
        } else {
            assert(!"Cannot reconstruct wff from parsing tree");
        }
    }
}

template ptwff<PropTag> wff_from_pt<PropTag>(const ParsingTree< SymTok, LabTok > &pt, const LibraryToolbox &tb);
template ptwff<PredTag> wff_from_pt<PredTag>(const ParsingTree< SymTok, LabTok > &pt, const LibraryToolbox &tb);

template<typename Tag>
bool ptvar_comp<Tag>::operator()(const ptvar<Tag> &x, const ptvar<Tag> &y) const { return *x < *y; }

template<typename T, typename Tag>
bool ptvar_pair_comp<T, Tag>::operator()(const std::pair<T, ptvar<Tag> > &x, const std::pair<T, ptvar<Tag> > &y) const {
    if (x.first < y.first) {
        return true;
    }
    if (y.first < x.first) {
        return false;
    }
    return ptvar_comp<Tag>()(x.second, y.second);
}

template<typename Tag>
ptvar_set<Tag> collect_tseitin_vars(const CNForm<Tag> &cnf)
{
    ptvar_set<Tag> ret;
    for (const auto &x : cnf) {
        for (const auto &y : x.first) {
            ret.insert(y.second);
        }
    }
    return ret;
}

template ptvar_set<PropTag> collect_tseitin_vars<PropTag>(const CNForm<PropTag> &cnf);

template<typename Tag>
ptvar_map< uint32_t, Tag > build_tseitin_map(const ptvar_set<Tag> &vars)
{
    ptvar_map< uint32_t, Tag > ret;
    uint32_t idx = 0;
    for (const auto &var : vars) {
        ret[var] = idx;
        idx++;
    }
    return ret;
}

template ptvar_map< uint32_t, PropTag > build_tseitin_map<PropTag>(const ptvar_set<PropTag> &vars);

template<typename Tag>
std::pair<CNFProblem, std::vector<Prover<CheckpointedProofEngine> > > build_cnf_problem(const CNForm<Tag> &cnf, const ptvar_map< uint32_t, Tag > &var_map)
{
    CNFProblem ret;
    std::vector< Prover< CheckpointedProofEngine > > provers;
    ret.var_num = var_map.size();
    for (const auto &clause : cnf) {
        ret.clauses.emplace_back();
        provers.push_back(clause.second);
        for (const auto &term : clause.first) {
            ret.clauses.back().push_back(std::make_pair(term.first, var_map.at(term.second)));
        }
    }
    return std::make_pair(ret, provers);
}

template std::pair<CNFProblem, std::vector<Prover<CheckpointedProofEngine> > > build_cnf_problem<PropTag>(const CNForm<PropTag> &cnf, const ptvar_map< uint32_t, PropTag > &var_map);
