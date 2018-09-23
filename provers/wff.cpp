
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

template<typename Tag>
Prover<CheckpointedProofEngine> TWffBase<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    (void) tb;
    return null_prover;
}

template<typename Tag>
Prover<CheckpointedProofEngine> TWffBase<Tag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
{
    (void) var;
    (void) positive;
    (void) tb;
    return null_prover;
}

template<typename Tag>
ptvar<Tag> TWff<Tag>::get_tseitin_var(const LibraryToolbox &tb) const {
    return TVar<Tag>::create(this->to_parsing_tree(tb), tb);
}

template<typename Tag>
std::tuple<CNFProblem, ptvar_map<uint32_t, Tag>, std::vector<Prover<CheckpointedProofEngine> > > TWff<Tag>::get_tseitin_cnf_problem(const LibraryToolbox &tb) const {
    CNForm<Tag> cnf;
    this->get_tseitin_form(cnf, tb, *this);
    cnf[{{true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TWff::id_rp, {{"ph", this->get_type_prover(tb)}}, {});
    auto vars = collect_tseitin_vars(cnf);
    auto map = build_tseitin_map(vars);
    auto problem = build_cnf_problem(cnf, map);
    return make_tuple(problem.first, map, problem.second);
}

template<typename Tag>
void TWff<Tag>::set_library_toolbox(const LibraryToolbox &tb) const
{
    for (auto child : this->get_children()) {
        child->set_library_toolbox(tb);
    }
}

template<typename Tag>
std::pair<bool, Prover<CheckpointedProofEngine> > TWff<Tag>::get_adv_truth_prover(const LibraryToolbox &tb) const
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

template<typename Tag>
std::pair<bool, Prover<CheckpointedProofEngine> > TWff<Tag>::adv_truth_internal(typename ptvar_set<Tag>::iterator cur_var, typename ptvar_set<Tag>::iterator end_var, const LibraryToolbox &tb) const {
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

template<typename Tag>
const RegisteredProver TWff<Tag>::adv_truth_1_rp = LibraryToolbox::register_prover({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )");
template<typename Tag>
const RegisteredProver TWff<Tag>::adv_truth_2_rp = LibraryToolbox::register_prover({"|- ch", "|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ps )");
template<typename Tag>
const RegisteredProver TWff<Tag>::adv_truth_3_rp = LibraryToolbox::register_prover({"|- ( ph -> ps )", "|- ( -. ph -> ps )"}, "|- ps");
template<typename Tag>
const RegisteredProver TWff<Tag>::adv_truth_4_rp = LibraryToolbox::register_prover({"|- ps", "|- ( ph <-> ps )"}, "|- ph");
template<typename Tag>
const RegisteredProver TWff<Tag>::id_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ph )");

template<typename Tag>
ptwff<Tag> ConvertibleTWff<Tag>::subst(ptvar<Tag> var, bool positive) const
{
    return this->imp_not_form()->subst(var, positive);
}

template<typename Tag>
Prover<CheckpointedProofEngine> ConvertibleTWff<Tag>::get_truth_prover(const LibraryToolbox &tb) const
{
    //return null_prover;
    return tb.build_registered_prover< CheckpointedProofEngine >(ConvertibleTWff::truth_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_truth_prover(tb)});
}

template<typename Tag>
bool ConvertibleTWff<Tag>::is_true() const
{
    return this->imp_not_form()->is_true();
}

template<typename Tag>
Prover<CheckpointedProofEngine> ConvertibleTWff<Tag>::get_falsity_prover(const LibraryToolbox &tb) const
{
    //return null_prover;
    return tb.build_registered_prover< CheckpointedProofEngine >(ConvertibleTWff::falsity_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_falsity_prover(tb)});
}

template<typename Tag>
bool ConvertibleTWff<Tag>::is_false() const
{
    return this->imp_not_form()->is_false();
}

template<typename Tag>
const RegisteredProver ConvertibleTWff<Tag>::truth_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- ph"}, "|- ps");
template<typename Tag>
const RegisteredProver ConvertibleTWff<Tag>::falsity_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- -. ph"}, "|- -. ps");

template<typename Tag>
std::string TTrue<Tag>::to_string() const {
    return "T.";
}

template<typename Tag>
ptwff<Tag> TTrue<Tag>::imp_not_form() const {
    return TTrue::create();
}

template<typename Tag>
ptwff<Tag> TTrue<Tag>::subst(ptvar<Tag> var, bool positive) const
{
    (void) var;
    (void) positive;
    return this->shared_from_this();
}

template<typename Tag>
void TTrue<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    (void) vars;
}

template<typename Tag>
Prover<CheckpointedProofEngine> TTrue<Tag>::get_truth_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TTrue::truth_rp, {}, {});
}

