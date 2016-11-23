#ifndef LIBRARY_H
#define LIBRARY_H

#include <unordered_map>

typedef uint32_t Tok;

class Statement {

};

class StringCache {
public:
    Tok get(std::string s);
    Tok create(std::string s);
    std::string resolve(Tok id);
private:
    Tok next_id = 1;
    std::unordered_map< std::string, Tok > dir;
    std::unordered_map< Tok, std::string > inv;
};

class Library
{
public:
    Library();
    Tok create_symbol(std::string s);
    Tok create_label(std::string s);
private:
    StringCache syms;
    StringCache labels;
};

#endif // LIBRARY_H
