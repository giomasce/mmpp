#ifndef PARSER_H
#define PARSER_H

#include <vector>

template< typename SymType, typename LabType >
struct ParsingTree {
    LabType label;
    SymType type;
    std::vector< ParsingTree< SymType, LabType > > children;

    bool operator==(const ParsingTree< SymType, LabType > &other) const {
        // The type does not participate in equality check, since it is assumed that it is a function of the label
        return this->label == other.label && this->children == other.children;
    }
};

template< typename SymType, typename LabType >
class Parser {
public:
    virtual ParsingTree< SymType, LabType > parse(const std::vector<SymType> &sent, SymType type) const {
        return this->parse(sent.begin(), sent.end(), type);
    }
    virtual ParsingTree< SymType, LabType > parse(typename std::vector<SymType>::const_iterator sent_begin, typename std::vector<SymType>::const_iterator sent_end, SymType type) const = 0;
    virtual ~Parser() {}
};

template< typename SymType, typename LabType >
void reconstruct_sentence_internal(const ParsingTree< SymType, LabType > &parsing_tree,
                                                     const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations,
                                                     const std::unordered_map< LabType, std::pair< SymType, std::vector< SymType > > > &ders_by_lab,
                                                     std::back_insert_iterator< std::vector< SymType > > it) {
    const auto &rule = ders_by_lab.at(parsing_tree.label);
    auto pt_it = parsing_tree.children.begin();
    for (const auto &sym : rule.second) {
        if (derivations.find(sym) == derivations.end()) {
            it = sym;
        } else {
            assert(pt_it != parsing_tree.children.end());
            reconstruct_sentence_internal(*pt_it, derivations, ders_by_lab, it);
            pt_it++;
        }
    }
    assert(pt_it == parsing_tree.children.end());
}

template< typename SymType, typename LabType >
std::vector< SymType > reconstruct_sentence(const ParsingTree< SymType, LabType > &parsing_tree,
                                            const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations,
                                            const std::unordered_map< LabType, std::pair< SymType, std::vector< SymType > > > &ders_by_lab) {
    std::vector< SymType > res;
    reconstruct_sentence_internal(parsing_tree, derivations, ders_by_lab, std::back_inserter(res));
    return res;
}

template< typename SymType, typename LabType >
std::unordered_map< LabType, std::pair< SymType, std::vector< SymType > > > compute_derivations_by_label(const std::unordered_map<SymType, std::vector<std::pair<LabType, std::vector<SymType> > > > &derivations) {
    std::unordered_map< LabType, std::pair< SymType, std::vector< SymType > > > ders;
    for (const auto &der_sym : derivations) {
        for (const auto &der : der_sym.second) {
            ders.insert(make_pair(der.first, make_pair(der_sym.first, der.second)));
        }
    }
    return ders;
}

#endif // PARSER_H
