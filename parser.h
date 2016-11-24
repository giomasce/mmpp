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
    std::set< Tok > vars;
    std::set< std::pair< Tok, Tok > > dists;                 // Must be stored ordered, for example using minmax()
    std::unordered_map< Tok, std::pair< Tok, Tok > > types;  // (constant, variable)
    std::unordered_map< Tok, std::vector< Tok > > hyps;
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

    bool check_var(Tok tok);

    FileTokenizer &ft;
    Library lib;
    Tok label;
    std::vector< std::string > toks;

    std::vector< ParserStackFrame > stack;
    std::set< Tok > consts;
    std::unordered_map< Tok, std::vector< Tok > > axioms;
    std::unordered_map< Tok, std::pair< std::vector< Tok >, std::vector< Tok > > > theos;
};

#endif // PARSER_H
