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
    bool operator()(const ptvar<Tag> &x, const ptvar<Tag> &y) const;
};
template<typename Tag>
using ptvar_set = std::set< ptvar<Tag>, ptvar_comp<Tag> >;
template< typename T, typename Tag >
using ptvar_map = std::map< ptvar<Tag>, T, ptvar_comp<Tag> >;

template< typename T, typename Tag >
struct ptvar_pair_comp {
    bool operator()(const std::pair< T, ptvar<Tag> > &x, const std::pair< T, ptvar<Tag> > &y) const;
};
template<typename Tag>
using CNForm = std::map< std::vector< std::pair< bool, ptvar<Tag> > >, Prover< CheckpointedProofEngine > >;

template<typename Tag>
class TNot;

template<typename Tag>
ptwff<Tag> wff_from_pt(const ParsingTree< SymTok, LabTok > &pt, const LibraryToolbox &tb);

template<typename Tag>
ptvar_set<Tag> collect_tseitin_vars(const CNForm<Tag> &cnf);
template<typename Tag>
ptvar_map< uint32_t, Tag > build_tseitin_map(const ptvar_set<Tag> &vars);
template<typename Tag>
std::pair<CNFProblem, std::vector<Prover<CheckpointedProofEngine> > > build_cnf_problem(const CNForm<Tag> &cnf, const ptvar_map< uint32_t, Tag > &var_map);

template<typename Tag_>
class TWffBase {
public:
    typedef Tag_ Tag;

    virtual ~TWffBase();
    virtual std::string to_string() const = 0;
    virtual ptwff<Tag> imp_not_form() const = 0;
    virtual ptwff<Tag> subst(ptvar<Tag> var, bool positive) const = 0;
    virtual void get_variables(ptvar_set<Tag> &vars) const = 0;
    virtual bool operator==(const TWff<Tag> &x) const = 0;
    virtual std::vector< ptwff<Tag> > get_children() const = 0;

    ParsingTree2<SymTok, LabTok> to_parsing_tree(const LibraryToolbox &tb) const;
    virtual Prover< CheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const;
    virtual bool is_true() const;
    virtual Prover< CheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const;
    virtual bool is_false() const;
    virtual Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
    virtual Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
    virtual Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const;
};

template<typename Tag>
class TWff : public TWffBase<Tag> {
public:
    virtual void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff &glob_ctx) const = 0;

    ptvar<Tag> get_tseitin_var(const LibraryToolbox &tb) const;
    std::tuple<CNFProblem, ptvar_map<uint32_t, Tag>, std::vector<Prover<CheckpointedProofEngine> > > get_tseitin_cnf_problem(const LibraryToolbox &tb) const;
    virtual void set_library_toolbox(const LibraryToolbox &tb) const;
    std::pair< bool, Prover< CheckpointedProofEngine > > get_adv_truth_prover(const LibraryToolbox &tb) const;

private:
    std::pair< bool, Prover< CheckpointedProofEngine > > adv_truth_internal(typename ptvar_set<Tag>::iterator cur_var, typename ptvar_set<Tag>::iterator end_var, const LibraryToolbox &tb) const;

    static const RegisteredProver adv_truth_1_rp;
    static const RegisteredProver adv_truth_2_rp;
    static const RegisteredProver adv_truth_3_rp;
    static const RegisteredProver adv_truth_4_rp;
    static const RegisteredProver id_rp;
};

/**
 * @brief A generic TWff that uses the imp_not form to provide truth and falsity.
 */
template<typename Tag>
class ConvertibleTWff : public virtual TWff<Tag> {
public:
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override;
    Prover< CheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const override;
    bool is_true() const override;
    Prover< CheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const override;
    bool is_false() const override;

private:
    static const RegisteredProver truth_rp;
    static const RegisteredProver falsity_rp;
};

template<typename Tag>
class TWff0 : public virtual TWff<Tag> {
public:
    std::vector< ptwff<Tag> > get_children() const override { return {}; }
};

template<typename Tag>
class TWff1 : public virtual TWff<Tag> {
public:
    std::vector< ptwff<Tag> > get_children() const override { return { this->get_a() }; }
    ptwff<Tag> get_a() const { return this->a; }

protected:
    TWff1() = delete;
    TWff1(ptwff<Tag> a) : a(a) {}

protected:
    ptwff<Tag> a;
};

