#pragma once

#include <string>
#include <memory>
#include <set>
#include <vector>

#include "mm/library.h"
#include "mm/proof.h"
#include "mm/toolbox.h"
#include "parsing/parser.h"
#include "provers/sat.h"
#include "mm/ptengine.h"

struct PropTag {};
struct PredTag {};

template<typename Tag>
class TWff;
template<typename Tag>
using ptwff = std::shared_ptr< const TWff<Tag> >;

template<typename Tag>
class TVar;
template<typename Tag>
using ptvar = std::shared_ptr< const TVar<Tag> >;
template<typename Tag>
struct ptvar_comp {
    bool operator()(const ptvar<Tag> &x, const ptvar<Tag> &y) const { return *x < *y; }
};
template<typename Tag>
using ptvar_set = std::set< ptvar<Tag>, ptvar_comp<Tag> >;
template< typename T, typename Tag >
using ptvar_map = std::map< ptvar<Tag>, T, ptvar_comp<Tag> >;

template< typename T, typename Tag >
struct ptvar_pair_comp {
    bool operator()(const std::pair< T, ptvar<Tag> > &x, const std::pair< T, ptvar<Tag> > &y) const {
        if (x.first < y.first) {
            return true;
        }
        if (y.first < x.first) {
            return false;
        }
        return ptvar_comp<Tag>()(x.second, y.second);
    }
};
template<typename Tag>
using CNForm = std::map< std::vector< std::pair< bool, ptvar<Tag> > >, Prover< CheckpointedProofEngine > >;

template<typename Tag>
class TNot;

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

template<typename Tag>
class TWffBase {
public:
    virtual ~TWffBase() {}
    virtual std::string to_string() const = 0;
    virtual ptwff<Tag> imp_not_form() const = 0;
    virtual ptwff<Tag> subst(ptvar<Tag> var, bool positive) const = 0;
    virtual void get_variables(ptvar_set<Tag> &vars) const = 0;
    virtual bool operator==(const TWff<Tag> &x) const = 0;
    virtual std::vector< ptwff<Tag> > get_children() const = 0;

    ParsingTree2<SymTok, LabTok> to_parsing_tree(const LibraryToolbox &tb) const
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
    virtual Prover< CheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const
    {
        (void) tb;
        return null_prover;
    }
    virtual bool is_true() const
    {
        return false;
    }
    virtual Prover< CheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const
    {
        (void) tb;
        return null_prover;
    }
    virtual bool is_false() const
    {
        return false;
    }
    virtual Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const
    {
        (void) tb;
        return null_prover;
    }
    virtual Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const
    {
        (void) tb;
        return null_prover;
    }
    virtual Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const
    {
        (void) var;
        (void) positive;
        (void) tb;
        return null_prover;
    }
};

//extern template class TWffBase<PropTag>;
//extern template class TWffBase<PredTag>;

