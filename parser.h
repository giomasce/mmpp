#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <set>
#include <unordered_map>

#include "library.h"

std::vector< std::string > tokenize(std::string in);

/*
 * Some notes on this Metamath parser:
 *  + It does not support file inclusion.
 *  + It does not enforce distinct variables.
 *  + It does not require that label tokens do not match any math token.
 *  + It does not support unknown points ("?") in proofs.
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
    std::set< std::pair< SymTok, SymTok > > dists;     // Must be stored ordered, for example using minmax()
    std::vector< LabTok > types;                       // Sentence is of type (constant, variable)
    std::set< LabTok > types_set;                      // For faster searching
    std::vector< LabTok > hyps;
};

class Parser {
public:
    Parser(FileTokenizer &ft, bool execute_proofs=true);
    void run();
    const Library &get_library() const;

private:
    void parse_c();
    void parse_v();
    void parse_f();
    void parse_e();
    void parse_d();
    void parse_a();
    void parse_p();

    bool check_var(SymTok tok) const;
    bool check_const(SymTok tok) const;
    bool check_type(LabTok tok) const;
    std::set<SymTok> collect_mand_vars(const std::vector<SymTok> &sent) const;
    std::set< SymTok > collect_opt_vars(const std::vector< LabTok > &proof, const std::set< SymTok > &mand_vars) const;
    void collect_vars_from_sentence(std::set<SymTok> &vars, const std::vector<SymTok> &sent) const;
    void collect_vars_from_proof(std::set<SymTok> &vars, const std::vector< LabTok > &proof) const;
    std::pair< int, std::vector<LabTok> > collect_mand_hyps(std::set<SymTok> vars) const;
    std::set<LabTok> collect_opt_hyps(std::set<SymTok> opt_vars) const;
    std::set<std::pair<SymTok, SymTok> > collect_mand_dists(std::set<SymTok> vars) const;
    const ParserStackFrame &get_final_frame() const;

    FileTokenizer &ft;
    bool execute_proofs;
    Library lib;
    LabTok label;
    std::vector< std::string > toks;

    std::vector< ParserStackFrame > stack;
    ParserStackFrame final_frame;
    //std::set< SymTok > consts;
};

class CompressedEncoder {
public:
    std::string push_code(CodeTok x);
};

class CompressedDecoder {
public:
    CodeTok push_char(char c);
private:
    int current = 0;
};

#endif // PARSER_H
