#ifndef STRINGCACHE_H
#define STRINGCACHE_H

#include <unordered_map>
#include <vector>

template< typename Tok >
class StringCache {
public:
    Tok get(std::string s) const {
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
            assert(this->inv.size() == this->next_id);
            this->inv.push_back(s);
            this->next_id++;
            return it->second;
        } else {
            return 0;
        }
    }
    std::string resolve(Tok id) const
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
    std::size_t size() const {
        return this->dir.size();
    }
    const std::vector< std::string > &get_cache() const {
        return this->inv;
    }

private:
    Tok next_id = 1;
    std::unordered_map< std::string, Tok > dir;
    std::vector< std::string > inv = std::vector< std::string >(1);
};

#endif // STRINGCACHE_H
