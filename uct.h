#ifndef UCT_H
#define UCT_H

#include <vector>
#include <memory>
#include <map>
#include <cstdint>
#include <random>

#include "utils/utils.h"
#include "parsing/parser.h"
#include "parsing/unif.h"
#include "library.h"
#include "toolbox.h"

class SentenceNode;
class StepNode;
class UCTProver;

class UCTProver : public enable_create< UCTProver > {
public:
    bool visit();
    const std::vector< ParsingTree2< SymTok, LabTok > > &get_hypotheses() const;
    const std::vector< const Assertion* > &get_useful_asses() const;
    LibraryToolbox &get_toolbox() const;
    std::ranlux48 &get_rand();

protected:
    UCTProver(LibraryToolbox &tb, const ParsingTree2< SymTok, LabTok > &thesis, const std::vector< ParsingTree2< SymTok, LabTok > > &hypotheses);
    ~UCTProver();
    void init();

private:
    void compute_useful_assertions();

    std::shared_ptr< SentenceNode > root;
    LibraryToolbox &tb;
    ParsingTree2< SymTok, LabTok > thesis;
    std::vector< ParsingTree2< SymTok, LabTok > > hypotheses;
    std::vector< const Assertion* > useful_asses;
    std::ranlux48 rand;
};

class SentenceNode : public enable_create< SentenceNode > {
public:
    bool visit();
    float get_value();
    uint32_t get_visit_num();

protected:
    SentenceNode(std::weak_ptr< UCTProver > uct, const ParsingTree2< SymTok, LabTok > &sentence);
    ~SentenceNode();

private:
    std::vector< std::shared_ptr< StepNode > > children;
    std::weak_ptr< UCTProver > uct;

    ParsingTree2< SymTok, LabTok > sentence;
    uint32_t visit_num = 0;
    bool proved = false;
    float value = 0.0;
    float total_children_value = 0.0;
    std::vector< const Assertion* >::const_iterator ass_it;
};

class StepNode : public enable_create< StepNode > {
public:
    bool visit();
    float get_value();
    uint32_t get_visit_num();

protected:
    StepNode(std::weak_ptr< UCTProver > uct, LabTok label, const SubstMap2< SymTok, LabTok > &const_subst_map);
    ~StepNode();

private:
    bool create_children();
    bool visit_child(size_t i);

    std::vector< std::shared_ptr< SentenceNode > > children;
    std::vector< std::shared_ptr< SentenceNode > > active_children;
    size_t worst_child = 0;
    std::weak_ptr< UCTProver > uct;

    LabTok label;
    SubstMap2< SymTok, LabTok > const_subst_map;
    SubstMap2< SymTok, LabTok > unconst_subst_map;
    bool proved = false;
    /*float value = 0.0;
    uint32_t visit_num = 0;*/
};

#endif // UCT_H