template<typename Tag>
class TWff : public TWffBase<Tag> {
public:
    virtual void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff &glob_ctx) const = 0;

    ptvar<Tag> get_tseitin_var(const LibraryToolbox &tb) const {
        return TVar<Tag>::create(this->to_parsing_tree(tb), tb);
    }
    std::tuple<CNFProblem, ptvar_map<uint32_t, Tag>, std::vector<Prover<CheckpointedProofEngine> > > get_tseitin_cnf_problem(const LibraryToolbox &tb) const {
        CNForm<Tag> cnf;
        this->get_tseitin_form(cnf, tb, *this);
        cnf[{{true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TWff::id_rp, {{"ph", this->get_type_prover(tb)}}, {});
        auto vars = collect_tseitin_vars(cnf);
        auto map = build_tseitin_map(vars);
        auto problem = build_cnf_problem(cnf, map);
        return make_tuple(problem.first, map, problem.second);
    }
    virtual void set_library_toolbox(const LibraryToolbox &tb) const
    {
        for (auto child : this->get_children()) {
            child->set_library_toolbox(tb);
        }
    }

    std::pair< bool, Prover< CheckpointedProofEngine > > get_adv_truth_prover(const LibraryToolbox &tb) const
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

private:
    std::pair< bool, Prover< CheckpointedProofEngine > > adv_truth_internal(typename ptvar_set<Tag>::iterator cur_var, typename ptvar_set<Tag>::iterator end_var, const LibraryToolbox &tb) const {
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

    static const RegisteredProver adv_truth_1_rp;
    static const RegisteredProver adv_truth_2_rp;
    static const RegisteredProver adv_truth_3_rp;
    static const RegisteredProver adv_truth_4_rp;
    static const RegisteredProver id_rp;
};

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

/**
 * @brief A generic TWff that uses the imp_not form to provide truth and falsity.
 */
template<typename Tag>
class ConvertibleTWff : public TWff<Tag> {
public:
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override
    {
        return this->imp_not_form()->subst(var, positive);
    }
    Prover< CheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const override
    {
        //return null_prover;
        return tb.build_registered_prover< CheckpointedProofEngine >(ConvertibleTWff::truth_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_truth_prover(tb)});
    }
    bool is_true() const override
    {
        return this->imp_not_form()->is_true();
    }
    Prover< CheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const override
    {
        //return null_prover;
        return tb.build_registered_prover< CheckpointedProofEngine >(ConvertibleTWff::falsity_rp, {{"ph", this->imp_not_form()->get_type_prover(tb)}, {"ps", this->get_type_prover(tb)}}, {this->get_imp_not_prover(tb), this->imp_not_form()->get_falsity_prover(tb)});
    }
    bool is_false() const override
    {
        return this->imp_not_form()->is_false();
    }
    //Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;

private:
    static const RegisteredProver truth_rp;
    static const RegisteredProver falsity_rp;
};

template<typename Tag>
const RegisteredProver ConvertibleTWff<Tag>::truth_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- ph"}, "|- ps");
template<typename Tag>
const RegisteredProver ConvertibleTWff<Tag>::falsity_rp = LibraryToolbox::register_prover({"|- ( ps <-> ph )", "|- -. ph"}, "|- -. ps");

template<typename Tag>
ptwff<Tag> wff_from_pt(const ParsingTree< SymTok, LabTok > &pt, const LibraryToolbox &tb);

template<typename Tag>
class TTrue : public TWff<Tag>, public enable_create< TTrue<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override {
        return "T.";
    }
    ptwff<Tag> imp_not_form() const override {
        return TTrue::create();
    }
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override
    {
        (void) var;
        (void) positive;
        return this->shared_from_this();
    }
    void get_variables(ptvar_set<Tag> &vars) const override
    {
        (void) vars;
    }
    Prover< CheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TTrue::truth_rp, {}, {});
    }
    bool is_true() const override
    {
        return true;
    }
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TTrue::type_rp, {}, {});
    }
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TTrue::imp_not_rp, {}, {});
    }
    Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const override
    {
        ptwff<Tag> subst = var;
        if (!positive) {
            subst = TNot<Tag>::create(subst);
        }
        return tb.build_registered_prover< CheckpointedProofEngine >(TTrue::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
    }
    bool operator==(const TWff<Tag> &x) const override {
        auto px = dynamic_cast< const TTrue* >(&x);
        if (px == nullptr) {
            return false;
        } else {
            return true;
        }
    }
    void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
    {
        cnf[{{true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TTrue::tseitin1_rp, {{"th", glob_ctx.get_type_prover(tb)}}, {});
    }
    std::vector< ptwff<Tag> > get_children() const override
    {
        return {};
    }

protected:
    TTrue() {
    }

private:
    static const RegisteredProver truth_rp;
    static const RegisteredProver type_rp;
    static const RegisteredProver imp_not_rp;
    static const RegisteredProver subst_rp;
    static const RegisteredProver tseitin1_rp;
};

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
class TFalse : public TWff<Tag>, public enable_create< TFalse<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override {
        return "F.";
    }
    ptwff<Tag> imp_not_form() const override {
        return TFalse::create();
    }
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override
    {
        (void) var;
        (void) positive;
        return this->shared_from_this();
    }
    void get_variables(ptvar_set<Tag> &vars) const override
    {
        (void) vars;
    }
    Prover< CheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TFalse::falsity_rp, {}, {});
    }
    bool is_false() const override
    {
        return true;
    }
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TFalse::type_rp, {}, {});
    }
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TFalse::imp_not_rp, {}, {});
    }
    Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const override
    {
        ptwff<Tag> subst = var;
        if (!positive) {
            subst = TNot<Tag>::create(subst);
        }
        return tb.build_registered_prover< CheckpointedProofEngine >(TFalse::subst_rp, {{"ph", subst->get_type_prover(tb)}}, {});
    }
    bool operator==(const TWff<Tag> &x) const override {
        auto px = dynamic_cast< const TFalse* >(&x);
        if (px == nullptr) {
            return false;
        } else {
            return true;
        }
    }
    void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
    {
        cnf[{{false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TFalse::tseitin1_rp, {{"th", glob_ctx.get_type_prover(tb)}}, {});
    }
    std::vector< ptwff<Tag> > get_children() const override
    {
        return {};
    }

protected:
    TFalse() {
    }

private:
    static const RegisteredProver falsity_rp;
    static const RegisteredProver type_rp;
    static const RegisteredProver imp_not_rp;
    static const RegisteredProver subst_rp;
    static const RegisteredProver tseitin1_rp;
};

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
class TVar : public TWff<Tag>, public enable_create< TVar<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
  typedef ParsingTree2< SymTok, LabTok > NameType;

    std::string to_string() const override {
        return this->string_repr;
    }
    ptwff<Tag> imp_not_form() const override {
        return this->shared_from_this();
    }
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override
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
    void get_variables(ptvar_set<Tag> &vars) const override
    {
        vars.insert(this->shared_from_this());
    }
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_type_prover(this->get_name());
        /*auto label = tb.get_var_sym_to_lab(tb.get_symbol(this->name));
    return [label](AbstractCheckpointedProofEngine &engine) {
        engine.process_label(label);
        return true;
    };*/
    }
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TVar::imp_not_rp, {{"ph", this->get_type_prover(tb)}}, {});
    }
    Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const override
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
    bool operator==(const TWff<Tag> &x) const override {
        auto px = dynamic_cast< const TVar* >(&x);
        if (px == nullptr) {
            return false;
        } else {
            return this->get_name() == px->get_name();
        }
    }
    bool operator<(const TVar &x) const {
        return this->get_name() < x.get_name();
    }
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
  {
      (void) cnf;
      (void) tb;
      (void) glob_ctx;
  }
  std::vector< ptwff<Tag> > get_children() const override
  {
      return {};
  }
  void set_library_toolbox(const LibraryToolbox &tb) const override
  {
      if (!this->name.quick_is_valid()) {
          this->name = var_cons_helper(string_repr, tb);
      }
  }
  const NameType &get_name() const {
      assert(this->name.quick_is_valid());
      return this->name;
  }

