#ifndef STEP_H
#define STEP_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <mutex>

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
class BackreferenceRegistryInner;

template< typename T >
class BackreferenceToken {
    friend class BackreferenceRegistry< T >;
    friend class BackreferenceRegistryInner< T >;
public:
    size_t get_id() const {
        return this->id;
    }

    ~BackreferenceToken() {
    }

private:
    BackreferenceToken(std::weak_ptr< BackreferenceRegistryInner< T > > registry_inner, size_t id) : registry_inner(registry_inner), id(id) {
    }
    std::weak_ptr< BackreferenceRegistryInner< T > > registry_inner;
    size_t id;
};

template< typename T >
class BackreferenceRegistryInner {
    friend class BackreferenceToken< T >;
    friend class BackreferenceRegistry< T >;
private:
    BackreferenceRegistryInner(BackreferenceRegistry< T > &registry) : registry(registry) {
    }

    // Funny trick from https://stackoverflow.com/a/20961251/807307
    struct MakeSharedEnabler;
    template< typename... Args >
    static std::shared_ptr< BackreferenceRegistryInner< T > > create(Args&&... args) {
        return std::make_shared< MakeSharedEnabler >(std::forward< Args >(args)...);
    }

    BackreferenceRegistry< T > &registry;
};

// Still part of the above funny trick
template< typename T >
struct BackreferenceRegistryInner< T >::MakeSharedEnabler : public BackreferenceRegistryInner< T > {
    template< typename... Args >
    MakeSharedEnabler(Args&&... args) : BackreferenceRegistryInner< T >(std::forward< Args >(args)...) {
    }
};

template< typename T >
class BackreferenceRegistry {
public:
    BackreferenceRegistry() : inner(BackreferenceRegistryInner< T >::create(*this)) {
    }

    template< typename... Args >
    std::shared_ptr< T > make_instance(const Args&... args) {
        size_t id = this->id_dist.get_id();
        auto token = BackreferenceToken< T >(this->inner, id);
        auto pointer = std::make_shared< T >(token, args...);
        std::unique_lock< std::mutex > lock(this->refs_mutex);
        this->references[id] = pointer;
        return pointer;
    }

private:
    IdDistributor id_dist;
    std::mutex refs_mutex;
    std::unordered_map< size_t, std::shared_ptr< T > > references;
    std::shared_ptr< BackreferenceRegistryInner< T > > inner;
};

class Step
{
public:
    Step(BackreferenceToken< Step > token);
    size_t get_id() const;

private:

    BackreferenceToken< Step > token;
    std::vector< std::shared_ptr< Step > > children;

};

#endif // STEP_H
