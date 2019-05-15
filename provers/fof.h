#pragma once

#include <iostream>

#include <giolib/memory.h>
#include <giolib/inheritance.h>
#include <giolib/containers.h>

namespace gio::mmpp::provers::fof {

class FOF;
class Uninterpreted;
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
class Variable;
class Forall;
class Exists;

struct fof_inheritance {
    typedef FOF base;
    typedef boost::mpl::vector<Uninterpreted, True, False, Equal, Distinct, And, Or, Iff, Not, Xor, Implies, Oeq, Variable, Forall, Exists> subtypes;
};

struct fof_cmp {
    bool operator()(const FOF &x, const FOF &y) const;
};

class FOF : public inheritance_base<fof_inheritance> {
public:
    virtual ~FOF() = default;
    virtual void print_to(std::ostream &s) const = 0;

protected:
    FOF() = default;
};

class Uninterpreted : public FOF, public gio::virtual_enable_create<Uninterpreted>, public inheritance_impl<Uninterpreted, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << this->name;
        if (!this->args.empty()) {
            s << "(";
            bool first = true;
            for (const auto &arg : this->args) {
                if (!first) {
                    s << ", ";
                }
                first = false;
                s << *arg;
            }
            s << ")";
        }
    }

    static bool compare(const Uninterpreted &x, const Uninterpreted &y) {
        if (x.name < y.name) { return true; }
        if (y.name < x.name) { return false; }
        return std::lexicographical_compare(x.args.begin(), x.args.end(), y.args.begin(), y.args.end(), [](const auto &x, const auto &y) {
            return fof_cmp()(*x, *y);
        });
    }

protected:
    Uninterpreted(const std::string &name, const std::vector<std::shared_ptr<const FOF>> &args) : name(name), args(args) {}

private:
    std::string name;
    std::vector<std::shared_ptr<const FOF>> args;
};

class True : public FOF, public gio::virtual_enable_create<True>, public inheritance_impl<True, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "⊤";
    }

    static bool compare(const True &x, const True &y) {
        (void) x;
        (void) y;
        return false;
    }

protected:
    True() {}
};

class False : public FOF, public gio::virtual_enable_create<False>, public inheritance_impl<False, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "⊥";
    }

    static bool compare(const False &x, const False &y) {
        (void) x;
        (void) y;
        return false;
    }

protected:
    False() {}
};

class Equal : public FOF, public gio::virtual_enable_create<Equal>, public inheritance_impl<Equal, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "=" << *this->y << ")";
    }

    static bool compare(const Equal &x, const Equal &y) {
        if (fof_cmp()(*x.x, *y.x)) { return true; }
        if (fof_cmp()(*y.x, *x.x)) { return false; }
        return fof_cmp()(*x.y, *y.y);
    }

protected:
    Equal(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Distinct : public FOF, public gio::virtual_enable_create<Distinct>, public inheritance_impl<Distinct, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "≠" << *this->y << ")";
    }

    static bool compare(const Distinct &x, const Distinct &y) {
        if (fof_cmp()(*x.x, *y.x)) { return true; }
        if (fof_cmp()(*y.x, *x.x)) { return false; }
        return fof_cmp()(*x.y, *y.y);
    }

protected:
    Distinct(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class And : public FOF, public gio::virtual_enable_create<And>, public inheritance_impl<And, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "∧" << *this->y << ")";
    }

    static bool compare(const And &x, const And &y) {
        if (fof_cmp()(*x.x, *y.x)) { return true; }
        if (fof_cmp()(*y.x, *x.x)) { return false; }
        return fof_cmp()(*x.y, *y.y);
    }

protected:
    And(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Or : public FOF, public gio::virtual_enable_create<Or>, public inheritance_impl<Or, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "∨" << *this->y << ")";
    }

    static bool compare(const Or &x, const Or &y) {
        if (fof_cmp()(*x.x, *y.x)) { return true; }
        if (fof_cmp()(*y.x, *x.x)) { return false; }
        return fof_cmp()(*x.y, *y.y);
    }

