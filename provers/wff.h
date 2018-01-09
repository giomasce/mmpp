#ifndef WFF_H
#define WFF_H

#include <string>
#include <memory>
#include <set>
#include <vector>

#include "mm/library.h"
#include "mm/proof.h"
#include "mm/toolbox.h"

class Wff;
typedef std::shared_ptr< Wff > pwff;

class Wff {
public:
    virtual ~Wff();
    virtual std::string to_string() const = 0;
    virtual pwff imp_not_form() const = 0;
    virtual pwff subst(std::string var, bool positive) const = 0;
    virtual std::vector< SymTok > to_sentence(const Library &lib) const = 0;
    Sentence to_asserted_sentence(const Library &lib) const;
    Sentence to_wff_sentence(const Library &lib) const;
    virtual void get_variables(std::set< std::string > &vars) const = 0;
    virtual Prover get_truth_prover(const LibraryToolbox &tb) const;
    virtual bool is_true() const;
    virtual Prover get_falsity_prover(const LibraryToolbox &tb) const;
    virtual bool is_false() const;
    virtual Prover get_type_prover(const LibraryToolbox &tb) const;
    virtual Prover get_imp_not_prover(const LibraryToolbox &tb) const;
    virtual Prover get_subst_prover(std::string var, bool positive, const LibraryToolbox &tb) const;
    virtual bool operator==(const Wff &x) const = 0;
    Prover get_adv_truth_prover(const LibraryToolbox &tb) const;

private:
    Prover adv_truth_internal(std::set<std::string>::iterator cur_var, std::set<std::string>::iterator end_var, const LibraryToolbox &tb) const;

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
    pwff subst(std::string var, bool positive) const;
    Prover get_truth_prover(const LibraryToolbox &tb) const;
    bool is_true() const;
    Prover get_falsity_prover(const LibraryToolbox &tb) const;
    bool is_false() const;
    Prover get_type_prover(const LibraryToolbox &tb) const;

private:
    static RegisteredProver truth_rp;
    static RegisteredProver falsity_rp;
};

class True : public Wff {
public:
    True();
    std::string to_string() const;
    pwff imp_not_form() const;
    pwff subst(std::string var, bool positive) const;
    std::vector< SymTok > to_sentence(const Library &lib) const;
    void get_variables(std::set< std::string > &vars) const;
    Prover get_truth_prover(const LibraryToolbox &tb) const;
    bool is_true() const;
    Prover get_type_prover(const LibraryToolbox &tb) const;
    Prover get_imp_not_prover(const LibraryToolbox &tb) const;
    Prover get_subst_prover(std::string var, bool positive, const LibraryToolbox &tb) const;
    bool operator==(const Wff &x) const;

private:
    static RegisteredProver truth_rp;
    static RegisteredProver type_rp;
    static RegisteredProver imp_not_rp;
    static RegisteredProver subst_rp;
};

class False : public Wff {
public:
    False();
    std::string to_string() const;
    pwff imp_not_form() const;
    pwff subst(std::string var, bool positive) const;
    std::vector< SymTok > to_sentence(const Library &lib) const;
    void get_variables(std::set< std::string > &vars) const;
    Prover get_falsity_prover(const LibraryToolbox &tb) const;
    bool is_false() const;
    Prover get_type_prover(const LibraryToolbox &tb) const;
    Prover get_imp_not_prover(const LibraryToolbox &tb) const;
    Prover get_subst_prover(std::string var, bool positive, const LibraryToolbox &tb) const;
    bool operator==(const Wff &x) const;

private:
    static RegisteredProver falsity_rp;
    static RegisteredProver type_rp;
    static RegisteredProver imp_not_rp;
    static RegisteredProver subst_rp;
};