template<typename Tag>
bool TTrue<Tag>::is_true() const
{
    return true;
}

template<typename Tag>
Prover<CheckpointedProofEngine> TTrue<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TTrue::type_rp, {}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TTrue<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TTrue::imp_not_rp, {}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TTrue<Tag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
{
    ptwff<Tag> subst = var;
    if (!positive) {
        subst = TNot<Tag>::create(subst);
    }
    return tb.build_registered_prover< CheckpointedProofEngine >(TTrue::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
}

template<typename Tag>
bool TTrue<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TTrue* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return true;
    }
}

template<typename Tag>
void TTrue<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    cnf[{{true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TTrue::tseitin1_rp, {{"th", glob_ctx.get_type_prover(tb)}}, {});
}

template<typename Tag>
const RegisteredProver TTrue<Tag>::truth_rp = LibraryToolbox::register_prover({}, "|- T.");
template<typename Tag>
const RegisteredProver TTrue<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff T.");
template<typename Tag>
const RegisteredProver TTrue<Tag>::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( T. <-> T. )");
template<typename Tag>
const RegisteredProver TTrue<Tag>::subst_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( T. <-> T. ) )");
template<typename Tag>
const RegisteredProver TTrue<Tag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> T. )");

template<typename Tag>
std::string TFalse<Tag>::to_string() const {
    return "F.";
}

template<typename Tag>
ptwff<Tag> TFalse<Tag>::imp_not_form() const {
    return TFalse::create();
}

template<typename Tag>
ptwff<Tag> TFalse<Tag>::subst(ptvar<Tag> var, bool positive) const
{
    (void) var;
    (void) positive;
    return this->shared_from_this();
}

template<typename Tag>
void TFalse<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    (void) vars;
}

template<typename Tag>
Prover<CheckpointedProofEngine> TFalse<Tag>::get_falsity_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TFalse::falsity_rp, {}, {});
}

template<typename Tag>
bool TFalse<Tag>::is_false() const
{
    return true;
}

template<typename Tag>
Prover<CheckpointedProofEngine> TFalse<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TFalse::type_rp, {}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TFalse<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TFalse::imp_not_rp, {}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TFalse<Tag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
{
    ptwff<Tag> subst = var;
    if (!positive) {
        subst = TNot<Tag>::create(subst);
    }
    return tb.build_registered_prover< CheckpointedProofEngine >(TFalse::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
}

template<typename Tag>
bool TFalse<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TFalse* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return true;
    }
}

template<typename Tag>
void TFalse<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    cnf[{{false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TFalse::tseitin1_rp, {{"th", glob_ctx.get_type_prover(tb)}}, {});
}

template<typename Tag>
const RegisteredProver TFalse<Tag>::falsity_rp = LibraryToolbox::register_prover({}, "|- -. F.");
template<typename Tag>
const RegisteredProver TFalse<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff F.");
template<typename Tag>
const RegisteredProver TFalse<Tag>::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( F. <-> F. )");
template<typename Tag>
const RegisteredProver TFalse<Tag>::subst_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( F. <-> F. ) )");
template<typename Tag>
const RegisteredProver TFalse<Tag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> -. F. )");

template<typename Tag>
std::string TVar<Tag>::to_string() const {
    return this->string_repr;
}

template<typename Tag>
ptwff<Tag> TVar<Tag>::imp_not_form() const {
    return this->shared_from_this();
}

template<typename Tag>
ptwff<Tag> TVar<Tag>::subst(ptvar<Tag> var, bool positive) const
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

template<typename Tag>
void TVar<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    vars.insert(this->shared_from_this());
}

template<typename Tag>
Prover<CheckpointedProofEngine> TVar<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_type_prover(this->get_name());
    /*auto label = tb.get_var_sym_to_lab(tb.get_symbol(this->name));
    return [label](AbstractCheckpointedProofEngine &engine) {
        engine.process_label(label);
        return true;
    };*/
}

