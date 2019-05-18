#pragma once

#include <iostream>

#include <giolib/memory.h>
#include <giolib/inheritance.h>
#include <giolib/containers.h>

namespace gio::mmpp::provers::fof {

class FOT;
class Variable;
class Functor;

struct fot_inheritance {
    typedef FOT base;
    typedef boost::mpl::vector<Variable, Functor> subtypes;
};

struct fot_cmp {
    bool operator()(const FOT &x, const FOT &y) const {
        return compare_base<fot_inheritance>(x, y);
    }
};

class FOT : public inheritance_base<fot_inheritance> {
public:
    virtual void print_to(std::ostream &s) const = 0;
    virtual bool has_free_var(const std::string &name) const = 0;
    virtual std::shared_ptr<const FOT> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const = 0;
    std::pair<std::set<std::string>, std::map<std::string, size_t>> collect_vars_functs() const;
    virtual void collect_vars_functs(std::pair<std::set<std::string>, std::map<std::string, size_t>> &vars_functs) const = 0;

protected:
    FOT() = default;
};

class Functor : public FOT, public gio::virtual_enable_create<Functor>, public inheritance_impl<Functor, fot_inheritance> {
public:
    void print_to(std::ostream &s) const override;
    bool has_free_var(const std::string &name) const override;
    std::shared_ptr<const FOT> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const override;
    void collect_vars_functs(std::pair<std::set<std::string>, std::map<std::string, size_t>> &vars_functs) const override;
    static bool compare(const Functor &x, const Functor &y);

protected:
    Functor(const std::string &name, const std::vector<std::shared_ptr<const FOT>> &args);

private:
    std::string name;
    std::vector<std::shared_ptr<const FOT>> args;
};

class Variable : public FOT, public gio::virtual_enable_create<Variable>, public inheritance_impl<Variable, fot_inheritance> {
public:
    void print_to(std::ostream &s) const override;
    bool has_free_var(const std::string &name) const override;
    std::shared_ptr<const FOT> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const override;
    void collect_vars_functs(std::pair<std::set<std::string>, std::map<std::string, size_t>> &vars_functs) const override;
    const std::string &get_name() const;
    static bool compare(const Variable &x, const Variable &y);

protected:
    Variable(const std::string &name);

private:
    std::string name;
};

class FOF;
class Predicate;
class True;
class False;
class Equal;
class Distinct;
class And;
class Or;
class Iff;
class Not;
class Xor;
class Implies;
class Oeq;
class Forall;
class Exists;

struct fof_inheritance {
    typedef FOF base;
    typedef boost::mpl::vector<Predicate, True, False, Equal, Distinct, And, Or, Iff, Not, Xor, Implies, Oeq, Forall, Exists> subtypes;
};

struct fof_cmp {
    bool operator()(const FOF &x, const FOF &y) const {
        return compare_base<fof_inheritance>(x, y);
    }
};

class FOF : public inheritance_base<fof_inheritance> {
public:
    virtual void print_to(std::ostream &s) const = 0;
    virtual bool has_free_var(const std::string &name) const = 0;
    virtual std::shared_ptr<const FOF> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const = 0;
    std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> collect_vars_functs_preds() const;
    virtual void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const = 0;

protected:
    FOF() = default;
};

class Predicate : public FOF, public gio::virtual_enable_create<Predicate>, public inheritance_impl<Predicate, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override;
    bool has_free_var(const std::string &name) const override;
    std::shared_ptr<const FOF> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const override;
    void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const override;
    static bool compare(const Predicate &x, const Predicate &y);

protected:
    Predicate(const std::string &name, const std::vector<std::shared_ptr<const FOT>> &args);

private:
    std::string name;
    std::vector<std::shared_ptr<const FOT>> args;
};

class True : public FOF, public gio::virtual_enable_create<True>, public inheritance_impl<True, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override;
    bool has_free_var(const std::string &name) const override;
    std::shared_ptr<const FOF> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const override;
    void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const override;
    static bool compare(const True &x, const True &y);

protected:
    True();
};

class False : public FOF, public gio::virtual_enable_create<False>, public inheritance_impl<False, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override;
    bool has_free_var(const std::string &name) const override;
    std::shared_ptr<const FOF> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const override;
    void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const override;
    static bool compare(const False &x, const False &y);

protected:
    False();
};

class Equal : public FOF, public gio::virtual_enable_create<Equal>, public inheritance_impl<Equal, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override;
    bool has_free_var(const std::string &name) const override;
    std::shared_ptr<const FOF> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const override;
    void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const override;
    const std::shared_ptr<const FOT> &get_left() const;
    const std::shared_ptr<const FOT> &get_right() const;
    static bool compare(const Equal &x, const Equal &y);

protected:
    Equal(const std::shared_ptr<const FOT> &x, const std::shared_ptr<const FOT> &y);

private:
    std::shared_ptr<const FOT> left;
    std::shared_ptr<const FOT> right;
};

