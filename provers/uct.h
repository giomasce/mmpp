#pragma once

#include <vector>
#include <memory>
#include <map>
#include <cstdint>
#include <random>

#include <boost/range/join.hpp>

#include <giolib/memory.h>

#include "utils/utils.h"
#include "parsing/parser.h"
#include "parsing/unif.h"
#include "mm/library.h"
#include "mm/toolbox.h"
#include "mm/engine.h"

class SentenceNode;
class StepNode;
class UCTProver;

enum VisitResult {
    PROVED,
    CONTINUE,
    DEAD,
};

class UCTProver : public gio::virtual_enable_create< UCTProver > {
public:
    VisitResult visit();
    const std::vector< ParsingTree2< SymTok, LabTok > > &get_hypotheses() const;
    const LibraryToolbox &get_toolbox() const;
    temp_allocator &get_temp_allocator();
    std::ranlux48 &get_rand();
    const std::set<std::pair<LabTok, LabTok> > &get_antidists() const;
    void replay_proof(CheckpointedProofEngine &engine) const;
    bool is_assertion_useful(const Assertion &ass) const;
    const std::unordered_map< LabTok, std::vector< LabTok > > &get_root_useful_asses() const;
    const std::unordered_map< LabTok, std::vector< LabTok > > &get_imp_con_useful_asses() const;
    void set_children_callbacks(std::vector< std::function< void() > > &&children_callbacks);
    std::function< void() > get_children_callback(size_t idx) const;

protected:
    UCTProver(const LibraryToolbox &tb, const ParsingTree2< SymTok, LabTok > &thesis, const std::vector< ParsingTree2< SymTok, LabTok > > &hypotheses, const std::set< std::pair< LabTok, LabTok > > &antidists = {});
    ~UCTProver();
    void init();

private:
    void compute_useful_assertions();

    std::shared_ptr< SentenceNode > root;
    std::set< std::pair< LabTok, LabTok > > antidists;
    const LibraryToolbox &tb;
    temp_stacked_allocator tsa;
    ParsingTree2< SymTok, LabTok > thesis;
    std::vector< ParsingTree2< SymTok, LabTok > > hypotheses;
    std::unordered_map< LabTok, std::vector< LabTok > > root_useful_asses;
    std::unordered_map< LabTok, std::vector< LabTok > > imp_con_useful_asses;
    std::ranlux48 rand;
    std::vector< std::function< void() > > children_callbacks;
};

class SentenceNode : public gio::virtual_enable_create< SentenceNode > {
public:
    VisitResult visit();
    float get_value();
    uint32_t get_visit_num();
    std::weak_ptr< StepNode > get_parent();
    const ParsingTree2< SymTok, LabTok > &get_sentence();
    void replay_proof(CheckpointedProofEngine &engine) const;

protected:
    SentenceNode(std::weak_ptr< UCTProver > uct, std::weak_ptr< StepNode > parent, const ParsingTree2< SymTok, LabTok > &sentence);
    ~SentenceNode();

private:
    bool check_subst_map(const SubstMap2< SymTok, LabTok > &subst_map, const Assertion &ass);

    std::weak_ptr< UCTProver > uct;
    std::vector< std::shared_ptr< StepNode > > children;
    std::weak_ptr< StepNode > parent;

    ParsingTree2< SymTok, LabTok > sentence;
    uint32_t visit_num = 0;
    bool exhausted = false;
    size_t hyp_num = 0;
    float value = 0.0;
    float total_children_value = 0.0;
    boost::range::joined_range< const std::vector< LabTok >, const std::vector< LabTok > > ass_range;
    boost::range::joined_range< const std::vector< LabTok >, const std::vector< LabTok > >::iterator ass_it;
};

class StepNode : public gio::virtual_enable_create< StepNode > {
public:
    VisitResult visit();
    float get_value() const;
    uint32_t get_visit_num() const;
    std::weak_ptr< SentenceNode > get_parent() const;
    void replay_proof(CheckpointedProofEngine &engine) const;
    const std::map< LabTok, gio::safe_weak_ptr< StepNode > > &get_open_vars() const;

protected:
    StepNode(std::weak_ptr< UCTProver > uct, std::weak_ptr< SentenceNode > parent, LabTok label, const SubstMap2< SymTok, LabTok > &const_subst_map, const std::map< LabTok, gio::safe_weak_ptr< StepNode > > &open_vars);
    ~StepNode();

private:
    VisitResult create_child(const ParsingTree2< SymTok, LabTok > &sent);
    VisitResult create_children();
    VisitResult visit_child(size_t i);

    std::weak_ptr< UCTProver > uct;
    std::vector< std::shared_ptr< SentenceNode > > children;
    std::weak_ptr< SentenceNode > parent;
    std::vector< std::shared_ptr< SentenceNode > > active_children;
    size_t worst_child = 0;

    LabTok label;
    SubstMap2< SymTok, LabTok > const_subst_map;
    SubstMap2< SymTok, LabTok > unconst_subst_map;
    bool exhausted = false;
    /*float value = 0.0;
    uint32_t visit_num = 0;*/
    std::map< LabTok, gio::safe_weak_ptr< StepNode > > open_vars;
};
