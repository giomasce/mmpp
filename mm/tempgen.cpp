#include "tempgen.h"

#include <iostream>
#include <mutex>

#include <giolib/containers.h>

TempGenerator::TempGenerator(const Library &lib) : lib(lib),
    temp_syms(SymTok(static_cast<SymTok::val_type>(lib.get_symbols_num())+1)), temp_labs(LabTok(static_cast<LabTok::val_type>(lib.get_labels_num())+1)),
    syms_base(lib.get_symbols_num()+1), labs_base(lib.get_labels_num()+1)
{
    assert(this->lib.is_immutable());
}

TempGenerator::~TempGenerator()
{
    size_t alloc_num = this->var_lab_to_sym.size();
    size_t dealloc_num = 0;
    for (const auto &free_tv : this->free_temp_vars) {
        dealloc_num += free_tv.second.size();
    }
    if (alloc_num != dealloc_num) {
        std::cerr << alloc_num - dealloc_num << " temporaries were allocated but never deallocated\n";
    }
}

void TempGenerator::create_temp_var(SymTok type_sym)
{
    // Create names and variables
    assert(lib.is_constant(type_sym));
    size_t idx = ++this->temp_idx[type_sym];
    std::string sym_name = this->lib.resolve_symbol(type_sym) + std::to_string(idx);
    assert(this->lib.get_symbol(sym_name) == SymTok{});
    std::string lab_name = "temp" + sym_name;
    assert(this->lib.get_label(lab_name) == LabTok{});
    SymTok sym = this->temp_syms.create(sym_name);
    LabTok lab = this->temp_labs.create(lab_name);
    this->temp_types[lab] = { type_sym, sym };
    this->temp_asses[lab] = {};
    this->temp_pts[lab] = { lab, type_sym, {} };
    this->temp_pt2s[lab] = pt_to_pt2(this->temp_pts[lab]);

    // Add the variables to a few structures
    //this->derivations.at(type_sym).push_back(pair< LabTok, vector< SymTok > >(lab, { sym }));
    this->ders_by_label[lab] = std::pair< SymTok, std::vector< SymTok > >(type_sym, { sym });
    gio::enlarge_and_set(this->var_lab_to_sym, lab.val() - this->labs_base) = sym;
    gio::enlarge_and_set(this->var_sym_to_lab, sym.val() - this->syms_base) = lab;
    gio::enlarge_and_set(this->var_lab_to_type_sym, lab.val() - this->labs_base) = type_sym;
    gio::enlarge_and_set(this->var_sym_to_type_sym, sym.val() - this->syms_base) = type_sym;

    // And insert it to the free list
    this->free_temp_vars[type_sym].push_back(lab);
}

std::pair<LabTok, SymTok> TempGenerator::new_temp_var(SymTok type_sym)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    if (this->free_temp_vars[type_sym].empty()) {
        this->create_temp_var(type_sym);
    }
    auto lab = this->free_temp_vars[type_sym].back();
    this->free_temp_vars[type_sym].pop_back();
    return std::make_pair(lab, this->var_lab_to_sym.at(lab.val() - this->labs_base));
}

void TempGenerator::release_temp_var(LabTok lab)
{
    std::unique_lock<std::mutex> lock(this->global_mutex);

    auto type_sym = this->var_lab_to_type_sym.at(lab.val() - this->labs_base);
    this->free_temp_vars[type_sym].push_back(lab);
}

LabTok TempGenerator::new_temp_label(std::string name)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    LabTok tok = this->temp_labs.get(name);
    if (tok == LabTok{}) {
        assert(this->lib.get_label(name) == LabTok{});
        tok = this->temp_labs.create(name);
    }
    assert(tok != LabTok{});
    return tok;
}

/*void TempGenerator::new_temp_var_frame()
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    std::map< SymTok, size_t > x;
    for (const auto &v : this->used_temp_vars) {
        x[v.first] = v.second.size();
    }
    this->temp_vars_stack.push_back(x);
}

void TempGenerator::release_temp_var_frame()
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    const auto &stack_pos = this->temp_vars_stack.back();
    for (auto &v : this->used_temp_vars) {
        SymTok type_sym = v.first;
        auto &used_vars = v.second;
        auto &free_vars = this->free_temp_vars.at(type_sym);
        auto it = stack_pos.find(type_sym);
        size_t pos = 0;
        if (it != stack_pos.end()) {
            pos = it->second;
        }
        for (auto it = used_vars.begin()+pos; it != used_vars.end(); it++) {
            free_vars.push_back(*it);
        }
        used_vars.resize(pos);
    }
    this->temp_vars_stack.pop_back();
}*/

SymTok TempGenerator::get_symbol(std::string s)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->temp_syms.get(s);
}

LabTok TempGenerator::get_label(std::string s)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->temp_labs.get(s);
}

std::string TempGenerator::resolve_symbol(SymTok tok)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->temp_syms.resolve(tok);
}

std::string TempGenerator::resolve_label(LabTok tok)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->temp_labs.resolve(tok);
}

size_t TempGenerator::get_symbols_num()
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->temp_syms.size();
}

size_t TempGenerator::get_labels_num()
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->temp_labs.size();
}

const Sentence &TempGenerator::get_sentence(LabTok label)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->temp_types.at(label);
}

const Assertion &TempGenerator::get_assertion(LabTok label)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->temp_asses.at(label);
}

LabTok TempGenerator::get_var_sym_to_lab(SymTok sym)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->var_sym_to_lab.at(sym.val() - this->syms_base);
}

SymTok TempGenerator::get_var_lab_to_sym(LabTok lab)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->var_lab_to_sym.at(lab.val() - this->labs_base);
}

SymTok TempGenerator::get_var_sym_to_type_sym(SymTok sym)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->var_sym_to_type_sym.at(sym.val() - this->syms_base);
}

SymTok TempGenerator::get_var_lab_to_type_sym(LabTok lab)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->var_lab_to_type_sym.at(lab.val() - this->labs_base);
}

const std::pair<SymTok, Sentence> &TempGenerator::get_derivation_rule(LabTok lab)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->ders_by_label.at(lab);
}

const ParsingTree<SymTok, LabTok> &TempGenerator::get_parsed_sent(LabTok label)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->temp_pts.at(label);
}

const ParsingTree2<SymTok, LabTok> &TempGenerator::get_parsed_sent2(LabTok label)
{
    std::unique_lock< std::mutex > lock(this->global_mutex);

    return this->temp_pt2s.at(label);
}
