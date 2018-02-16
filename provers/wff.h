#pragma once

#include <string>
#include <memory>
#include <set>
#include <vector>

#include "mm/library.h"
#include "mm/proof.h"
#include "mm/toolbox.h"
#include "parsing/parser.h"

class Wff;
typedef std::shared_ptr< const Wff > pwff;

class Var;
typedef std::shared_ptr< const Var > pvar;
struct pvar_comp {
    bool operator()(const pvar x, const pvar y);
};
typedef std::set< pvar, pvar_comp > pvar_set;

class Wff {
public:
    virtual ~Wff();
    virtual std::string to_string() const = 0;
    virtual pwff imp_not_form() const = 0;
    virtual pwff subst(pvar var, bool positive) const = 0;
    /*virtual std::vector< SymTok > to_sentence(const Library &lib) const = 0;
    Sentence to_asserted_sentence(const Library &lib) const;
    Sentence to_wff_sentence(const Library &lib) const;*/
    ParsingTree2<SymTok, LabTok> to_parsing_tree(const LibraryToolbox &tb) const;
    virtual void get_variables(pvar_set &vars) const = 0;
    virtual Prover< AbstractCheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const;
    virtual bool is_true() const;
    virtual Prover< AbstractCheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const;
    virtual bool is_false() const;
    virtual Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
    virtual Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
    virtual Prover< AbstractCheckpointedProofEngine > get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const;
    virtual bool operator==(const Wff &x) const = 0;
    Prover< AbstractCheckpointedProofEngine > get_adv_truth_prover(const LibraryToolbox &tb) const;

private:
    Prover< AbstractCheckpointedProofEngine > adv_truth_internal(pvar_set::iterator cur_var, pvar_set::iterator end_var, const LibraryToolbox &tb) const;

    static RegisteredProver adv_truth_1_rp;
    static RegisteredProver adv_truth_2_rp;
    static RegisteredProver adv_truth_3_rp;
    static RegisteredProver adv_truth_4_rp;
};

/**
 * @brief A generic Wff that uses the imp_not form to provide truth and falsity.
 */
class ConvertibleWff : public Wff {
public:
    pwff subst(pvar var, bool positive) const;
    Prover< AbstractCheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const;
    bool is_true() const;
    Prover< AbstractCheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const;
    bool is_false() const;
    //Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;

private:
    static RegisteredProver truth_rp;
    static RegisteredProver falsity_rp;
};

class True : public Wff, public enable_create< True > {
public:
    std::string to_string() const;
    pwff imp_not_form() const;
    pwff subst(pvar var, bool positive) const;
    std::vector< SymTok > to_sentence(const Library &lib) const;
    void get_variables(pvar_set &vars) const;
    Prover< AbstractCheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const;
    bool is_true() const;
    Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
    Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
    Prover< AbstractCheckpointedProofEngine > get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const;
    bool operator==(const Wff &x) const;

protected:
    True();

private:
    static RegisteredProver truth_rp;
    static RegisteredProver type_rp;
    static RegisteredProver imp_not_rp;
    static RegisteredProver subst_rp;
};

class False : public Wff, public enable_create< False > {
public:
    std::string to_string() const;
    pwff imp_not_form() const;
    pwff subst(pvar var, bool positive) const;
    //std::vector< SymTok > to_sentence(const Library &lib) const;
    void get_variables(pvar_set &vars) const;
    Prover< AbstractCheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const;
    bool is_false() const;
    Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
    Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
    Prover< AbstractCheckpointedProofEngine > get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const;
    bool operator==(const Wff &x) const;

protected:
    False();

private:
    static RegisteredProver falsity_rp;
    static RegisteredProver type_rp;
    static RegisteredProver imp_not_rp;
    static RegisteredProver subst_rp;
};

class Var : public Wff, public enable_create< Var > {
public:
  typedef ParsingTree2< SymTok, LabTok > NameType;

  std::string to_string() const;
  pwff imp_not_form() const;
  pwff subst(pvar var, bool positive) const;
  //std::vector< SymTok > to_sentence(const Library &lib) const;
  void get_variables(pvar_set &vars) const;
  Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
  Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
  Prover< AbstractCheckpointedProofEngine > get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  bool operator<(const Var &x) const;
  NameType get_name() const {
      return this->name;
  }

protected:
  Var(NameType name, std::string string_repr);
  Var(const std::string &string_repr, const LibraryToolbox &tb);

private:
  NameType name;
  std::string string_repr;

  static RegisteredProver imp_not_rp;
  static RegisteredProver subst_pos_1_rp;
  static RegisteredProver subst_pos_2_rp;
  static RegisteredProver subst_pos_3_rp;
  static RegisteredProver subst_pos_truth_rp;
  static RegisteredProver subst_neg_1_rp;
  static RegisteredProver subst_neg_2_rp;
  static RegisteredProver subst_neg_3_rp;
  static RegisteredProver subst_neg_falsity_rp;
  static RegisteredProver subst_indep_rp;
};

class Not : public Wff, public enable_create< Not > {
public:
  std::string to_string() const;
  pwff imp_not_form() const;
  pwff subst(pvar var, bool positive) const;
  //std::vector< SymTok > to_sentence(const Library &lib) const;
  void get_variables(pvar_set &vars) const;
  Prover< AbstractCheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const;
  bool is_true() const;
  Prover< AbstractCheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const;
  bool is_false() const;
  Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
  Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
  Prover< AbstractCheckpointedProofEngine > get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }

protected:
  Not(pwff a);

private:
  pwff a;

  static RegisteredProver falsity_rp;
  static RegisteredProver type_rp;
  static RegisteredProver imp_not_rp;
  static RegisteredProver subst_rp;
};