class Var : public Wff {
public:
  Var(std::string name);
  std::string to_string() const;
  pwff imp_not_form() const;
  pwff subst(std::string var, bool positive) const;
  std::vector< SymTok > to_sentence(const Library &lib) const;
  void get_variables(std::set< std::string > &vars) const;
  Prover get_type_prover(const LibraryToolbox &tb) const;
  Prover get_imp_not_prover(const LibraryToolbox &tb) const;
  Prover get_subst_prover(std::string var, bool positive, const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  std::string get_name() const {
      return this->name;
  }

private:
  std::string name;

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

class Not : public Wff {
public:
  Not(pwff a);
  std::string to_string() const;
  pwff imp_not_form() const;
  pwff subst(std::string var, bool positive) const;
  std::vector< SymTok > to_sentence(const Library &lib) const;
  void get_variables(std::set< std::string > &vars) const;
  Prover get_truth_prover(const LibraryToolbox &tb) const;
  bool is_true() const;
  Prover get_falsity_prover(const LibraryToolbox &tb) const;
  bool is_false() const;
  Prover get_type_prover(const LibraryToolbox &tb) const;
  Prover get_imp_not_prover(const LibraryToolbox &tb) const;
  Prover get_subst_prover(std::string var, bool positive, const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }

private:
  pwff a;

  static RegisteredProver falsity_rp;
  static RegisteredProver type_rp;
  static RegisteredProver imp_not_rp;
  static RegisteredProver subst_rp;
};

class Imp : public Wff {
public:
  Imp(pwff a, pwff b);
  std::string to_string() const;
  pwff imp_not_form() const;
  pwff subst(std::string var, bool positive) const;
  std::vector< SymTok > to_sentence(const Library &lib) const;
  void get_variables(std::set< std::string > &vars) const;
  Prover get_truth_prover(const LibraryToolbox &tb) const;
  bool is_true() const;
  Prover get_falsity_prover(const LibraryToolbox &tb) const;
  bool is_false() const;
  Prover get_type_prover(const LibraryToolbox &tb) const;
  Prover get_imp_not_prover(const LibraryToolbox &tb) const;
  Prover get_subst_prover(std::string var, bool positive, const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

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

class Biimp : public ConvertibleWff {
public:
  Biimp(pwff a, pwff b);
  std::string to_string() const;
  pwff imp_not_form() const;
  pwff half_imp_not_form() const;
  std::vector< SymTok > to_sentence(const Library &lib) const;
  void get_variables(std::set< std::string > &vars) const;
  Prover get_type_prover(const LibraryToolbox &tb) const;
  Prover get_imp_not_prover(const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

private:
  pwff a, b;

  static RegisteredProver type_rp;
  static RegisteredProver imp_not_1_rp;
  static RegisteredProver imp_not_2_rp;
};

class And : public ConvertibleWff {
public:
  And(pwff a, pwff b);
  std::string to_string() const;
  std::vector< SymTok > to_sentence(const Library &lib) const;
  pwff imp_not_form() const;
  pwff half_imp_not_form() const;
  void get_variables(std::set< std::string > &vars) const;
  Prover get_type_prover(const LibraryToolbox &tb) const;
  Prover get_imp_not_prover(const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

private:
  pwff a, b;

  static RegisteredProver type_rp;
  static RegisteredProver imp_not_1_rp;
  static RegisteredProver imp_not_2_rp;
};

class Or : public ConvertibleWff {
public:
  Or(pwff a, pwff b);
  std::string to_string() const;
  std::vector< SymTok > to_sentence(const Library &lib) const;
  pwff imp_not_form() const;
  pwff half_imp_not_form() const;
  void get_variables(std::set< std::string > &vars) const;
  Prover get_type_prover(const LibraryToolbox &tb) const;
  Prover get_imp_not_prover(const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

private:
  pwff a, b;

  static RegisteredProver type_rp;
  static RegisteredProver imp_not_1_rp;
  static RegisteredProver imp_not_2_rp;
};

class Nand : public ConvertibleWff {
public:
  Nand(pwff a, pwff b);
  std::string to_string() const;
  std::vector< SymTok > to_sentence(const Library &lib) const;
  pwff imp_not_form() const;
  pwff half_imp_not_form() const;
  void get_variables(std::set< std::string > &vars) const;
  Prover get_type_prover(const LibraryToolbox &tb) const;
  Prover get_imp_not_prover(const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

private:
  pwff a, b;

  static RegisteredProver type_rp;
  static RegisteredProver imp_not_1_rp;
  static RegisteredProver imp_not_2_rp;
};

class Xor : public ConvertibleWff {
public:
  Xor(pwff a, pwff b);
  std::string to_string() const;
  std::vector<SymTok> to_sentence(const Library &lib) const;
  pwff imp_not_form() const;
  pwff half_imp_not_form() const;
  void get_variables(std::set< std::string > &vars) const;
  Prover get_type_prover(const LibraryToolbox &tb) const;
  Prover get_imp_not_prover(const LibraryToolbox &tb) const;
  bool operator==(const Wff &x) const;
  pwff get_a() const {
      return this->a;
  }
  pwff get_b() const {
      return this->b;
  }

private:
  pwff a, b;

  static RegisteredProver type_rp;
  static RegisteredProver imp_not_1_rp;
  static RegisteredProver imp_not_2_rp;
};

class And3 : public ConvertibleWff {
public:
    And3(pwff a, pwff b, pwff c);
    std::string to_string() const;
    std::vector<SymTok> to_sentence(const Library &lib) const;
    pwff imp_not_form() const;
    pwff half_imp_not_form() const;
    void get_variables(std::set< std::string > &vars) const;
    Prover get_type_prover(const LibraryToolbox &tb) const;
    Prover get_imp_not_prover(const LibraryToolbox &tb) const;
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

  private:
    pwff a, b, c;

    static RegisteredProver type_rp;
    static RegisteredProver imp_not_1_rp;
    static RegisteredProver imp_not_2_rp;
};

class Or3 : public ConvertibleWff {
public:
    Or3(pwff a, pwff b, pwff c);
    std::string to_string() const;
    std::vector<SymTok> to_sentence(const Library &lib) const;
    pwff imp_not_form() const;
    pwff half_imp_not_form() const;
    void get_variables(std::set< std::string > &vars) const;
    Prover get_type_prover(const LibraryToolbox &tb) const;
    Prover get_imp_not_prover(const LibraryToolbox &tb) const;
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

  private:
    pwff a, b, c;

    static RegisteredProver type_rp;
    static RegisteredProver imp_not_1_rp;
    static RegisteredProver imp_not_2_rp;
};

#endif // WFF_H
