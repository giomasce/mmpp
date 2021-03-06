#pragma once

#include <map>
#include <vector>
#include <unordered_map>
#include <mutex>

#include <boost/interprocess/sync/null_mutex.hpp>

#include "library.h"
#include "parsing/parser.h"

class TempGenerator {
public:
    TempGenerator(const Library &lib);
    ~TempGenerator();
    std::pair<LabTok, SymTok> new_temp_var(SymTok type_sym);
    void release_temp_var(LabTok lab);
    LabTok new_temp_label(std::string name);
    //void new_temp_var_frame();
    //void release_temp_var_frame();

    // Library-like interface
    SymTok get_symbol(std::string s);
    LabTok get_label(std::string s);
    std::string resolve_symbol(SymTok tok);
    std::string resolve_label(LabTok tok);
    size_t get_symbols_num();
    size_t get_labels_num();
    const Sentence &get_sentence(LabTok label);
    const Assertion &get_assertion(LabTok label);

    // LibraryToolbox-like interface
    LabTok get_var_sym_to_lab(SymTok sym);
    SymTok get_var_lab_to_sym(LabTok lab);
    SymTok get_var_sym_to_type_sym(SymTok sym);
    SymTok get_var_lab_to_type_sym(LabTok lab);
    const std::pair<SymTok, Sentence> &get_derivation_rule(LabTok lab);
    const ParsingTree< SymTok, LabTok > &get_parsed_sent(LabTok label);
    const ParsingTree2<SymTok, LabTok> &get_parsed_sent2(LabTok label);

private:
    void create_temp_var(SymTok type_sym);

    std::mutex global_mutex;
    //boost::interprocess::null_mutex global_mutex;

    const Library &lib;
    std::map< SymTok, size_t > temp_idx;
    std::unordered_map<LabTok, Sentence> temp_types;
    std::unordered_map<LabTok, Assertion> temp_asses;
    std::unordered_map<LabTok, ParsingTree<SymTok, LabTok>> temp_pts;
    std::unordered_map<LabTok, ParsingTree2<SymTok, LabTok>> temp_pt2s;
    StringCache< SymTok > temp_syms;
    StringCache< LabTok > temp_labs;
    std::size_t syms_base;
    std::size_t labs_base;
    std::map<SymTok, std::vector<LabTok>> free_temp_vars;

    std::vector< LabTok > var_sym_to_lab;
    std::vector< SymTok > var_lab_to_sym;
    std::vector< SymTok > var_sym_to_type_sym;
    std::vector< SymTok > var_lab_to_type_sym;
    std::unordered_map< LabTok, std::pair< SymTok, std::vector< SymTok > > > ders_by_label;
};
