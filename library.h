#ifndef LIBRARY_H
#define LIBRARY_H

#include <unordered_map>
#include <vector>
#include <utility>
#include <tuple>
#include <limits>
#include <set>
#include <type_traits>

#include <cassert>

typedef uint16_t SymTok;
typedef uint32_t LabTok;

static_assert(std::is_integral< SymTok >::value);
static_assert(std::is_unsigned< SymTok >::value);
static_assert(std::is_integral< LabTok >::value);
static_assert(std::is_unsigned< LabTok >::value);

class Proof {

};

class Assertion {
public:
    Assertion();
    Assertion(bool theorem,
              std::set< std::pair< SymTok, SymTok > > dists,
              std::vector< LabTok > hyps,
              LabTok thesis,
              std::vector< LabTok > proof = {});
    bool is_valid();

private:
    bool valid;
    bool theorem;
    std::set< std::pair< SymTok, SymTok > > dists;
    std::vector< LabTok > hyps;
    LabTok thesis;
    std::vector< LabTok > proof;
};

template< typename Tok >
class StringCache {
public:
    Tok get(std::string s) {
        auto it = this->dir.find(s);
        if (it == this->dir.end()) {
            return 0;
        } else {
            return it->second;
        }
    }
    Tok create(std::string s)
    {
        auto it = this->dir.find(s);
        if (it == this->dir.end()) {
            bool res;
            assert(this->next_id != std::numeric_limits< Tok >::max());
            tie(it, res) = this->dir.insert(make_pair(s, this->next_id));
            assert(res);
            std::tie(std::ignore, res) = this->inv.insert(make_pair(this->next_id, s));
            assert(res);
            this->next_id++;
            return it->second;
        } else {
            return 0;
        }
    }
    std::string resolve(Tok id)
    {
        return this->inv.at(id);
    }
    Tok get_or_create(std::string s) {
        Tok tok = this->get(s);
        if (tok == 0) {
            tok = this->create(s);
        }
        return tok;
    }
    std::size_t size() {
        return this->dir.size();
    }
private:
    Tok next_id = 1;
    std::unordered_map< std::string, Tok > dir;
    std::unordered_map< Tok, std::string > inv;
};

class Library
{
public:
    Library();
    SymTok create_symbol(std::string s);
    LabTok create_label(std::string s);
    SymTok get_symbol(std::string s);
    LabTok get_label(std::string s);
    std::size_t get_symbol_num();
    std::size_t get_label_num();
    void add_sentence(LabTok label, std::vector< SymTok > content);
    std::vector<SymTok> get_sentence(LabTok label);
    void add_assertion(LabTok label, const Assertion &ass);
    Assertion get_assertion(LabTok label);

private:
    StringCache< SymTok > syms;
    StringCache< LabTok > labels;

    // Vector is more efficient if labels are known to be contiguous and starting from 1; in the general case the unordered_map might be better
    //std::unordered_map< LabTok, std::vector< SymTok > > sentences;
    std::vector< std::vector< SymTok > > sentences;
    std::vector< Assertion > assertions;
};

#endif // LIBRARY_H