protected:
  TVar(const std::string &string_repr) : string_repr(string_repr)
  {
  }
  TVar(NameType name, std::string string_repr) :
      name(name), string_repr(string_repr) {
  }
  TVar(const std::string &string_repr, const LibraryToolbox &tb) :
      name(TVar::var_cons_helper(string_repr, tb)), string_repr(string_repr) {
  }
  TVar(const NameType &name, const LibraryToolbox &tb) :
      name(name), string_repr(tb.print_sentence(tb.reconstruct_sentence(pt2_to_pt(name))).to_string()) {
  }

private:
  static auto var_cons_helper(const std::string &string_repr, const LibraryToolbox &tb) {
      auto sent = tb.read_sentence(string_repr);
      auto pt = tb.parse_sentence(sent.begin(), sent.end(), tb.get_turnstile_alias());
      return pt_to_pt2(pt);
  }

  mutable NameType name;
  std::string string_repr;

  static const RegisteredProver imp_not_rp;
  static const RegisteredProver subst_pos_1_rp;
  static const RegisteredProver subst_pos_2_rp;
  static const RegisteredProver subst_pos_3_rp;
  static const RegisteredProver subst_pos_truth_rp;
  static const RegisteredProver subst_neg_1_rp;
  static const RegisteredProver subst_neg_2_rp;
  static const RegisteredProver subst_neg_3_rp;
  static const RegisteredProver subst_neg_falsity_rp;
  static const RegisteredProver subst_indep_rp;
};

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
class TNot : public TWff<Tag>, public enable_create< TNot<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override {
        return "-. " + this->a->to_string();
    }
    ptwff<Tag> imp_not_form() const override {
        return TNot::create(this->a->imp_not_form());
    }
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override
    {
        return TNot::create(this->a->subst(var, positive));
    }
    void get_variables(ptvar_set<Tag> &vars) const override
    {
        this->a->get_variables(vars);
    }
    Prover< CheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const override
    {
        return this->a->get_falsity_prover(tb);
    }
    bool is_true() const override
    {
        return this->a->is_false();
    }
    Prover< CheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TNot::falsity_rp, {{ "ph", this->a->get_type_prover(tb) }}, { this->a->get_truth_prover(tb) });
    }
    bool is_false() const override
    {
        return this->a->is_true();
    }
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TNot::type_rp, {{ "ph", this->a->get_type_prover(tb) }}, {});
    }
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TNot::imp_not_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }}, { this->a->get_imp_not_prover(tb) });
    }
    Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const override
    {
        ptwff<Tag> ant;
        if (positive) {
            ant = var;
        } else {
            ant = TNot::create(var);
        }
        return tb.build_registered_prover< CheckpointedProofEngine >(TNot::subst_rp, {{"ph", ant->get_type_prover(tb)}, {"ps", this->a->get_type_prover(tb)}, {"ch", this->a->subst(var, positive)->get_type_prover(tb)}}, {this->a->get_subst_prover(var, positive, tb)});
    }
    bool operator==(const TWff<Tag> &x) const override {
        auto px = dynamic_cast< const TNot* >(&x);
        if (px == nullptr) {
            return false;
        } else {
            return *this->get_a() == *px->get_a();
        }
    }
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
  {
      this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
      cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNot::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNot::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
  }
  std::vector< ptwff<Tag> > get_children() const override
  {
      return { this->get_a() };
  }
  ptwff<Tag> get_a() const {
      return this->a;
  }

