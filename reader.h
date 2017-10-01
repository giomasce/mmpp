#ifndef READER_H
#define READER_H

#include <vector>
#include <string>
#include <set>
#include <unordered_map>

#include <boost/filesystem.hpp>

#include <cryptopp/sha.h>

#include "library.h"

std::vector< std::string > tokenize(const std::string &in);

/*
 * Some notes on this Metamath parser:
 *  + It does not require that label tokens do not match any math token.
 */

class TokenGenerator {
public:
    virtual std::pair< bool, std::string > next() = 0;
    virtual void compute_digest() = 0;
    virtual std::string get_digest() const = 0;
    virtual ~TokenGenerator();
};

class FileTokenizer : public TokenGenerator {
public:
    //FileTokenizer(const std::string &filename);
    FileTokenizer(const boost::filesystem::path &filename);
    std::pair< bool, std::string > next();
    void compute_digest();
    std::string get_digest() const;
    ~FileTokenizer();
private:
    FileTokenizer(std::string filename, boost::filesystem::path base_path);
    char get_char();

    boost::filesystem::ifstream fin;
    boost::filesystem::path base_path;
    FileTokenizer *cascade;
    bool white;
    std::vector< char > buf;
    std::pair< bool, std::string > finalize_token(bool comment);
    CryptoPP::SHA256 hasher;
    byte digest[CryptoPP::SHA256::DIGESTSIZE];
};

class Reader {
public:
    Reader(TokenGenerator &tg, bool execute_proofs=true, bool store_comments=false);
    void run();
    const LibraryImpl &get_library() const;

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
    std::vector<std::vector<std::pair<bool, std::string> > > parse_comment(const std::string &comment);
    void parse_t_comment(const std::string &comment);
    void parse_t_code(const std::vector<std::vector<std::pair<bool, std::string> > > &code);
    void parse_j_comment(const std::string &comment);
    void parse_j_code(const std::vector<std::vector<std::pair<bool, std::string> > > &code);

    bool check_var(SymTok tok) const;
    bool check_const(SymTok tok) const;
    bool check_type(LabTok tok) const;
    std::set<SymTok> collect_mand_vars(const std::vector<SymTok> &sent) const;
    std::set< SymTok > collect_opt_vars(const std::vector< LabTok > &proof, const std::set< SymTok > &mand_vars) const;
    void collect_vars_from_sentence(std::set<SymTok> &vars, const std::vector<SymTok> &sent) const;
    void collect_vars_from_proof(std::set<SymTok> &vars, const std::vector< LabTok > &proof) const;
    std::pair< std::vector< LabTok >, std::vector<LabTok> > collect_mand_hyps(std::set<SymTok> vars) const;
    std::set<LabTok> collect_opt_hyps(std::set<SymTok> opt_vars) const;
    std::set<std::pair<SymTok, SymTok> > collect_mand_dists(std::set<SymTok> vars) const;
    std::set<std::pair<SymTok, SymTok> > collect_opt_dists(std::set<SymTok> opt_vars, std::set<SymTok> mand_vars) const;
    const StackFrame &get_final_frame() const;

    TokenGenerator *tg;
    bool execute_proofs;
    bool store_comments;
    LibraryImpl lib;
    LabTok label;
    LabTok number;
    std::string last_comment;
    std::vector< std::string > toks;
    std::string t_comment;
    std::string j_comment;

    std::vector< StackFrame > stack;
    StackFrame final_frame;
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

#endif // READER_H