protected:
    Or(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Iff : public FOF, public gio::virtual_enable_create<Iff>, public inheritance_impl<Iff, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "⇔" << *this->y << ")";
    }

    static bool compare(const Iff &x, const Iff &y) {
        if (fof_cmp()(*x.x, *y.x)) { return true; }
        if (fof_cmp()(*y.x, *x.x)) { return false; }
        return fof_cmp()(*x.y, *y.y);
    }

protected:
    Iff(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Xor : public FOF, public gio::virtual_enable_create<Xor>, public inheritance_impl<Xor, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "⊻" << *this->y << ")";
    }

    static bool compare(const Xor &x, const Xor &y) {
        if (fof_cmp()(*x.x, *y.x)) { return true; }
        if (fof_cmp()(*y.x, *x.x)) { return false; }
        return fof_cmp()(*x.y, *y.y);
    }

protected:
    Xor(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Not : public FOF, public gio::virtual_enable_create<Not>, public inheritance_impl<Not, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "¬" << *this->x;
    }

    static bool compare(const Not &x, const Not &y) {
        return fof_cmp()(*x.x, *y.x);
    }

protected:
    Not(const std::shared_ptr<const FOF> &x) : x(x) {}

private:
    std::shared_ptr<const FOF> x;
};

class Implies : public FOF, public gio::virtual_enable_create<Implies>, public inheritance_impl<Implies, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "⇒" << *this->y << ")";
    }

    static bool compare(const Implies &x, const Implies &y) {
        if (fof_cmp()(*x.x, *y.x)) { return true; }
        if (fof_cmp()(*y.x, *x.x)) { return false; }
        return fof_cmp()(*x.y, *y.y);
    }

protected:
    Implies(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Oeq : public FOF, public gio::virtual_enable_create<Oeq>, public inheritance_impl<Oeq, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "≈" << *this->y << ")";
    }

    static bool compare(const Oeq &x, const Oeq &y) {
        if (fof_cmp()(*x.x, *y.x)) { return true; }
        if (fof_cmp()(*y.x, *x.x)) { return false; }
        return fof_cmp()(*x.y, *y.y);
    }

protected:
    Oeq(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Variable : public FOF, public gio::virtual_enable_create<Variable>, public inheritance_impl<Variable, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << this->name;
    }

    static bool compare(const Variable &x, const Variable &y) {
        return x.name < y.name;
    }

protected:
    Variable(const std::string &name) : name(name) {}

private:
    std::string name;
};

class Forall : public FOF, public gio::virtual_enable_create<Forall>, public inheritance_impl<Forall, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "∀" << *this->var << " " << *this->x;
    }

    static bool compare(const Forall &x, const Forall &y) {
        if (fof_cmp()(*x.var, *y.var)) { return true; }
        if (fof_cmp()(*y.var, *x.var)) { return false; }
        return fof_cmp()(*x.x, *y.x);
    }

protected:
    Forall(const std::shared_ptr<const Variable> &var, const std::shared_ptr<const FOF> &x) : var(var), x(x) {}

private:
    std::shared_ptr<const Variable> var;
    std::shared_ptr<const FOF> x;
};

class Exists : public FOF, public gio::virtual_enable_create<Exists>, public inheritance_impl<Exists, fof_inheritance> {
public:
    void print_to(std::ostream &s) const override {
        s << "∃" << *this->var << " " << *this->x;
    }

    static bool compare(const Exists &x, const Exists &y) {
        if (fof_cmp()(*x.var, *y.var)) { return true; }
        if (fof_cmp()(*y.var, *x.var)) { return false; }
        return fof_cmp()(*x.x, *y.x);
    }

protected:
    Exists(const std::shared_ptr<const Variable> &var, const std::shared_ptr<const FOF> &x) : var(var), x(x) {}

private:
    std::shared_ptr<const Variable> var;
    std::shared_ptr<const FOF> x;
};

inline bool fof_cmp::operator()(const FOF &x, const FOF &y) const {
    return compare_base<fof_inheritance>(x, y);
}

}