class Imp : public Wff, public enable_create< Imp > {
public:
  std::string to_string() const;
  pwff imp_not_form() const;
  pwff subst(pvar var, bool positive) const;
  //std::vector< SymTok > to_sentence(const Library &lib) const;
  void get_variables(pvar_set &vars) const;
  Prover< AbstractCheckpointedProofEngine > get_truth_prover(const LibraryToolbox &tb) const;
  bool is_true() const;
  Prover< AbstractCheckpointedProofEngine > get_falsity_prover(const LibraryToolbox &tb) const;
  bool is_false() const;
  Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
  Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
  Prover< AbstractCheckpointedProofEngine > get_subst_prover(pvar var, bool positive, const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

protected:
  Imp(pwff a, pwff b);

private:
  pwff a, b;

  static RegisteredProver truth_1_rp;
  static RegisteredProver truth_2_rp;
  static RegisteredProver falsity_1_rp;
  static RegisteredProver falsity_2_rp;
  static RegisteredProver falsity_3_rp;
  static RegisteredProver type_rp;
  static RegisteredProver imp_not_rp;
  static RegisteredProver subst_rp;
};

class Biimp : public ConvertibleWff, public enable_create< Biimp > {
public:
  std::string to_string() const;
  pwff imp_not_form() const;
  pwff half_imp_not_form() const;
  //std::vector< SymTok > to_sentence(const Library &lib) const;
  void get_variables(pvar_set &vars) const;
  Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
  Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

protected:
  Biimp(pwff a, pwff b);

private:
  pwff a, b;

  static RegisteredProver type_rp;
  static RegisteredProver imp_not_1_rp;
  static RegisteredProver imp_not_2_rp;
};

class And : public ConvertibleWff, public enable_create< And > {
public:
  std::string to_string() const;
  //std::vector< SymTok > to_sentence(const Library &lib) const;
  pwff imp_not_form() const;
  pwff half_imp_not_form() const;
  void get_variables(pvar_set &vars) const;
  Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
  Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

protected:
  And(pwff a, pwff b);

private:
  pwff a, b;

  static RegisteredProver type_rp;
  static RegisteredProver imp_not_1_rp;
  static RegisteredProver imp_not_2_rp;
};

class Or : public ConvertibleWff, public enable_create< Or > {
public:
  std::string to_string() const;
  //std::vector< SymTok > to_sentence(const Library &lib) const;
  pwff imp_not_form() const;
  pwff half_imp_not_form() const;
  void get_variables(pvar_set &vars) const;
  Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
  Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

protected:
  Or(pwff a, pwff b);

private:
  pwff a, b;

  static RegisteredProver type_rp;
  static RegisteredProver imp_not_1_rp;
  static RegisteredProver imp_not_2_rp;
};

class Nand : public ConvertibleWff, public enable_create< Nand > {
public:
  std::string to_string() const;
  //std::vector< SymTok > to_sentence(const Library &lib) const;
  pwff imp_not_form() const;
  pwff half_imp_not_form() const;
  void get_variables(pvar_set &vars) const;
  Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
  Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

protected:
  Nand(pwff a, pwff b);

private:
  pwff a, b;

  static RegisteredProver type_rp;
  static RegisteredProver imp_not_1_rp;
  static RegisteredProver imp_not_2_rp;
};

class Xor : public ConvertibleWff, public enable_create< Xor > {
public:
  std::string to_string() const;
  //std::vector<SymTok> to_sentence(const Library &lib) const;
  pwff imp_not_form() const;
  pwff half_imp_not_form() const;
  void get_variables(pvar_set &vars) const;
  Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
  Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

protected:
  Xor(pwff a, pwff b);

private:
  pwff a, b;

  static RegisteredProver type_rp;
  static RegisteredProver imp_not_1_rp;
  static RegisteredProver imp_not_2_rp;
};

class And3 : public ConvertibleWff, public enable_create< And3 > {
public:
    std::string to_string() const;
    //std::vector<SymTok> to_sentence(const Library &lib) const;
    pwff imp_not_form() const;
    pwff half_imp_not_form() const;
    void get_variables(pvar_set &vars) const;
    Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
    Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
    bool operator==(const Wff &x) const;
    pwff get_a() const {
        return this->a;
    }
    pwff get_b() const {
        return this->b;
    }
    pwff get_c() const {
        return this->c;
    }

protected:
    And3(pwff a, pwff b, pwff c);

  private:
    pwff a, b, c;

    static RegisteredProver type_rp;
    static RegisteredProver imp_not_1_rp;
    static RegisteredProver imp_not_2_rp;
};

class Or3 : public ConvertibleWff, public enable_create< Or3 > {
public:
    std::string to_string() const;
    //std::vector<SymTok> to_sentence(const Library &lib) const;
    pwff imp_not_form() const;
    pwff half_imp_not_form() const;
    void get_variables(pvar_set &vars) const;
    Prover< AbstractCheckpointedProofEngine > get_type_prover(const LibraryToolbox &tb) const;
    Prover< AbstractCheckpointedProofEngine > get_imp_not_prover(const LibraryToolbox &tb) const;
    bool operator==(const Wff &x) const;
    pwff get_a() const {
        return this->a;
    }
    pwff get_b() const {
        return this->b;
    }
    pwff get_c() const {
        return this->c;
    }

protected:
    Or3(pwff a, pwff b, pwff c);

  private:
    pwff a, b, c;

    static RegisteredProver type_rp;
    static RegisteredProver imp_not_1_rp;
    static RegisteredProver imp_not_2_rp;
};
