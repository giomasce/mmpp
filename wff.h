#ifndef WFF_H
#define WFF_H

#include <string>
#include <memory>
#include <set>
#include <vector>

#include "library.h"
#include "proof.h"
#include "toolbox.h"

class Wff;
typedef std::shared_ptr< Wff > pwff;

class Wff {
public:
    virtual ~Wff();
    virtual std::string to_string() const = 0;
    virtual pwff imp_not_form() const = 0;
    virtual pwff subst(std::string var, bool positive) const = 0;
    virtual std::vector< SymTok > to_sentence(const Library &lib) const = 0;
    virtual void get_variables(std::set< std::string > &vars) const = 0;
    virtual Prover get_truth_prover(const LibraryToolbox &tb) const;
    virtual Prover get_falsity_prover(const LibraryToolbox &tb) const;
    virtual Prover get_type_prover(const LibraryToolbox &tb) const;
    virtual Prover get_imp_not_prover(const LibraryToolbox &tb) const;
    virtual Prover get_subst_prover(std::string var, bool positive, const LibraryToolbox &tb) const;
    Prover get_adv_truth_prover(const LibraryToolbox &tb) const;
};

class ConvertibleWff : public Wff {
public:
    pwff subst(std::string var, bool positive) const;
    Prover get_truth_prover(const LibraryToolbox &tb) const;
    Prover get_falsity_prover(const LibraryToolbox &tb) const;
    Prover get_type_prover(const LibraryToolbox &tb) const;
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
    Prover get_type_prover(const LibraryToolbox &tb) const;
    Prover get_imp_not_prover(const LibraryToolbox &tb) const;
    Prover get_subst_prover(std::string var, bool positive, const LibraryToolbox &tb) const;
private:
    static RegisteredProver truth_rp;
    static RegisteredProver type_rp;
    static RegisteredProver imp_not_rp;
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
    Prover get_type_prover(const LibraryToolbox &tb) const;
    Prover get_imp_not_prover(const LibraryToolbox &tb) const;
    Prover get_subst_prover(std::string var, bool positive, const LibraryToolbox &tb) const;
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

private:
  std::string name;
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
  Prover get_falsity_prover(const LibraryToolbox &tb) const;
  Prover get_type_prover(const LibraryToolbox &tb) const;
  Prover get_imp_not_prover(const LibraryToolbox &tb) const;
  Prover get_subst_prover(std::string var, bool positive, const LibraryToolbox &tb) const;

private:
  pwff a;

  static RegisteredProver falsity_rp;
  static RegisteredProver type_rp;
  static RegisteredProver imp_not_rp;
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
  Prover get_falsity_prover(const LibraryToolbox &tb) const;
  Prover get_type_prover(const LibraryToolbox &tb) const;
  Prover get_imp_not_prover(const LibraryToolbox &tb) const;
  Prover get_subst_prover(std::string var, bool positive, const LibraryToolbox &tb) const;

private:
  pwff a, b;
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

private:
  pwff a, b;
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

private:
  pwff a, b;
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

private:
  pwff a, b;
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

private:
  pwff a, b;
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

private:
  pwff a, b;
};

#endif // WFF_H
