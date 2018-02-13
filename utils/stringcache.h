#pragma once

#include <unordered_map>
#include <vector>

template< typename TokType >
class StringCache {
public:
    StringCache(size_t nex_id = 1) :
        next_id(nex_id) {
    }

    TokType get(std::string s) const {
        auto it = this->dir.find(s);
        if (it == this->dir.end()) {
            return {};
        } else {
            return it->second;
        }
    }

    TokType create(std::string s)
    {
        auto it = this->dir.find(s);
        if (it == this->dir.end()) {
            bool res;
            assert(this->next_id != std::numeric_limits< TokType >::max());
            std::tie(it, res) = this->dir.insert(make_pair(s, this->next_id));
            assert(res);
            std::tie(std::ignore, res) = this->inv.insert(std::make_pair(this->next_id, s));
            assert(res);
            this->next_id++;
            return it->second;
        } else {
            return {};
        }
    }

    std::string resolve(TokType id) const
    {
        auto it = this->inv.find(id);
        if (it == this->inv.end()) {
            return "";
        } else {
            return it->second;
        }
    }

    TokType get_or_create(std::string s) {
        TokType tok = this->get(s);
        if (tok == TokType{}) {
            tok = this->create(s);
        }
        return tok;
    }

    std::size_t size() const {
        return this->dir.size();
    }

    const std::unordered_map< TokType, std::string > &get_cache() const {
        return this->inv;
    }

private:
    TokType next_id;
    std::unordered_map< std::string, TokType > dir;
    std::unordered_map< TokType, std::string > inv;
};
