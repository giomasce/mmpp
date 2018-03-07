#pragma once

#include <vector>
#include <algorithm>
#include <stdexcept>

template< typename Key, typename Value >
class VectorMap {
public:

    class Iterator;
    class ConstIterator;

    typedef Key key_type;
    typedef Value mapped_type;
    typedef Iterator iterator;
    typedef ConstIterator const_iterator;
    typedef std::pair< const Key, Value > value_type;

    VectorMap() {
    }

    void reserve(size_t count) {
        return this->container.reserve(count);
    }

    std::pair< iterator, bool > insert_slow(const value_type &value) {
        auto it = std::find_if(this->container.begin(), this->container.end(),
                               [&](const std::pair< Key, Value > &x)->bool {
            return x.first == value.first;
        });
        if (it == this->container.end()) {
            this->container.push_back(value);
            auto it2 = this->container.end() - 1;
            return std::make_pair(Iterator(it2), true);
        } else {
            return std::make_pair(Iterator(it), false);
        }
    }

    std::pair< iterator, bool > insert(const value_type &value) {
        this->container.push_back(value);
        auto it2 = this->container.end() - 1;
        return std::make_pair(Iterator(it2), true);
    }

    iterator find(const Key &key) {
        for (auto it = this->container.begin(); it != this->container.end(); it++) {
            if (it->first == key) {
                return Iterator(it);
            }
        }
        return this->end();
    }

    const_iterator find(const Key &key) const {
        for (auto it = this->container.begin(); it != this->container.end(); it++) {
            if (it->first == key) {
                return ConstIterator(it);
            }
        }
        return this->end();
    }

    Value &at(const Key &key) {
        auto it = this->find(key);
        if (it == this->end()) {
            throw std::out_of_range("does not exist");
        } else {
            return it->second;
        }
    }

    const Value &at(const Key &key) const {
        auto it = this->find(key);
        if (it == this->end()) {
            throw std::out_of_range("does not exist");
        } else {
            return it->second;
        }
    }

    Value &operator[](const Key &key) {
        auto it = this->find(key);
        if (it == this->end()) {
            this->container.emplace_back();
            return this->container.back().second;
        } else {
            return it->second;
        }
    }

    void clear() {
        this->container.clear();
    }

    bool empty() const {
        return this->container.empty();
    }

    iterator begin() noexcept {
        return Iterator(this->container.begin());
    }

    const_iterator begin() const noexcept {
        return ConstIterator(this->container.begin());
    }

    const_iterator cbegin() const noexcept {
        return ConstIterator(this->container.cbegin());
    }

    iterator end() noexcept {
        return Iterator(this->container.end());
    }

    const_iterator end() const noexcept {
        return ConstIterator(this->container.end());
    }

    const_iterator cend() const noexcept {
        return ConstIterator(this->container.cend());
    }

private:
    std::vector< value_type > container;

public:
    class Iterator {
        friend class VectorMap< Key, Value >::ConstIterator;
    public:
        Iterator() : it() {
        }

        Iterator(typename std::vector< typename VectorMap< Key, Value >::value_type >::iterator it) : it(it) {
        }

        bool operator==(const Iterator &x) const {
            return this->it == x.it;
        }

        bool operator!=(const Iterator &x) const {
            return this->it != x.it;
        }

        Iterator &operator=(const Iterator &x) {
            this->it = x.it;
            return *this;
        }

        Iterator &operator++() {
            ++this->it;
            return *this;
        }

        Iterator operator++(int) {
            auto x = *this;
            this->it++;
            return x;
        }

        typename VectorMap< Key, Value >::value_type &operator*() const {
            return *this->it;
        }

        typename VectorMap< Key, Value >::value_type *operator->() const {
            return &(this->operator*());
        }

    private:
        typename std::vector< typename VectorMap< Key, Value >::value_type >::iterator it;
    };

    class ConstIterator {
    public:
        ConstIterator() : it() {
        }

        ConstIterator(typename std::vector< typename VectorMap< Key, Value >::value_type >::const_iterator it) : it(it) {
        }

        ConstIterator(const typename VectorMap< Key, Value >::Iterator &it) : it(it.it) {
        }

        bool operator==(const ConstIterator &x) const {
            return this->it == x.it;
        }

        bool operator!=(const ConstIterator &x) const {
            return this->it != x.it;
        }

        ConstIterator &operator++() {
            ++this->it;
            return *this;
        }

        ConstIterator operator++(int) {
            auto x = *this;
            this->it++;
            return x;
        }

        ConstIterator &operator=(const ConstIterator &x) {
            this->it = x.it;
            return *this;
        }

        const typename VectorMap< Key, Value >::value_type &operator*() const {
            return *this->it;
        }

        const typename VectorMap< Key, Value >::value_type *operator->() const {
            return &(this->operator*());
        }

    private:
        typename std::vector< typename VectorMap< Key, Value >::value_type >::const_iterator it;
    };
};