template<typename Tag>
class TWff2 : public virtual TWff<Tag> {
public:
    std::vector< ptwff<Tag> > get_children() const override { return { this->get_a(), this->get_b() }; }
    ptwff<Tag> get_a() const { return this->a; }
    ptwff<Tag> get_b() const { return this->b; }

protected:
    TWff2() = delete;
    TWff2(ptwff<Tag> a, ptwff<Tag> b) : a(a), b(b) {}

protected:
    ptwff<Tag> a;
    ptwff<Tag> b;
};

template<typename Tag>
class TWff3 : public virtual TWff<Tag> {
public:
    std::vector< ptwff<Tag> > get_children() const override { return { this->get_a(), this->get_b(), this->get_c() }; }
    ptwff<Tag> get_a() const { return this->a; }
    ptwff<Tag> get_b() const { return this->b; }
    ptwff<Tag> get_c() const { return this->c; }

protected:
    TWff3() = delete;
    TWff3(ptwff<Tag> a, ptwff<Tag> b, ptwff<Tag> c) : a(a), b(b), c(c) {}

protected:
    ptwff<Tag> a;
    ptwff<Tag> b;
    ptwff<Tag> c;
};

template<typename Tag>
class TTrueBase : public TWff0<Tag>, public virtual_enable_shared_from_this<TTrueBase<Tag>> {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override;
    void get_variables(ptvar_set<Tag> &vars) const override;
    Prover< CheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const override;
    bool is_true() const override;
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const override;
    bool operator==(const TWff<Tag> &x) const override;
    void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;

private:
    static const RegisteredProver truth_rp;
    static const RegisteredProver type_rp;
    static const RegisteredProver imp_not_rp;
    static const RegisteredProver subst_rp;
    static const RegisteredProver tseitin1_rp;
};

template<typename Tag>
class TTrue;

template<>
class TTrue<PropTag> : public TTrueBase<PropTag>, public enable_create<TTrue<PropTag>> {
protected:
    using TTrueBase<PropTag>::TTrueBase;
};

template<>
class TTrue<PredTag> : public TTrueBase<PredTag>, public enable_create<TTrue<PredTag>> {
protected:
    using TTrueBase<PredTag>::TTrueBase;
};

template<typename Tag>
class TFalseBase : public TWff0<Tag>, public virtual_enable_shared_from_this<TFalseBase<Tag>> {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override;
    void get_variables(ptvar_set<Tag> &vars) const override;
    Prover< CheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const override;
    bool is_false() const override;
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const override;
    bool operator==(const TWff<Tag> &x) const override;
    void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;

private:
    static const RegisteredProver falsity_rp;
    static const RegisteredProver type_rp;
    static const RegisteredProver imp_not_rp;
    static const RegisteredProver subst_rp;
    static const RegisteredProver tseitin1_rp;
};

template<typename Tag>
class TFalse;

template<>
class TFalse<PropTag> : public TFalseBase<PropTag>, public enable_create<TFalse<PropTag>> {
protected:
    using TFalseBase<PropTag>::TFalseBase;
};

template<>
class TFalse<PredTag> : public TFalseBase<PredTag>, public enable_create<TFalse<PredTag>> {
protected:
    using TFalseBase<PredTag>::TFalseBase;
};

template<typename Tag>
class TVar : public TWff0<Tag>, public enable_create< TVar<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    typedef ParsingTree2< SymTok, LabTok > NameType;

    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override;
    void get_variables(ptvar_set<Tag> &vars) const override;
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const override;
    bool operator==(const TWff<Tag> &x) const override;
    bool operator<(const TVar &x) const;
    void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;
    void set_library_toolbox(const LibraryToolbox &tb) const override;
    const NameType &get_name() const;

protected:
    TVar() = delete;
    TVar(const std::string &string_repr);
    TVar(NameType name, std::string string_repr);
    TVar(const std::string &string_repr, const LibraryToolbox &tb);
    TVar(const NameType &name, const LibraryToolbox &tb);

private:
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
class TNotBase : public TWff1<Tag>, public virtual_enable_shared_from_this< TNotBase<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override;
    void get_variables(ptvar_set<Tag> &vars) const override;
    Prover< CheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const override;
    bool is_true() const override;
    Prover< CheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const override;
    bool is_false() const override;
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const override;
    bool operator==(const TWff<Tag> &x) const override;
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;

protected:
  using TWff1<Tag>::TWff1;

private:
  static const RegisteredProver falsity_rp;
  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_rp;
  static const RegisteredProver subst_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
};

