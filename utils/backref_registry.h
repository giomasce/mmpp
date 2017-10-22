#ifndef BACKREF_REGISTRY_H
#define BACKREF_REGISTRY_H

#include <memory>
#include <unordered_map>
#include <atomic>
#include <mutex>

#include <cassert>

class IdDistributor {
public:
    IdDistributor(size_t first_id = 0) : last_id(first_id) {
    }

    size_t get_id() {
        size_t ret = this->last_id++;
        return ret;
    }

private:
    std::atomic< size_t > last_id;
};

template< typename T >
class BackreferenceRegistry;

template< typename T >
class BackreferenceToken {
    friend class BackreferenceRegistry< T >;
public:
    size_t get_id() const {
        return this->id;
    }

    ~BackreferenceToken() {
        std::shared_ptr< BackreferenceRegistry< T > > strong_reg = this->registry.lock();
        if (strong_reg != NULL) {
            strong_reg->ref_disappearing(this->id);
        }
    }

private:
    BackreferenceToken(std::weak_ptr< BackreferenceRegistry< T > > registry, size_t id) : registry(registry), id(id) {
    }
    std::weak_ptr< BackreferenceRegistry< T > > registry;
    size_t id;
};

template< typename T >
class BackreferenceRegistry {
    friend class BackreferenceToken< T >;
public:
    template< typename... Args >
    std::shared_ptr< T > make_instance(const Args&... args) {
        assert(!this->weak_this.expired());
        size_t id = this->id_dist.get_id();
        auto token = BackreferenceToken< T >(this->weak_this, id);
        auto pointer = T::create(token, args...);
        std::unique_lock< std::mutex > lock(this->refs_mutex);
        this->references[id] = pointer;
        return pointer;
    }

    std::shared_ptr< T > at(size_t id) {
        std::unique_lock< std::mutex > lock(this->refs_mutex);
        return this->references.at(id);
    }

    static std::shared_ptr< BackreferenceRegistry< T > > create() {
        auto pointer = std::make_shared< enable_make< BackreferenceRegistry< T > > >();
        pointer->weak_this = pointer;
        return pointer;
    }

protected:
    BackreferenceRegistry() = default;

private:
    void ref_disappearing(size_t id) {
        std::unique_lock< std::mutex > lock(this->refs_mutex);
        this->references.erase(id);
    }

    IdDistributor id_dist;
    std::mutex refs_mutex;
    std::unordered_map< size_t, std::shared_ptr< T > > references;
    std::weak_ptr< BackreferenceRegistry< T > > weak_this;
};

#endif // BACKREF_REGISTRY_H
