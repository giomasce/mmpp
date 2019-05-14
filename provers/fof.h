#pragma once

#include <iostream>

#include <giolib/memory.h>

namespace gio::mmpp::provers::fof {

class FOF {
public:
    virtual ~FOF() = default;
    virtual void print_to(std::ostream &s) const {
        s << "formula";
    }

protected:
    FOF() = default;
};

class Uninterpreted : public FOF, public gio::virtual_enable_create<Uninterpreted> {
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

protected:
    Uninterpreted(const std::string &name, const std::vector<std::shared_ptr<const FOF>> &args) : name(name), args(args) {}

private:
    std::string name;
    std::vector<std::shared_ptr<const FOF>> args;
};

class True : public FOF, public gio::virtual_enable_create<True> {
public:
    void print_to(std::ostream &s) const override {
        s << "⊤";
    }

protected:
    True() {}
};

class False : public FOF, public gio::virtual_enable_create<False> {
public:
    void print_to(std::ostream &s) const override {
        s << "⊥";
    }

protected:
    False() {}
};

class Equal : public FOF, public gio::virtual_enable_create<Equal> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "=" << *this->y << ")";
    }

protected:
    Equal(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Distinct : public FOF, public gio::virtual_enable_create<Distinct> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "≠" << *this->y << ")";
    }

protected:
    Distinct(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class And : public FOF, public gio::virtual_enable_create<And> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "∧" << *this->y << ")";
    }

protected:
    And(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Or : public FOF, public gio::virtual_enable_create<Or> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "∨" << *this->y << ")";
    }

protected:
    Or(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Iff : public FOF, public gio::virtual_enable_create<Iff> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "⇔" << *this->y << ")";
    }

protected:
    Iff(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Xor : public FOF, public gio::virtual_enable_create<Xor> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "⊻" << *this->y << ")";
    }

protected:
    Xor(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Not : public FOF, public gio::virtual_enable_create<Not> {
public:
    void print_to(std::ostream &s) const override {
        s << "¬" << *this->x;
    }

protected:
    Not(const std::shared_ptr<const FOF> &x) : x(x) {}

private:
    std::shared_ptr<const FOF> x;
};

class Implies : public FOF, public gio::virtual_enable_create<Implies> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "⇒" << *this->y << ")";
    }

protected:
    Implies(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Oeq : public FOF, public gio::virtual_enable_create<Oeq> {
public:
    void print_to(std::ostream &s) const override {
        s << "(" << *this->x << "≈" << *this->y << ")";
    }

protected:
    Oeq(const std::shared_ptr<const FOF> &x, const std::shared_ptr<const FOF> &y) : x(x), y(y) {}

private:
    std::shared_ptr<const FOF> x;
    std::shared_ptr<const FOF> y;
};

class Variable : public FOF, public gio::virtual_enable_create<Variable> {
public:
    void print_to(std::ostream &s) const override {
        s << this->name;
    }

protected:
    Variable(const std::string &name) : name(name) {}

private:
    std::string name;
};

class Forall : public FOF, public gio::virtual_enable_create<Forall> {
public:
    void print_to(std::ostream &s) const override {
        s << "∀" << this->var << " " << *this->x;
    }

protected:
    Forall(const std::shared_ptr<const Variable> &var, const std::shared_ptr<const FOF> &x) : var(var), x(x) {}

private:
    std::shared_ptr<const Variable> var;
    std::shared_ptr<const FOF> x;
};

class Exists : public FOF, public gio::virtual_enable_create<Exists> {
public:
    void print_to(std::ostream &s) const override {
        s << "∃" << this->var << " " << *this->x;
    }

protected:
    Exists(const std::shared_ptr<const Variable> &var, const std::shared_ptr<const FOF> &x) : var(var), x(x) {}

private:
    std::shared_ptr<const Variable> var;
    std::shared_ptr<const FOF> x;
};

}
