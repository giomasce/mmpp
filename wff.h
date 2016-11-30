#ifndef WFF_H
#define WFF_H

#include <string>
#include <memory>
#include <set>
#include <vector>

#include "library.h"

class Wff;
typedef std::shared_ptr< Wff > pwff;

class Wff {
public:
    virtual std::string to_string() = 0;
    virtual pwff imp_not_form() = 0;
    virtual std::vector< LabTok > prove_true(const LibraryInterface &lib);
    virtual std::vector< LabTok > prove_false(const LibraryInterface &lib);
};

class True : public Wff {
public:
    True();
    std::string to_string();
    pwff imp_not_form();
    std::vector< LabTok > prove_true(const LibraryInterface &lib);
};

class False : public Wff {
public:
    False();
    std::string to_string();
    pwff imp_not_form();
    std::vector< LabTok > prove_false(const LibraryInterface &lib);
};

class Var : public Wff {
public:
  Var(std::string name);
  std::string to_string();
  pwff imp_not_form();

private:
  std::string name;
};

class Not : public Wff {
public:
  Not(pwff a);
  std::string to_string();
  pwff imp_not_form();
  std::vector< LabTok > prove_true(const LibraryInterface &lib);
  std::vector< LabTok > prove_false(const LibraryInterface &lib);

private:
  pwff a;
};

class Imp : public Wff {
public:
  Imp(pwff a, pwff b);
  std::string to_string();
  pwff imp_not_form();

  std::vector< LabTok > prove_true(const LibraryInterface &lib);

private:
  pwff a, b;
};

class Biimp : public Wff {
public:
  Biimp(pwff a, pwff b);
  std::string to_string();
  pwff imp_not_form();

private:
  pwff a, b;
};

class And : public Wff {
public:
  And(pwff a, pwff b);
  std::string to_string();
  pwff imp_not_form();

private:
  pwff a, b;
};

class Or : public Wff {
public:
  Or(pwff a, pwff b);
  std::string to_string();
  pwff imp_not_form();

private:
  pwff a, b;
};

class Nand : public Wff {
public:
  Nand(pwff a, pwff b);
  std::string to_string();
  pwff imp_not_form();

private:
  pwff a, b;
};

class Xor : public Wff {
public:
  Xor(pwff a, pwff b);
  std::string to_string();
  pwff imp_not_form();

private:
  pwff a, b;
};

#endif // WFF_H