template<typename Tag>
Prover<CheckpointedProofEngine> TVar<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TVar::imp_not_rp, {{"ph", this->get_type_prover(tb)}}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TVar<Tag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
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

template<typename Tag>
bool TVar<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TVar* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return this->get_name() == px->get_name();
    }
}

template<typename Tag>
bool TVar<Tag>::operator<(const TVar &x) const {
    return this->get_name() < x.get_name();
}

template<typename Tag>
void TVar<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
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

template<typename Tag>
void TVar<Tag>::set_library_toolbox(const LibraryToolbox &tb) const
{
    if (!this->name.quick_is_valid()) {
        this->name = var_cons_helper(string_repr, tb);
    }
}

template<typename Tag>
const typename TVar<Tag>::NameType &TVar<Tag>::get_name() const {
    assert(this->name.quick_is_valid());
    return this->name;
}

template<typename Tag>
TVar<Tag>::TVar(const std::string &string_repr) : string_repr(string_repr)
{
}

template<typename Tag>
TVar<Tag>::TVar(TVar::NameType name, std::string string_repr) :
    name(name), string_repr(string_repr) {
}

template<typename Tag>
TVar<Tag>::TVar(const std::string &string_repr, const LibraryToolbox &tb) :
    name(var_cons_helper(string_repr, tb)), string_repr(string_repr) {
}

template<typename Tag>
TVar<Tag>::TVar(const TVar::NameType &name, const LibraryToolbox &tb) :
    name(name), string_repr(tb.print_sentence(tb.reconstruct_sentence(pt2_to_pt(name))).to_string()) {
}

template<typename Tag>
const RegisteredProver TVar<Tag>::imp_not_rp = LibraryToolbox::register_prover({}, "|- ( ph <-> ph )");
template<typename Tag>
const RegisteredProver TVar<Tag>::subst_pos_1_rp = LibraryToolbox::register_prover({}, "|- ( T. -> ( ph -> ( T. <-> ph ) ) )");
template<typename Tag>
const RegisteredProver TVar<Tag>::subst_pos_2_rp = LibraryToolbox::register_prover({"|- T.", "|- ( T. -> ( ph -> ( T. <-> ph ) ) )"}, "|- ( ph -> ( T. <-> ph ) )");
template<typename Tag>
const RegisteredProver TVar<Tag>::subst_pos_3_rp = LibraryToolbox::register_prover({"|- ( ph -> ( T. <-> ph ) )"}, "|- ( ph -> ( ph <-> T. ) )");
template<typename Tag>
const RegisteredProver TVar<Tag>::subst_pos_truth_rp = LibraryToolbox::register_prover({}, "|- T.");
template<typename Tag>
const RegisteredProver TVar<Tag>::subst_neg_1_rp = LibraryToolbox::register_prover({}, "|- ( -. F. -> ( -. ph -> ( F. <-> ph ) ) )");
template<typename Tag>
const RegisteredProver TVar<Tag>::subst_neg_2_rp = LibraryToolbox::register_prover({"|- -. F.", "|- ( -. F. -> ( -. ph -> ( F. <-> ph ) ) )"}, "|- ( -. ph -> ( F. <-> ph ) )");
template<typename Tag>
const RegisteredProver TVar<Tag>::subst_neg_3_rp = LibraryToolbox::register_prover({"|- ( -. ph -> ( F. <-> ph ) )"}, "|- ( -. ph -> ( ph <-> F. ) )");
template<typename Tag>
const RegisteredProver TVar<Tag>::subst_neg_falsity_rp = LibraryToolbox::register_prover({}, "|- -. F.");
template<typename Tag>
const RegisteredProver TVar<Tag>::subst_indep_rp = LibraryToolbox::register_prover({}, "|- ( ps -> ( ph <-> ph ) )");

template<typename Tag>
std::string TNot<Tag>::to_string() const {
    return "-. " + this->a->to_string();
}

template<typename Tag>
ptwff<Tag> TNot<Tag>::imp_not_form() const {
    return TNot::create(this->a->imp_not_form());
}

template<typename Tag>
ptwff<Tag> TNot<Tag>::subst(ptvar<Tag> var, bool positive) const
{
    return TNot::create(this->a->subst(var, positive));
}

template<typename Tag>
void TNot<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
}

template<typename Tag>
Prover<CheckpointedProofEngine> TNot<Tag>::get_truth_prover(const LibraryToolbox &tb) const
{
    return this->a->get_falsity_prover(tb);
}

