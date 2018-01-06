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

template< typename T, typename U >
class BackreferenceRegistry;

template< typename T, typename U >
class BackreferenceToken {
    friend class BackreferenceRegistry< T, U >;
public:
    size_t get_id() const {
        return this->id;
    }

    std::weak_ptr< U > get_main() const {
        auto strong_reg = this->registry.lock();
        if (!strong_reg) {
            return {};
        } else {
            return strong_reg->get_main();
        }
    }

    BackreferenceToken() = delete;
    BackreferenceToken(BackreferenceToken&) = delete;
    BackreferenceToken(BackreferenceToken&&) = default;

    ~BackreferenceToken() {
        std::shared_ptr< BackreferenceRegistry< T, U > > strong_reg = this->registry.lock();
        if (strong_reg) {
            strong_reg->ref_disappearing(this->id);
        }
    }

private:
    BackreferenceToken(std::weak_ptr< BackreferenceRegistry< T, U > > registry, size_t id) : registry(registry), id(id) {
    }
    std::weak_ptr< BackreferenceRegistry< T, U > > registry;
    size_t id;
};

template< typename T, typename U >
class BackreferenceRegistry {
    friend class BackreferenceToken< T, U >;
public:
    // Emulate the behaviour of a map
    typedef size_t key_type;
    typedef std::shared_ptr< T > mapped_type;

    template< typename... Args >
    std::shared_ptr< T > make_instance(const Args&... args) {
        assert(!this->weak_this.expired());
        size_t id = this->id_dist.get_id();
        auto token = BackreferenceToken< T, U >(this->weak_this, id);
        auto pointer = T::create(std::move(token), args...);
        std::unique_lock< std::mutex > lock(this->refs_mutex);
        this->references[id] = pointer;
        return pointer;
    }

    std::shared_ptr< T > at(size_t id) {
        std::unique_lock< std::mutex > lock(this->refs_mutex);
        return this->references.at(id);
    }

    template< typename... Args >
    static std::shared_ptr< BackreferenceRegistry< T, U > > create(const Args&... args) {
        auto pointer = std::make_shared< enable_make< BackreferenceRegistry< T, U > > >(args...);
        pointer->weak_this = pointer;
        return pointer;
    }

    std::weak_ptr< U > get_main() const {
        return this->main;
    }

    void set_main(std::weak_ptr< U > main) {
        this->main = main;
    }

protected:
    BackreferenceRegistry() {}
    BackreferenceRegistry(std::weak_ptr< U > main) : main(main) {}

private:
    void ref_disappearing(size_t id) {
        std::unique_lock< std::mutex > lock(this->refs_mutex);
        this->references.erase(id);
    }

    IdDistributor id_dist;
    std::mutex refs_mutex;
    std::unordered_map< size_t, std::shared_ptr< T > > references;
    std::weak_ptr< BackreferenceRegistry< T, U > > weak_this;
    std::weak_ptr< U > main;
};

#endif // BACKREF_REGISTRY_H
