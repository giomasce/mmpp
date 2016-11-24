#ifndef LIBRARY_H
#define LIBRARY_H

#include <unordered_map>
#include <vector>

typedef uint32_t Tok;

class Assertion {
public:
    Assertion(bool theorem, std::vector< std::pair< Tok, Tok > > types, std::vector< std::pair< Tok, Tok > > dists, std::vector< std::vector< Tok > > hyps, std::vector< Tok > thesis, std::vector< Tok > proof = {});
private:
    bool theorem;
    std::vector< std::pair< Tok, Tok > > types;
    std::vector< std::pair< Tok, Tok > > dists;
    std::vector< std::vector< Tok > > hyps;
    std::vector< Tok > thesis;
    std::vector< Tok > proof;
};

class StringCache {
public:
    Tok get(std::string s);
    Tok create(std::string s);
    std::string resolve(Tok id);
    Tok get_or_create(std::string s);
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
    Tok get_symbol(std::string s);
    Tok get_label(std::string s);
private:
    StringCache syms;
    StringCache labels;
};

#endif // LIBRARY_H