template<typename Tag>
bool TNot<Tag>::is_true() const
{
    return this->a->is_false();
}

template<typename Tag>
Prover<CheckpointedProofEngine> TNot<Tag>::get_falsity_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TNot::falsity_rp, {{ "ph", this->a->get_type_prover(tb) }}, { this->a->get_truth_prover(tb) });
}

template<typename Tag>
bool TNot<Tag>::is_false() const
{
    return this->a->is_true();
}

template<typename Tag>
Prover<CheckpointedProofEngine> TNot<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TNot::type_rp, {{ "ph", this->a->get_type_prover(tb) }}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TNot<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TNot::imp_not_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }}, { this->a->get_imp_not_prover(tb) });
}

template<typename Tag>
Prover<CheckpointedProofEngine> TNot<Tag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
{
    ptwff<Tag> ant;
    if (positive) {
        ant = var;
    } else {
        ant = TNot::create(var);
    }
    return tb.build_registered_prover< CheckpointedProofEngine >(TNot::subst_rp, {{"ph", ant->get_type_prover(tb)}, {"ps", this->a->get_type_prover(tb)}, {"ch", this->a->subst(var, positive)->get_type_prover(tb)}}, {this->a->get_subst_prover(var, positive, tb)});
}

template<typename Tag>
bool TNot<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TNot* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a();
    }
}

template<typename Tag>
void TNot<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNot::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNot::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

template<typename Tag>
const RegisteredProver TNot<Tag>::falsity_rp = LibraryToolbox::register_prover({ "|- ph" }, "|- -. -. ph");
template<typename Tag>
const RegisteredProver TNot<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff -. ph");
template<typename Tag>
const RegisteredProver TNot<Tag>::imp_not_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )"}, "|- ( -. ph <-> -. ps )");
template<typename Tag>
const RegisteredProver TNot<Tag>::subst_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )"}, "|- ( ph -> ( -. ps <-> -. ch ) )");
template<typename Tag>
const RegisteredProver TNot<Tag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ph \\/ -. -. ph ) )");
template<typename Tag>
const RegisteredProver TNot<Tag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ph \\/ -. ph ) )");

template<typename Tag>
std::string TImp<Tag>::to_string() const {
    return "( " + this->a->to_string() + " -> " + this->b->to_string() + " )";
}

template<typename Tag>
ptwff<Tag> TImp<Tag>::imp_not_form() const {
    return TImp::create(this->a->imp_not_form(), this->b->imp_not_form());
}

template<typename Tag>
ptwff<Tag> TImp<Tag>::subst(ptvar<Tag> var, bool positive) const
{
    return TImp::create(this->a->subst(var, positive), this->b->subst(var, positive));
}

template<typename Tag>
void TImp<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

template<typename Tag>
Prover<CheckpointedProofEngine> TImp<Tag>::get_truth_prover(const LibraryToolbox &tb) const
{
    if (this->b->is_true()) {
        return tb.build_registered_prover< CheckpointedProofEngine >(TImp::truth_1_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, { this->b->get_truth_prover(tb) });
    }
    if (this->a->is_false()) {
        return tb.build_registered_prover< CheckpointedProofEngine >(TImp::truth_2_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, { this->a->get_falsity_prover(tb) });
    }
    return null_prover;
    //return cascade_provers(first_prover, second_prover);
}

template<typename Tag>
bool TImp<Tag>::is_true() const
{
    return this->b->is_true() || this->a->is_false();
}

template<typename Tag>
Prover<CheckpointedProofEngine> TImp<Tag>::get_falsity_prover(const LibraryToolbox &tb) const
{
    auto theorem_prover = tb.build_registered_prover< CheckpointedProofEngine >(TImp::falsity_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto mp_prover1 = tb.build_registered_prover< CheckpointedProofEngine >(TImp::falsity_2_rp,
    {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { this->a->get_truth_prover(tb), theorem_prover });
    auto mp_prover2 = tb.build_registered_prover< CheckpointedProofEngine >(TImp::falsity_3_rp,
    {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { mp_prover1, this->b->get_falsity_prover(tb) });
    return mp_prover2;
}

template<typename Tag>
bool TImp<Tag>::is_false() const
{
    return this->a->is_true() && this->b->is_false();
}

template<typename Tag>
Prover<CheckpointedProofEngine> TImp<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TImp::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TImp<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TImp::imp_not_rp,
    {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }, {"ch", this->b->get_type_prover(tb)}, {"th", this->b->imp_not_form()->get_type_prover(tb)}},
    { this->a->get_imp_not_prover(tb), this->b->get_imp_not_prover(tb) });
}

