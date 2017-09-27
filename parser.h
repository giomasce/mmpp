#ifndef PARSER_H
#define PARSER_H

template< typename LabType >
struct ParsingTree {
    LabType label;
    std::vector< ParsingTree< LabType > > children;
};

template< typename SymType, typename LabType >
class Parser {
public:
    virtual ParsingTree< LabType > parse(const std::vector<SymType> &sent, SymType type) const = 0;
    virtual ~Parser() {}
};

#endif // PARSER_H
