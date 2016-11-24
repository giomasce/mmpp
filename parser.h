#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <set>
#include <unordered_map>

#include "library.h"

/*
 * Some notes on this Metamath parser:
 *  + At the moment it does not support file inclusion. File inclusions
 *    are treated (more or less) like comments.
 *  + It does not require that label tokens do not match any math token.
 *  + It does not support compressed proofs.
 */

class FileTokenizer {
public:
    FileTokenizer(std::istream &in);
    std::string next();
private:
    std::istream &in;
    bool white;
    std::vector< char > buf;
    std::string finalize_token();
};

struct ParserStackFrame {
    std::set< SymTok > vars;
    std::set< std::pair< SymTok, SymTok > > dists;                 // Must be stored ordered, for example using minmax()
    std::unordered_map< LabTok, std::pair< SymTok, SymTok > > types;  // (constant, variable)
    std::unordered_map< LabTok, std::vector< SymTok > > hyps;
};

class Parser {
public:
    Parser(FileTokenizer &ft);
    void run();
private:
    void parse_c();
    void parse_v();
    void parse_f();
    void parse_e();
    void parse_d();
    void parse_a();
    void parse_p();

    bool check_var(SymTok tok);

    FileTokenizer &ft;
    Library lib;
    LabTok label;
    std::vector< std::string > toks;

    std::vector< ParserStackFrame > stack;
    std::set< SymTok > consts;
};

#endif // PARSER_H
