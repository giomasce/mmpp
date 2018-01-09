#pragma once

#include <vector>
#include <utility>

#include <boost/filesystem/path.hpp>

#include "utils/utils.h"
#include "funds.h"

std::vector< std::string > tokenize(const std::string &in);

/*
 * Some notes on this Metamath parser:
 *  + It does not require that label tokens do not match any math token.
 */

class TokenGenerator {
public:
    virtual std::pair< bool, std::string > next() = 0;
    virtual ~TokenGenerator();
};

class FileTokenizer : public TokenGenerator {
public:
    //FileTokenizer(const std::string &filename);
    FileTokenizer(const boost::filesystem::path &filename, Reportable *reportable = NULL);
    std::pair< bool, std::string > next();
    ~FileTokenizer();
private:
    FileTokenizer(std::string filename, boost::filesystem::path base_path);
    void set_file_size();
    char get_char();

    boost::filesystem::ifstream fin;
    boost::filesystem::path base_path;
    FileTokenizer *cascade;
    bool white;
    std::vector< char > buf;
    std::pair< bool, std::string > finalize_token(bool comment);
    boost::filesystem::ifstream::pos_type filesize;
    size_t pos = 0;
    Reportable *reportable;
};