template<typename Tag>
Prover<CheckpointedProofEngine> TImp<Tag>::get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
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

template<typename Tag>
bool TImp<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TImp* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
void TImp<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TImp::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TImp::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TImp::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

template<typename Tag>
const RegisteredProver TImp<Tag>::truth_1_rp = LibraryToolbox::register_prover({ "|- ps" }, "|- ( ph -> ps )");
template<typename Tag>
const RegisteredProver TImp<Tag>::truth_2_rp = LibraryToolbox::register_prover({ "|- -. ph" }, "|- ( ph -> ps )");
template<typename Tag>
const RegisteredProver TImp<Tag>::falsity_1_rp = LibraryToolbox::register_prover({}, "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )");
template<typename Tag>
const RegisteredProver TImp<Tag>::falsity_2_rp = LibraryToolbox::register_prover({ "|- ph", "|- ( ph -> ( -. ps -> -. ( ph -> ps ) ) )"}, "|- ( -. ps -> -. ( ph -> ps ) )");
template<typename Tag>
const RegisteredProver TImp<Tag>::falsity_3_rp = LibraryToolbox::register_prover({ "|- ( -. ps -> -. ( ph -> ps ) )", "|- -. ps"}, "|- -. ( ph -> ps )");
template<typename Tag>
const RegisteredProver TImp<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph -> ps )");
template<typename Tag>
const RegisteredProver TImp<Tag>::imp_not_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ch <-> th )"}, "|- ( ( ph -> ch ) <-> ( ps -> th ) )");
template<typename Tag>
const RegisteredProver TImp<Tag>::subst_rp = LibraryToolbox::register_prover({"|- ( ph -> ( ps <-> ch ) )", "|- ( ph -> ( th <-> ta ) )"}, "|- ( ph -> ( ( ps -> th ) <-> ( ch -> ta ) ) )");
template<typename Tag>
const RegisteredProver TImp<Tag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ ps ) \\/ -. ( ph -> ps ) ) )");
template<typename Tag>
const RegisteredProver TImp<Tag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ph \\/ ( ph -> ps ) ) )");
template<typename Tag>
const RegisteredProver TImp<Tag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ps \\/ ( ph -> ps ) ) )");

template<typename Tag>
std::string TBiimp<Tag>::to_string() const {
    return "( " + this->a->to_string() + " <-> " + this->b->to_string() + " )";
}

template<typename Tag>
ptwff<Tag> TBiimp<Tag>::imp_not_form() const {
    auto ain = this->a->imp_not_form();
    auto bin = this->b->imp_not_form();
    return TNot<Tag>::create(TImp<Tag>::create(TImp<Tag>::create(ain, bin), TNot<Tag>::create(TImp<Tag>::create(bin, ain))));
}

template<typename Tag>
ptwff<Tag> TBiimp<Tag>::half_imp_not_form() const
{
    auto ain = this->a;
    auto bin = this->b;
    return TNot<Tag>::create(TImp<Tag>::create(TImp<Tag>::create(ain, bin), TNot<Tag>::create(TImp<Tag>::create(bin, ain))));
}

template<typename Tag>
void TBiimp<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

template<typename Tag>
Prover<CheckpointedProofEngine> TBiimp<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TBiimp::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TBiimp<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TBiimp::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TBiimp::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

template<typename Tag>
bool TBiimp<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TBiimp* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
void TBiimp<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin4_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

template<typename Tag>
const RegisteredProver TBiimp<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph <-> ps )");
template<typename Tag>
const RegisteredProver TBiimp<Tag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph <-> ps ) <-> -. ( ( ph -> ps ) -> -. ( ps -> ph ) ) )");
template<typename Tag>
const RegisteredProver TBiimp<Tag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
template<typename Tag>
const RegisteredProver TBiimp<Tag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ -. ps ) \\/ ( ph <-> ps ) ) )");
template<typename Tag>
const RegisteredProver TBiimp<Tag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ ps ) \\/ ( ph <-> ps ) ) )");
template<typename Tag>
const RegisteredProver TBiimp<Tag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ -. ps ) \\/ -. ( ph <-> ps ) ) )");
template<typename Tag>
const RegisteredProver TBiimp<Tag>::tseitin4_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ ps ) \\/ -. ( ph <-> ps ) ) )");