protected:
  TNot(ptwff<Tag> a) :
      a(a) {
  }

private:
  ptwff<Tag> a;

  static const RegisteredProver falsity_rp;
  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_rp;
  static const RegisteredProver subst_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
};

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
class TImp : public TWff<Tag>, public enable_create< TImp<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override {
        return "( " + this->a->to_string() + " -> " + this->b->to_string() + " )";
    }
    ptwff<Tag> imp_not_form() const override {
        return TImp::create(this->a->imp_not_form(), this->b->imp_not_form());
    }
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override
    {
        return TImp::create(this->a->subst(var, positive), this->b->subst(var, positive));
    }
    void get_variables(ptvar_set<Tag> &vars) const override
    {
        this->a->get_variables(vars);
        this->b->get_variables(vars);
    }
    Prover< CheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const override
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
    bool is_true() const override
    {
        return this->b->is_true() || this->a->is_false();
    }
    Prover< CheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const override
    {
        auto theorem_prover = tb.build_registered_prover< CheckpointedProofEngine >(TImp::falsity_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
        auto mp_prover1 = tb.build_registered_prover< CheckpointedProofEngine >(TImp::falsity_2_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { this->a->get_truth_prover(tb), theorem_prover });
        auto mp_prover2 = tb.build_registered_prover< CheckpointedProofEngine >(TImp::falsity_3_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, { mp_prover1, this->b->get_falsity_prover(tb) });
        return mp_prover2;
    }
    bool is_false() const override
    {
        return this->a->is_true() && this->b->is_false();
    }
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TImp::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
    }
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TImp::imp_not_rp,
        {{"ph", this->a->get_type_prover(tb)}, {"ps", this->a->imp_not_form()->get_type_prover(tb) }, {"ch", this->b->get_type_prover(tb)}, {"th", this->b->imp_not_form()->get_type_prover(tb)}},
        { this->a->get_imp_not_prover(tb), this->b->get_imp_not_prover(tb) });
    }
    Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const override
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
    bool operator==(const TWff<Tag> &x) const override {
        auto px = dynamic_cast< const TImp* >(&x);
        if (px == nullptr) {
            return false;
        } else {
            return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
        }
    }
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
  {
      this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
      this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
      cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TImp::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TImp::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TImp::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
  }
  std::vector< ptwff<Tag> > get_children() const override
  {
      return { this->get_a(), this->get_b() };
  }
  ptwff<Tag> get_a() const {
      return this->a;
  }
  ptwff<Tag> get_b() const {
      return this->b;
  }

