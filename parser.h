#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <set>
#include <unordered_map>

#include <boost/filesystem.hpp>

#include "library.h"

std::vector< std::string > tokenize(const std::string &in);

/*
 * Some notes on this Metamath parser:
 *  + It does not require that label tokens do not match any math token.
 *  + It does not support unknown points ("?") in proofs.
 */

class TokenGenerator {
public:
    virtual std::pair< bool, std::string > next() = 0;
    virtual ~TokenGenerator();
};

class FileTokenizer : public TokenGenerator {
public:
    FileTokenizer(std::string filename);
    std::pair< bool, std::string > next();
    ~FileTokenizer();
private:
    FileTokenizer(std::string filename, boost::filesystem::path base_path);
    std::fstream fin;
    boost::filesystem::path base_path;
    FileTokenizer *cascade;
    bool white;
    std::vector< char > buf;
    std::pair< bool, std::string > finalize_token(bool comment);
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
    Parser(TokenGenerator &tg, bool execute_proofs=true, bool store_comments=false);
    void run();
    const Library &get_library() const;

private:
    std::pair< bool, std::string > next_token();
    void parse_c();
    void parse_v();
    void parse_f();
    void parse_e();
    void parse_d();
    void parse_a();
    void parse_p();
    void process_comment(const std::string &comment);
    void parse_t_comment(const std::string &comment);

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
    std::set<std::pair<SymTok, SymTok> > collect_opt_dists(std::set<SymTok> opt_vars, std::set<SymTok> mand_vars) const;
    const ParserStackFrame &get_final_frame() const;

    TokenGenerator *tg;
    bool execute_proofs;
    bool store_comments;
    Library lib;
    LabTok label;
    std::string last_comment;
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