template<typename Tag>
std::string TAnd<Tag>::to_string() const {
    return "( " + this->a->to_string() + " /\\ " + this->b->to_string() + " )";
}

template<typename Tag>
ptwff<Tag> TAnd<Tag>::imp_not_form() const
{
    return TNot<Tag>::create(TImp<Tag>::create(this->a->imp_not_form(), TNot<Tag>::create(this->b->imp_not_form())));
}

template<typename Tag>
ptwff<Tag> TAnd<Tag>::half_imp_not_form() const {
    return TNot<Tag>::create(TImp<Tag>::create(this->a, TNot<Tag>::create(this->b)));
}

template<typename Tag>
void TAnd<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

template<typename Tag>
Prover<CheckpointedProofEngine> TAnd<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TAnd::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TAnd<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TAnd::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TAnd::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

template<typename Tag>
bool TAnd<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TAnd* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
void TAnd<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

template<typename Tag>
const RegisteredProver TAnd<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph /\\ ps )");
template<typename Tag>
const RegisteredProver TAnd<Tag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph /\\ ps ) <-> -. ( ph -> -. ps ) )");
template<typename Tag>
const RegisteredProver TAnd<Tag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
template<typename Tag>
const RegisteredProver TAnd<Tag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ -. ps ) \\/ ( ph /\\ ps ) ) )");
template<typename Tag>
const RegisteredProver TAnd<Tag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ph \\/ -. ( ph /\\ ps ) ) )");
template<typename Tag>
const RegisteredProver TAnd<Tag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ps \\/ -. ( ph /\\ ps ) ) )");

template<typename Tag>
std::string TOr<Tag>::to_string() const {
    return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " )";
}

template<typename Tag>
ptwff<Tag> TOr<Tag>::imp_not_form() const {
    return TImp<Tag>::create(TNot<Tag>::create(this->a->imp_not_form()), this->b->imp_not_form());
}

template<typename Tag>
ptwff<Tag> TOr<Tag>::half_imp_not_form() const
{
    return TImp<Tag>::create(TNot<Tag>::create(this->a), this->b);
}

template<typename Tag>
void TOr<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

template<typename Tag>
Prover<CheckpointedProofEngine> TOr<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TOr::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TOr<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TOr::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TOr::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

template<typename Tag>
bool TOr<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TOr* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
void TOr<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

template<typename Tag>
const RegisteredProver TOr<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/ ps )");
template<typename Tag>
const RegisteredProver TOr<Tag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/ ps ) <-> ( -. ph -> ps ) )");
template<typename Tag>
const RegisteredProver TOr<Tag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
template<typename Tag>
const RegisteredProver TOr<Tag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ ps ) \\/ -. ( ph \\/ ps ) ) )");
template<typename Tag>
const RegisteredProver TOr<Tag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ph \\/ ( ph \\/ ps ) ) )");
template<typename Tag>
const RegisteredProver TOr<Tag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ps \\/ ( ph \\/ ps ) ) )");

template<typename Tag>
std::string TNand<Tag>::to_string() const {
    return "( " + this->a->to_string() + " -/\\ " + this->b->to_string() + " )";
}

template<typename Tag>
ptwff<Tag> TNand<Tag>::imp_not_form() const {
    return TNot<Tag>::create(TAnd<Tag>::create(this->a->imp_not_form(), this->b->imp_not_form()))->imp_not_form();
}

template<typename Tag>
ptwff<Tag> TNand<Tag>::half_imp_not_form() const
{
    return TNot<Tag>::create(TAnd<Tag>::create(this->a, this->b));
}

template<typename Tag>
void TNand<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

template<typename Tag>
Prover<CheckpointedProofEngine> TNand<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TNand::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TNand<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TNand::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TNand::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

template<typename Tag>
bool TNand<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TNand* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
void TNand<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNand::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNand::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNand::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