protected:
  TImp(ptwff<Tag> a, ptwff<Tag> b) :
      a(a), b(b) {
  }

private:
  ptwff<Tag> a, b;

  static const RegisteredProver truth_1_rp;
  static const RegisteredProver truth_2_rp;
  static const RegisteredProver falsity_1_rp;
  static const RegisteredProver falsity_2_rp;
  static const RegisteredProver falsity_3_rp;
  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_rp;
  static const RegisteredProver subst_rp;
  static const RegisteredProver mp_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
};

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
class TBiimp : public ConvertibleTWff<Tag>, public enable_create< TBiimp<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override {
        return "( " + this->a->to_string() + " <-> " + this->b->to_string() + " )";
    }
    ptwff<Tag> imp_not_form() const override {
        auto ain = this->a->imp_not_form();
        auto bin = this->b->imp_not_form();
        return TNot<Tag>::create(TImp<Tag>::create(TImp<Tag>::create(ain, bin), TNot<Tag>::create(TImp<Tag>::create(bin, ain))));
    }
    ptwff<Tag> half_imp_not_form() const
    {
        auto ain = this->a;
        auto bin = this->b;
        return TNot<Tag>::create(TImp<Tag>::create(TImp<Tag>::create(ain, bin), TNot<Tag>::create(TImp<Tag>::create(bin, ain))));
    }
    void get_variables(ptvar_set<Tag> &vars) const override
    {
        this->a->get_variables(vars);
        this->b->get_variables(vars);
    }
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TBiimp::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
    }
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
    {
        auto first = tb.build_registered_prover< CheckpointedProofEngine >(TBiimp::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
        auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
        auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TBiimp::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
        return compose;
    }
    bool operator==(const TWff<Tag> &x) const override {
        auto px = dynamic_cast< const TBiimp* >(&x);
        if (px == nullptr) {
            return false;
        } else {
            return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
        }
    }
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
  {
      this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
      this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
      cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TBiimp::tseitin4_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
  }
  std::vector< ptwff<Tag> > get_children() const override
  {
      return { this->get_a(), this->get_b() };
  }
  ptwff<Tag> get_a() const {
      return this->a;
  }
  ptwff<Tag> get_b() const {
      return this->b;
  }

protected:
  TBiimp(ptwff<Tag> a, ptwff<Tag> b) :
      a(a), b(b) {
  }

private:
  ptwff<Tag> a, b;

  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_1_rp;
  static const RegisteredProver imp_not_2_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
  static const RegisteredProver tseitin4_rp;
};

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
class TAnd : public ConvertibleTWff<Tag>, public enable_create< TAnd<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override {
        return "( " + this->a->to_string() + " /\\ " + this->b->to_string() + " )";
    }
    ptwff<Tag> imp_not_form() const override
    {
        return TNot<Tag>::create(TImp<Tag>::create(this->a->imp_not_form(), TNot<Tag>::create(this->b->imp_not_form())));
    }
  ptwff<Tag> half_imp_not_form() const {
      return TNot<Tag>::create(TImp<Tag>::create(this->a, TNot<Tag>::create(this->b)));
  }
  void get_variables(ptvar_set<Tag> &vars) const override
  {
      this->a->get_variables(vars);
      this->b->get_variables(vars);
  }
  Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
  {
      return tb.build_registered_prover< CheckpointedProofEngine >(TAnd::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
  }
  Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
  {
      auto first = tb.build_registered_prover< CheckpointedProofEngine >(TAnd::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
      auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
      auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TAnd::imp_not_2_rp,
      {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
      return compose;
  }
  bool operator==(const TWff<Tag> &x) const override {
      auto px = dynamic_cast< const TAnd* >(&x);
      if (px == nullptr) {
          return false;
      } else {
          return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
      }
  }
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
  {
      this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
      this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
      cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TAnd::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
  }
  std::vector< ptwff<Tag> > get_children() const override
  {
      return { this->get_a(), this->get_b() };
  }
  ptwff<Tag> get_a() const {
      return this->a;
  }
  ptwff<Tag> get_b() const {
      return this->b;
  }

protected:
  TAnd(ptwff<Tag> a, ptwff<Tag> b) :
      a(a), b(b) {
  }

private:
  ptwff<Tag> a, b;

  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_1_rp;
  static const RegisteredProver imp_not_2_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
};

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
class TOr : public ConvertibleTWff<Tag>, public enable_create< TOr<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override {
        return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " )";
    }
    ptwff<Tag> imp_not_form() const override {
        return TImp<Tag>::create(TNot<Tag>::create(this->a->imp_not_form()), this->b->imp_not_form());
    }
    ptwff<Tag> half_imp_not_form() const
    {
        return TImp<Tag>::create(TNot<Tag>::create(this->a), this->b);
    }
    void get_variables(ptvar_set<Tag> &vars) const override
    {
        this->a->get_variables(vars);
        this->b->get_variables(vars);
    }
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TOr::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
    }
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
    {
        auto first = tb.build_registered_prover< CheckpointedProofEngine >(TOr::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
        auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
        auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TOr::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
        return compose;
    }
    bool operator==(const TWff<Tag> &x) const override {
        auto px = dynamic_cast< const TOr* >(&x);
        if (px == nullptr) {
            return false;
        } else {
            return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
        }
    }
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
  {
      this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
      this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
      cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TOr::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
  }
  std::vector< ptwff<Tag> > get_children() const override
  {
      return { this->get_a(), this->get_b() };
  }
  ptwff<Tag> get_a() const {
      return this->a;
  }
  ptwff<Tag> get_b() const {
      return this->b;
  }

protected:
  TOr(ptwff<Tag> a, ptwff<Tag> b) :
      a(a), b(b) {
  }

private:
  ptwff<Tag> a, b;

  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_1_rp;
  static const RegisteredProver imp_not_2_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
};

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
class TNand : public ConvertibleTWff<Tag>, public enable_create< TNand<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override {
        return "( " + this->a->to_string() + " -/\\ " + this->b->to_string() + " )";
    }
    ptwff<Tag> imp_not_form() const override {
        return TNot<Tag>::create(TAnd<Tag>::create(this->a->imp_not_form(), this->b->imp_not_form()))->imp_not_form();
    }
    ptwff<Tag> half_imp_not_form() const
    {
        return TNot<Tag>::create(TAnd<Tag>::create(this->a, this->b));
    }
    void get_variables(ptvar_set<Tag> &vars) const override
    {
        this->a->get_variables(vars);
        this->b->get_variables(vars);
    }
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TNand::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
    }
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
    {
        auto first = tb.build_registered_prover< CheckpointedProofEngine >(TNand::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
        auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
        auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TNand::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
        return compose;
    }
    bool operator==(const TWff<Tag> &x) const override {
        auto px = dynamic_cast< const TNand* >(&x);
        if (px == nullptr) {
            return false;
        } else {
            return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
        }
    }
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
  {
      this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
      this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
      cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNand::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNand::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TNand::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
  }
  std::vector< ptwff<Tag> > get_children() const override
  {
      return { this->get_a(), this->get_b() };
  }
  ptwff<Tag> get_a() const {
      return this->a;
  }
  ptwff<Tag> get_b() const {
      return this->b;
  }

protected:
  TNand(ptwff<Tag> a, ptwff<Tag> b) :
      a(a), b(b) {
  }

private:
  ptwff<Tag> a, b;

  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_1_rp;
  static const RegisteredProver imp_not_2_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
};

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
class TXor : public ConvertibleTWff<Tag>, public enable_create< TXor<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override {
        return "( " + this->a->to_string() + " \\/_ " + this->b->to_string() + " )";
    }
    ptwff<Tag> imp_not_form() const override {
        return TNot<Tag>::create(TBiimp<Tag>::create(this->a->imp_not_form(), this->b->imp_not_form()))->imp_not_form();
    }
    ptwff<Tag> half_imp_not_form() const
    {
        return TNot<Tag>::create(TBiimp<Tag>::create(this->a, this->b));
    }
    void get_variables(ptvar_set<Tag> &vars) const override
    {
        this->a->get_variables(vars);
        this->b->get_variables(vars);
    }
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TXor::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }}, {});
    }
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
    {
        auto first = tb.build_registered_prover< CheckpointedProofEngine >(TXor::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}}, {});
        auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
        auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TXor::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
        return compose;
    }
    bool operator==(const TWff<Tag> &x) const override {
        auto px = dynamic_cast< const TXor* >(&x);
        if (px == nullptr) {
            return false;
        } else {
            return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b();
        }
    }
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
  {
      this->get_a()->get_tseitin_form(cnf, tb, glob_ctx);
      this->get_b()->get_tseitin_form(cnf, tb, glob_ctx);
      cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin1_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {false, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin2_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{true, this->get_a()->get_tseitin_var(tb)}, {false, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin3_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
      cnf[{{false, this->get_a()->get_tseitin_var(tb)}, {true, this->get_b()->get_tseitin_var(tb)}, {true, this->get_tseitin_var(tb)}}] = tb.build_registered_prover(TXor::tseitin4_rp, {{"ph", this->get_a()->get_type_prover(tb)}, {"ps", this->get_b()->get_type_prover(tb)}, {"th", glob_ctx.get_type_prover(tb)}}, {});
  }
  std::vector< ptwff<Tag> > get_children() const override
  {
      return { this->get_a(), this->get_b() };
  }
  ptwff<Tag> get_a() const {
      return this->a;
  }
  ptwff<Tag> get_b() const {
      return this->b;
  }

protected:
  TXor(ptwff<Tag> a, ptwff<Tag> b) :
      a(a), b(b) {
  }

private:
  ptwff<Tag> a, b;

  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_1_rp;
  static const RegisteredProver imp_not_2_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
  static const RegisteredProver tseitin4_rp;
};

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
class TAnd3 : public ConvertibleTWff<Tag>, public enable_create< TAnd3<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override
    {
        return "( " + this->a->to_string() + " /\\ " + this->b->to_string() + " /\\ " + this->c->to_string() + " )";
    }
    ptwff<Tag> imp_not_form() const override
    {
        return TAnd<Tag>::create(TAnd<Tag>::create(this->a, this->b), this->c)->imp_not_form();
    }
    ptwff<Tag> half_imp_not_form() const
    {
        return TAnd<Tag>::create(TAnd<Tag>::create(this->a, this->b), this->c);
    }
    void get_variables(ptvar_set<Tag> &vars) const override
    {
        this->a->get_variables(vars);
        this->b->get_variables(vars);
        this->c->get_variables(vars);
    }
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TAnd3::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
    }
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
    {
        auto first = tb.build_registered_prover< CheckpointedProofEngine >(TAnd3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
        auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
        auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TAnd3::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
        return compose;
    }
    bool operator==(const TWff<Tag> &x) const override {
        auto px = dynamic_cast< const TAnd3* >(&x);
        if (px == nullptr) {
            return false;
        } else {
            return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b() && *this->get_c() == *px->get_c();
        }
    }
    void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
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
    std::vector< ptwff<Tag> > get_children() const override
    {
        return { this->get_a(), this->get_b(), this->get_c() };
    }
    ptwff<Tag> get_a() const {
        return this->a;
    }
    ptwff<Tag> get_b() const {
        return this->b;
    }
    ptwff<Tag> get_c() const {
        return this->c;
    }

protected:
    TAnd3(ptwff<Tag> a, ptwff<Tag> b, ptwff<Tag> c) :
        a(a), b(b), c(c)
    {
    }

  private:
    ptwff<Tag> a, b, c;

    static const RegisteredProver type_rp;
    static const RegisteredProver imp_not_1_rp;
    static const RegisteredProver imp_not_2_rp;
    static const RegisteredProver tseitin1_rp;
    static const RegisteredProver tseitin2_rp;
    static const RegisteredProver tseitin3_rp;
    static const RegisteredProver tseitin4_rp;
    static const RegisteredProver tseitin5_rp;
    static const RegisteredProver tseitin6_rp;
};

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
class TOr3 : public ConvertibleTWff<Tag>, public enable_create< TOr3<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override
    {
        return "( " + this->a->to_string() + " \\/ " + this->b->to_string() + " \\/ " + this->c->to_string() + " )";
    }
    ptwff<Tag> imp_not_form() const override
    {
        return TOr<Tag>::create(TOr<Tag>::create(this->a, this->b), this->c)->imp_not_form();
    }
    ptwff<Tag> half_imp_not_form() const
    {
        return TOr<Tag>::create(TOr<Tag>::create(this->a, this->b), this->c);
    }
    void get_variables(ptvar_set<Tag> &vars) const override
    {
        this->a->get_variables(vars);
        this->b->get_variables(vars);
        this->c->get_variables(vars);
    }
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override
    {
        return tb.build_registered_prover< CheckpointedProofEngine >(TOr3::type_rp, {{ "ph", this->a->get_type_prover(tb) }, { "ps", this->b->get_type_prover(tb) }, { "ch", this->c->get_type_prover(tb) }}, {});
    }
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override
    {
        auto first = tb.build_registered_prover< CheckpointedProofEngine >(TOr3::imp_not_1_rp, {{"ph", this->a->get_type_prover(tb)}, {"ps", this->b->get_type_prover(tb)}, {"ch", this->c->get_type_prover(tb)}}, {});
        auto second = this->half_imp_not_form()->get_imp_not_prover(tb);
        auto compose = tb.build_registered_prover< CheckpointedProofEngine >(TOr3::imp_not_2_rp,
        {{"ph", this->get_type_prover(tb)}, {"ps", this->half_imp_not_form()->get_type_prover(tb)}, {"ch", this->imp_not_form()->get_type_prover(tb)}}, {first, second});
        return compose;
    }
    void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override
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
    std::vector< ptwff<Tag> > get_children() const override
    {
        return { this->get_a(), this->get_b(), this->get_c() };
    }
    bool operator==(const TWff<Tag> &x) const override {
        auto px = dynamic_cast< const TOr3* >(&x);
        if (px == nullptr) {
            return false;
        } else {
            return *this->get_a() == *px->get_a() && *this->get_b() == *px->get_b() && *this->get_c() == *px->get_c();
        }
    }
    ptwff<Tag> get_a() const {
        return this->a;
    }
    ptwff<Tag> get_b() const {
        return this->b;
    }
    ptwff<Tag> get_c() const {
        return this->c;
    }