template<typename Tag>
class TNot;

template<>
class TNot<PropTag> : public TNotBase<PropTag>, public enable_create<TNot<PropTag>> {
protected:
    using TNotBase<PropTag>::TNotBase;
};

template<>
class TNot<PredTag> : public TNotBase<PredTag>, public enable_create<TNot<PredTag>> {
protected:
    using TNotBase<PredTag>::TNotBase;
};

template<typename Tag>
class TImpBase : public TWff2<Tag>, public virtual_enable_shared_from_this< TImpBase<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
    ptwff<Tag> subst(ptvar<Tag> var, bool positive) const override;
    void get_variables(ptvar_set<Tag> &vars) const override;
    Prover< CheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const override;
    bool is_true() const override;
    Prover< CheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const override;
    bool is_false() const override;
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_subst_prover(ptvar<Tag> var, bool positive, const LibraryToolbox &tb) const override;
    bool operator==(const TWff<Tag> &x) const override;
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;

protected:
    using TWff2<Tag>::TWff2;

private:
  static const RegisteredProver truth_1_rp;
  static const RegisteredProver truth_2_rp;
  static const RegisteredProver falsity_1_rp;
  static const RegisteredProver falsity_2_rp;
  static const RegisteredProver falsity_3_rp;
  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_rp;
  static const RegisteredProver subst_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
};

template<typename Tag>
class TImp;

template<>
class TImp<PropTag> : public TImpBase<PropTag>, public enable_create<TImp<PropTag>> {
protected:
    using TImpBase<PropTag>::TImpBase;
};

template<>
class TImp<PredTag> : public TImpBase<PredTag>, public enable_create<TImp<PredTag>> {
protected:
    using TImpBase<PredTag>::TImpBase;
};

template<typename Tag>
class TBiimpBase : public TWff2<Tag>, public ConvertibleTWff<Tag>, public virtual_enable_shared_from_this< TBiimpBase<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
    ptwff<Tag> half_imp_not_form() const;
    void get_variables(ptvar_set<Tag> &vars) const override;
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
    bool operator==(const TWff<Tag> &x) const override;
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;

protected:
    using TWff2<Tag>::TWff2;

private:
  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_1_rp;
  static const RegisteredProver imp_not_2_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
  static const RegisteredProver tseitin4_rp;
};

template<typename Tag>
class TBiimp;

template<>
class TBiimp<PropTag> : public TBiimpBase<PropTag>, public enable_create<TBiimp<PropTag>> {
protected:
    using TBiimpBase<PropTag>::TBiimpBase;
};

template<>
class TBiimp<PredTag> : public TBiimpBase<PredTag>, public enable_create<TBiimp<PredTag>> {
protected:
    using TBiimpBase<PredTag>::TBiimpBase;
};

template<typename Tag>
class TAndBase : public TWff2<Tag>, public ConvertibleTWff<Tag>, public virtual_enable_shared_from_this< TAndBase<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
  ptwff<Tag> half_imp_not_form() const;
  void get_variables(ptvar_set<Tag> &vars) const override;
  Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
  Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
  bool operator==(const TWff<Tag> &x) const override;
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;

protected:
    using TWff2<Tag>::TWff2;

private:
  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_1_rp;
  static const RegisteredProver imp_not_2_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
};

template<typename Tag>
class TAnd;

template<>
class TAnd<PropTag> : public TAndBase<PropTag>, public enable_create<TAnd<PropTag>> {
protected:
    using TAndBase<PropTag>::TAndBase;
};

template<>
class TAnd<PredTag> : public TAndBase<PredTag>, public enable_create<TAnd<PredTag>> {
protected:
    using TAndBase<PredTag>::TAndBase;
};

template<typename Tag>
class TOrBase : public TWff2<Tag>, public ConvertibleTWff<Tag>, public virtual_enable_shared_from_this< TOrBase<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
    ptwff<Tag> half_imp_not_form() const;
    void get_variables(ptvar_set<Tag> &vars) const override;
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
    bool operator==(const TWff<Tag> &x) const override;
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;

protected:
    using TWff2<Tag>::TWff2;

private:
  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_1_rp;
  static const RegisteredProver imp_not_2_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
};

template<typename Tag>
class TOr;

template<>
class TOr<PropTag> : public TOrBase<PropTag>, public enable_create<TOr<PropTag>> {
protected:
    using TOrBase<PropTag>::TOrBase;
};