template<typename Tag>
const RegisteredProver TNand<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph -/\\ ps )");
template<typename Tag>
const RegisteredProver TNand<Tag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph -/\\ ps ) <-> -. ( ph /\\ ps ) )");
template<typename Tag>
const RegisteredProver TNand<Tag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
template<typename Tag>
const RegisteredProver TNand<Tag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ -. ps ) \\/ -. ( ph -/\\ ps ) ) )");
template<typename Tag>
const RegisteredProver TNand<Tag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ph \\/ ( ph -/\\ ps ) ) )");
template<typename Tag>
const RegisteredProver TNand<Tag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ps \\/ ( ph -/\\ ps ) ) )");

template<typename Tag>
std::string TXor<Tag>::to_string() const {
    return "( " + this->a->to_string() + " \\/_ " + this->b->to_string() + " )";
}

template<typename Tag>
ptwff<Tag> TXor<Tag>::imp_not_form() const {
    return TNot<Tag>::create(TBiimp<Tag>::create(this->a->imp_not_form(), this->b->imp_not_form()))->imp_not_form();
}

template<typename Tag>
ptwff<Tag> TXor<Tag>::half_imp_not_form() const
{
    return TNot<Tag>::create(TBiimp<Tag>::create(this->a, this->b));
}

template<typename Tag>
void TXor<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
}

template<typename Tag>
Prover<CheckpointedProofEngine> TXor<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TXor::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TXor<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TXor::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TXor::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

template<typename Tag>
bool TXor<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TXor* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
    }
}

template<typename Tag>
void TXor<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
{
    this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
    this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
    cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin4_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
}

template<typename Tag>
const RegisteredProver TXor<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/_ ps )");
template<typename Tag>
const RegisteredProver TXor<Tag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/_ ps ) <-> -. ( ph <-> ps ) )");
template<typename Tag>
const RegisteredProver TXor<Tag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
template<typename Tag>
const RegisteredProver TXor<Tag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ -. ps ) \\/ -. ( ph \\/_ ps ) ) )");
template<typename Tag>
const RegisteredProver TXor<Tag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ ps ) \\/ -. ( ph \\/_ ps ) ) )");
template<typename Tag>
const RegisteredProver TXor<Tag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ -. ps ) \\/ ( ph \\/_ ps ) ) )");
template<typename Tag>
const RegisteredProver TXor<Tag>::tseitin4_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ ps ) \\/ ( ph \\/_ ps ) ) )");

template<typename Tag>
std::string TAnd3<Tag>::to_string() const
{
    return "( " + this->a->to_string() + " /\\ " + this->b->to_string() + " /\\ " + this->c->to_string() + " )";
}

template<typename Tag>
ptwff<Tag> TAnd3<Tag>::imp_not_form() const
{
    return TAnd<Tag>::create(TAnd<Tag>::create(this->a, this->b), this->c)->imp_not_form();
}

template<typename Tag>
ptwff<Tag> TAnd3<Tag>::half_imp_not_form() const
{
    return TAnd<Tag>::create(TAnd<Tag>::create(this->a, this->b), this->c);
}

template<typename Tag>
void TAnd3<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
    this->c->get_variables(vars);
}

template<typename Tag>
Prover<CheckpointedProofEngine> TAnd3<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TAnd3::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TAnd3<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TAnd3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TAnd3::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

template<typename Tag>
bool TAnd3<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TAnd3* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b() && *this->get_c() == *px->get_c();
    }
}

template<typename Tag>
void TAnd3<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
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

template<typename Tag>
const RegisteredProver TAnd3<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph /\\ ps /\\ ch )");
template<typename Tag>
const RegisteredProver TAnd3<Tag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph /\\ ps /\\ ch ) <-> ( ( ph /\\ ps ) /\\ ch ) )");
template<typename Tag>
const RegisteredProver TAnd3<Tag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
template<typename Tag>
const RegisteredProver TAnd3<Tag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ph \\/ -. ps ) \\/ ( ph /\\ ps ) ) )");
template<typename Tag>
const RegisteredProver TAnd3<Tag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ph \\/ -. ( ph /\\ ps ) ) )");
template<typename Tag>
const RegisteredProver TAnd3<Tag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ps \\/ -. ( ph /\\ ps ) ) )");
template<typename Tag>
const RegisteredProver TAnd3<Tag>::tseitin4_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( -. ( ph /\\ ps ) \\/ -. ch ) \\/ ( ph /\\ ps /\\ ch ) ) )");
template<typename Tag>
const RegisteredProver TAnd3<Tag>::tseitin5_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph /\\ ps ) \\/ -. ( ph /\\ ps /\\ ch ) ) )");
template<typename Tag>
const RegisteredProver TAnd3<Tag>::tseitin6_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ch \\/ -. ( ph /\\ ps /\\ ch ) ) )");

