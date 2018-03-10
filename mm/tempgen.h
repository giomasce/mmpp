#pragma once

#include <map>
#include <vector>
#include <unordered_map>
#include <mutex>

#include <boost/interprocess/sync/null_mutex.hpp>

#include "library.h"

class TempGenerator {
public:
    TempGenerator(const Library &lib);
    std::pair< LabTok, SymTok > new_temp_var(SymTok type_sym);
    LabTok new_temp_label(std::string name);
    void new_temp_var_frame();
    void release_temp_var_frame();

    // Library-like interface
    SymTok get_symbol(std::string s);
    LabTok get_label(std::string s);
    std::string resolve_symbol(SymTok tok);
    std::string resolve_label(LabTok tok);
    SymTok get_symbols_num();
    LabTok get_labels_num();
    const Sentence &get_sentence(LabTok label);

    // LibraryToolbox-like interface
    LabTok get_var_sym_to_lab(SymTok sym);
    SymTok get_var_lab_to_sym(LabTok lab);
    SymTok get_var_sym_to_type_sym(SymTok sym);
    SymTok get_var_lab_to_type_sym(LabTok lab);
    const std::pair<SymTok, Sentence> &get_derivation_rule(LabTok lab);

private:
    void create_temp_var(SymTok type_sym);

    std::mutex global_mutex;
    //boost::interprocess::null_mutex global_mutex;

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
    std::unordered_map< LabTok, std::pair< SymTok, std::vector< SymTok > > > ders_by_label;
};