template<>
class TOr<PredTag> : public TOrBase<PredTag>, public enable_create<TOr<PredTag>> {
protected:
    using TOrBase<PredTag>::TOrBase;
};

template<typename Tag>
class TNandBase : public TWff2<Tag>, public ConvertibleTWff<Tag>, public virtual_enable_shared_from_this< TNandBase<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
    ptwff<Tag> half_imp_not_form() const;
    void get_variables(ptvar_set<Tag> &vars) const override;
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
    bool operator==(const TWff<Tag> &x) const override;
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;

protected:
    using TWff2<Tag>::TWff2;

private:
  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_1_rp;
  static const RegisteredProver imp_not_2_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
};

template<typename Tag>
class TNand;

template<>
class TNand<PropTag> : public TNandBase<PropTag>, public enable_create<TNand<PropTag>> {
protected:
    using TNandBase<PropTag>::TNandBase;
};

template<>
class TNand<PredTag> : public TNandBase<PredTag>, public enable_create<TNand<PredTag>> {
protected:
    using TNandBase<PredTag>::TNandBase;
};

template<typename Tag>
class TXorBase : public TWff2<Tag>, public ConvertibleTWff<Tag>, public virtual_enable_shared_from_this< TXorBase<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
    ptwff<Tag> half_imp_not_form() const;
    void get_variables(ptvar_set<Tag> &vars) const override;
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
    bool operator==(const TWff<Tag> &x) const override;
  void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;

protected:
    using TWff2<Tag>::TWff2;

private:
  static const RegisteredProver type_rp;
  static const RegisteredProver imp_not_1_rp;
  static const RegisteredProver imp_not_2_rp;
  static const RegisteredProver tseitin1_rp;
  static const RegisteredProver tseitin2_rp;
  static const RegisteredProver tseitin3_rp;
  static const RegisteredProver tseitin4_rp;
};

template<typename Tag>
class TXor;

template<>
class TXor<PropTag> : public TXorBase<PropTag>, public enable_create<TXor<PropTag>> {
protected:
    using TXorBase<PropTag>::TXorBase;
};

template<>
class TXor<PredTag> : public TXorBase<PredTag>, public enable_create<TXor<PredTag>> {
protected:
    using TXorBase<PredTag>::TXorBase;
};

template<typename Tag>
class TAnd3Base : public TWff3<Tag>, public ConvertibleTWff<Tag>, public virtual_enable_shared_from_this< TAnd3Base<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
    ptwff<Tag> half_imp_not_form() const;
    void get_variables(ptvar_set<Tag> &vars) const override;
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
    bool operator==(const TWff<Tag> &x) const override;
    void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;

protected:
    using TWff3<Tag>::TWff3;

private:
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
class TAnd3;

template<>
class TAnd3<PropTag> : public TAnd3Base<PropTag>, public enable_create<TAnd3<PropTag>> {
protected:
    using TAnd3Base<PropTag>::TAnd3Base;
};

template<>
class TAnd3<PredTag> : public TAnd3Base<PredTag>, public enable_create<TAnd3<PredTag>> {
protected:
    using TAnd3Base<PredTag>::TAnd3Base;
};

template<typename Tag>
class TOr3Base : public TWff3<Tag>, public ConvertibleTWff<Tag>, public virtual_enable_shared_from_this< TOr3Base<Tag> > {
    friend ptwff<Tag> wff_from_pt<Tag>(const ParsingTree<SymTok, LabTok> &pt, const LibraryToolbox &tb);
public:
    std::string to_string() const override;
    ptwff<Tag> imp_not_form() const override;
    ptwff<Tag> half_imp_not_form() const;
    void get_variables(ptvar_set<Tag> &vars) const override;
    Prover< CheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const override;
    Prover< CheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const override;
    bool operator==(const TWff<Tag> &x) const override;
    void get_tseitin_form(CNForm<Tag> &cnf, const LibraryToolbox &tb, const TWff<Tag> &glob_ctx) const override;

protected:
    using TWff3<Tag>::TWff3;

private:
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
class TOr3;

template<>
class TOr3<PropTag> : public TOr3Base<PropTag>, public enable_create<TOr3<PropTag>> {
protected:
    using TOr3Base<PropTag>::TOr3Base;
};

template<>
class TOr3<PredTag> : public TOr3Base<PredTag>, public enable_create<TOr3<PredTag>> {
protected:
    using TOr3Base<PredTag>::TOr3Base;
};

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
using pvar_set = ptvar_set<PropTag>;
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