template<typename Tag>
std::string TOr3<Tag>::to_string() const
{
    return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " \\/ " + this->c->to_string() + " )";
}

template<typename Tag>
ptwff<Tag> TOr3<Tag>::imp_not_form() const
{
    return TOr<Tag>::create(TOr<Tag>::create(this->a, this->b), this->c)->imp_not_form();
}

template<typename Tag>
ptwff<Tag> TOr3<Tag>::half_imp_not_form() const
{
    return TOr<Tag>::create(TOr<Tag>::create(this->a, this->b), this->c);
}

template<typename Tag>
void TOr3<Tag>::get_variables(ptvar_set<Tag> &vars) const
{
    this->a->get_variables(vars);
    this->b->get_variables(vars);
    this->c->get_variables(vars);
}

template<typename Tag>
Prover<CheckpointedProofEngine> TOr3<Tag>::get_type_prover(const LibraryToolbox &tb) const
{
    return tb.build_registered_prover< CheckpointedProofEngine >(TOr3::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
}

template<typename Tag>
Prover<CheckpointedProofEngine> TOr3<Tag>::get_imp_not_prover(const LibraryToolbox &tb) const
{
    auto first = tb.build_registered_prover< CheckpointedProofEngine >(TOr3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
    auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
    auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TOr3::imp_not_2_rp,
    {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
    return compose;
}

template<typename Tag>
bool TOr3<Tag>::operator==(const TWff<Tag> &x) const {
    auto px = dynamic_cast< const TOr3* >(&x);
    if (px == nullptr) {
        return false;
    } else {
        return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b() && *this->get_c() == *px->get_c();
    }
}

template<typename Tag>
void TOr3<Tag>::get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const
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

template<typename Tag>
const RegisteredProver TOr3<Tag>::type_rp = LibraryToolbox::register_prover({}, "wff ( ph \\/ ps \\/ ch )");
template<typename Tag>
const RegisteredProver TOr3<Tag>::imp_not_1_rp = LibraryToolbox::register_prover({}, "|- ( ( ph \\/ ps \\/ ch ) <-> ( ( ph \\/ ps ) \\/ ch ) )");
template<typename Tag>
const RegisteredProver TOr3<Tag>::imp_not_2_rp = LibraryToolbox::register_prover({"|- ( ph <-> ps )", "|- ( ps <-> ch )"}, "|- ( ph <-> ch )");
template<typename Tag>
const RegisteredProver TOr3<Tag>::tseitin1_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ph \\/ ps ) \\/ -. ( ph \\/ ps ) ) )");
template<typename Tag>
const RegisteredProver TOr3<Tag>::tseitin2_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ph \\/ ( ph \\/ ps ) ) )");
template<typename Tag>
const RegisteredProver TOr3<Tag>::tseitin3_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ps \\/ ( ph \\/ ps ) ) )");
template<typename Tag>
const RegisteredProver TOr3<Tag>::tseitin4_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( ( ( ph \\/ ps ) \\/ ch ) \\/ -. ( ph \\/ ps \\/ ch ) ) )");
template<typename Tag>
const RegisteredProver TOr3<Tag>::tseitin5_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ( ph \\/ ps ) \\/ ( ph \\/ ps \\/ ch ) ) )");
template<typename Tag>
const RegisteredProver TOr3<Tag>::tseitin6_rp = LibraryToolbox::register_prover({}, "|- ( th -> ( -. ch \\/ ( ph \\/ ps \\/ ch ) ) )");

template class TWff<PropTag>;
template class TWff<PredTag>;
template class TVar<PropTag>;
template class TVar<PredTag>;
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
        return TVar<Tag>::create(pt_to_pt2(pt), tb);
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
template ptvar_set<PredTag> collect_tseitin_vars<PredTag>(const CNForm<PredTag> &cnf);

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
template ptvar_map< uint32_t, PredTag > build_tseitin_map<PredTag>(const ptvar_set<PredTag> &vars);

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
template std::pair<CNFProblem, std::vector<Prover<CheckpointedProofEngine> > > build_cnf_problem<PredTag>(const CNForm<PredTag> &cnf, const ptvar_map< uint32_t, PredTag > &var_map);