class Distinct : public FOF, public gio::virtual_enable_create<Distinct>, public inheritance_impl<Distinct, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override;
    bool has_free_var(const std::string &name) const override;
    std::shared_ptr<const FOF> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const override;
    void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const override;
    static bool compare(const Distinct &x, const Distinct &y);

protected:
    Distinct(const std::shared_ptr<const FOT> &x, const std::shared_ptr<const FOT> &y);

private:
    std::shared_ptr<const FOT> left;
    std::shared_ptr<const FOT> right;
};

template<typename T>
class FOF2 : public FOF, public virtual_enable_shared_from_this<FOF2<T>> {
public:
    void print_to(std::ostream &s) const override;
    bool has_free_var(const std::string &name) const override;
    std::shared_ptr<const FOF> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const override;
    void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const override;
    const std::shared_ptr<const FOF> &get_left() const;
    const std::shared_ptr<const FOF> &get_right() const;
    static bool compare(const T &x, const T &y);

protected:
    FOF2(const std::shared_ptr<const FOF> &left, const std::shared_ptr<const FOF> &right);
    virtual const std::string &get_symbol() const = 0;

    std::shared_ptr<const FOF> left;
    std::shared_ptr<const FOF> right;
};

class And : public FOF2<And>, public gio::virtual_enable_create<And>, public inheritance_impl<And, fof_inheritance> {
    using FOF2::FOF2;
protected:
    virtual const std::string &get_symbol() const override final;
};

class Or : public FOF2<Or>, public gio::virtual_enable_create<Or>, public inheritance_impl<Or, fof_inheritance> {
    using FOF2::FOF2;
protected:
    virtual const std::string &get_symbol() const override final;
};

class Iff : public FOF2<Iff>, public gio::virtual_enable_create<Iff>, public inheritance_impl<Iff, fof_inheritance> {
    using FOF2::FOF2;
protected:
    virtual const std::string &get_symbol() const override final;
};

class Xor : public FOF2<Xor>, public gio::virtual_enable_create<Xor>, public inheritance_impl<Xor, fof_inheritance> {
    using FOF2::FOF2;
protected:
    virtual const std::string &get_symbol() const override final;
};

class Not : public FOF, public gio::virtual_enable_create<Not>, public inheritance_impl<Not, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override;
    bool has_free_var(const std::string &name) const override;
    std::shared_ptr<const FOF> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const override;
    void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const override;
    const std::shared_ptr<const FOF> &get_arg() const;
    static bool compare(const Not &x, const Not &y);

protected:
    Not(const std::shared_ptr<const FOF> &arg);

private:
    std::shared_ptr<const FOF> arg;
};

class Implies : public FOF2<Implies>, public gio::virtual_enable_create<Implies>, public inheritance_impl<Implies, fof_inheritance> {
    using FOF2::FOF2;
protected:
    virtual const std::string &get_symbol() const override final;
};

class Oeq : public FOF2<Oeq>, public gio::virtual_enable_create<Oeq>, public inheritance_impl<Oeq, fof_inheritance> {
    using FOF2::FOF2;
protected:
    virtual const std::string &get_symbol() const override final;
};

class Forall : public FOF, public gio::virtual_enable_create<Forall>, public inheritance_impl<Forall, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override;
    bool has_free_var(const std::string &name) const override;
    std::shared_ptr<const FOF> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const override;
    void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const override;
    const std::shared_ptr<const Variable> &get_var() const;
    const std::shared_ptr<const FOF> &get_arg() const;
    static bool compare(const Forall &x, const Forall &y);

protected:
    Forall(const std::shared_ptr<const Variable> &var, const std::shared_ptr<const FOF> &x);

private:
    std::shared_ptr<const Variable> var;
    std::shared_ptr<const FOF> arg;
};

class Exists : public FOF, public gio::virtual_enable_create<Exists>, public inheritance_impl<Exists, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override;
    bool has_free_var(const std::string &name) const override;
    std::shared_ptr<const FOF> replace(const std::string &var_name, const std::shared_ptr<const FOT> &term) const override;
    void collect_vars_functs_preds(std::tuple<std::set<std::string>, std::map<std::string, size_t>, std::map<std::string, size_t>> &vars_functs_preds) const override;
    const std::shared_ptr<const Variable> &get_var() const;
    const std::shared_ptr<const FOF> &get_arg() const;
    static bool compare(const Exists &x, const Exists &y);

protected:
    Exists(const std::shared_ptr<const Variable> &var, const std::shared_ptr<const FOF> &x);

private:
    std::shared_ptr<const Variable> var;
    std::shared_ptr<const FOF> arg;
};

extern template class FOF2<And>;
extern template class FOF2<Or>;
extern template class FOF2<Iff>;
extern template class FOF2<Xor>;
extern template class FOF2<Implies>;
extern template class FOF2<Oeq>;

}
