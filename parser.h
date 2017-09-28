#ifndef PARSER_H
#define PARSER_H

#include <vector>

template< typename LabType >
struct ParsingTree {
    LabType label;
    std::vector< ParsingTree< LabType > > children;

    bool operator==(const ParsingTree< LabType > &other) const {
        return this->label == other.label && this->children == other.children;
    }
};

template< typename SymType, typename LabType >
class Parser {
public:
    virtual ParsingTree< LabType > parse(const std::vector<SymType> &sent, SymType type) const = 0;
    virtual ~Parser() {}
};

#endif // PARSER_H
