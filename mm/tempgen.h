#pragma once

#include <map>
#include <vector>
#include <unordered_map>

#include "library.h"

class TempGenerator {
public:
    TempGenerator(const Library &lib);
    std::pair< LabTok, SymTok > new_temp_var(SymTok type_sym);
    LabTok new_temp_label(std::string name);
    void new_temp_var_frame();
    void release_temp_var_frame();

    // Library-like interface
    SymTok get_symbol(std::string s) const;
    LabTok get_label(std::string s) const;
    std::string resolve_symbol(SymTok tok) const;
    std::string resolve_label(LabTok tok) const;
    std::size_t get_symbols_num() const;
    std::size_t get_labels_num() const;
    const Sentence &get_sentence(LabTok label) const;

    // LibraryToolbox-like interface
    LabTok get_var_sym_to_lab(SymTok sym) const;
    SymTok get_var_lab_to_sym(LabTok lab) const;
    SymTok get_var_sym_to_type_sym(SymTok sym) const;
    SymTok get_var_lab_to_type_sym(LabTok lab) const;

private:
    void create_temp_var(SymTok type_sym);

    const Library &lib;
    std::map< SymTok, size_t > temp_idx;
    std::unordered_map< LabTok, Sentence > temp_types;
    StringCache< SymTok > temp_syms;
    StringCache< LabTok > temp_labs;
    std::size_t syms_base;
    std::size_t labs_base;
    std::map< SymTok, std::vector< std::pair< LabTok, SymTok > > > free_temp_vars;
    std::map< SymTok, std::vector< std::pair< LabTok, SymTok > > > used_temp_vars;
    std::vector< std::map< SymTok, size_t > > temp_vars_stack;

    std::vector< LabTok > var_sym_to_lab;
    std::vector< SymTok > var_lab_to_sym;
    std::vector< SymTok > var_sym_to_type_sym;
    std::vector< SymTok > var_lab_to_type_sym;
};