protected:
    TOr3(ptwff<Tag> a, ptwff<Tag> b, ptwff<Tag> c) :
        a(a), b(b), c(c)
    {
    }

  private:
    ptwff<Tag> a, b, c;

    static const RegisteredProver type_rp;
    static const RegisteredProver imp_not_1_rp;
    static const RegisteredProver imp_not_2_rp;
    static const RegisteredProver tseitin1_rp;
    static const RegisteredProver tseitin2_rp;
    static const RegisteredProver tseitin3_rp;
    static const RegisteredProver tseitin4_rp;
    static const RegisteredProver tseitin5_rp;
    static const RegisteredProver tseitin6_rp;
};

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

extern template class TWff<PropTag>;
extern template class TWff<PredTag>;
extern template class TVar<PropTag>;
extern template class TVar<PredTag>;
extern template class TTrue<PropTag>;
extern template class TTrue<PredTag>;
extern template class TFalse<PropTag>;
extern template class TFalse<PredTag>;
extern template class TNot<PropTag>;
extern template class TNot<PredTag>;
extern template class TImp<PropTag>;
extern template class TImp<PredTag>;
extern template class TBiimp<PropTag>;
extern template class TBiimp<PredTag>;
extern template class TAnd<PropTag>;
extern template class TAnd<PredTag>;
extern template class TOr<PropTag>;
extern template class TOr<PredTag>;
extern template class TNand<PropTag>;
extern template class TNand<PredTag>;
extern template class TXor<PropTag>;
extern template class TXor<PredTag>;
extern template class TAnd3<PropTag>;
extern template class TAnd3<PredTag>;
extern template class TOr3<PropTag>;
extern template class TOr3<PredTag>;

using Wff = TWff<PropTag>;
using pwff = ptwff<PropTag>;
using Var = TVar<PropTag>;
using True = TTrue<PropTag>;
using False = TFalse<PropTag>;
using Not = TNot<PropTag>;
using Imp = TImp<PropTag>;
using Biimp = TBiimp<PropTag>;
using And = TAnd<PropTag>;
using Or = TOr<PropTag>;
using Nand = TNand<PropTag>;
using Xor = TXor<PropTag>;
using And3 = TAnd3<PropTag>;
using Or3 = TOr3<PropTag>;

using pvar_set = ptvar_set<PropTag>;
