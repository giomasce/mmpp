#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <vector>
#include <list>
#include <atomic>
#include <queue>

#include <giolib/exception.h>

#include "platform.h"
#include "utils.h"

class Yielder {
public:
    Yielder(coroutine_push< void > &base_yield);
    void operator()() {
        this->yield_impl();
    }

private:
    coroutine_push< void > &yield_impl;
};

class Coroutine {
public:
    Coroutine() : coro_impl() {}
    template< typename T >
    Coroutine(std::shared_ptr< T > body) : coro_impl(make_coroutine(body)) {}
    bool execute();
    template< typename T >
    void set_body(std::shared_ptr< T > body) {
        this->coro_impl = make_coroutine(body);
    }

private:

    template< typename T >
    static decltype(auto) make_coroutine(std::shared_ptr< T > body) {
        return std::make_unique< coroutine_pull< void > >([body](coroutine_push< void > &yield_impl) mutable {
            Yielder yield(yield_impl);
            yield();
            try {
                (*body)(yield);
            } catch (const coroutine_forced_unwind&) {
                // Rethow internal coroutine exceptions, as per their specifications
                throw;
            } catch (...) {
                std::cerr << "Coroutine filed with exception" << std::endl;
                gio::default_exception_handler(std::current_exception());
            }
        });
    }

    std::unique_ptr< coroutine_pull< void > > coro_impl;
};

/*
 * An auto coroutine has a shared pointer to itself, so it is not automatically
 * canceled when the creator drops its shared pointer.
 */
template< typename T >
std::shared_ptr< Coroutine > make_auto_coroutine(std::shared_ptr< T > body) {
    auto coro = std::make_shared< Coroutine >();
    auto new_body = [coro,body](Yielder &yield) mutable {
        Finally f([&coro]() {
           coro = nullptr;
        });
        (*body)(yield);
    };
    auto shared_new_body = std::make_shared< decltype(new_body) >(new_body);
    coro->set_body(shared_new_body);
    return coro;
}

struct CTMComp;

class CoroutineThreadManager {
public:
    struct CoroutineRuntimeData {
        CoroutineRuntimeData() : coroutine(), running_time(0), budget(0) {}

        std::weak_ptr< Coroutine > coroutine;
        std::chrono::steady_clock::duration running_time;
        std::chrono::steady_clock::duration budget;
    };

    struct CTMComp {
        bool operator()(const std::pair< std::chrono::system_clock::time_point, CoroutineThreadManager::CoroutineRuntimeData > &x, const std::pair< std::chrono::system_clock::time_point, CoroutineThreadManager::CoroutineRuntimeData > &y) const;
    };

    const std::chrono::steady_clock::duration BUDGET_QUANTUM = std::chrono::milliseconds(100);

    CoroutineThreadManager(size_t thread_num);
    ~CoroutineThreadManager();
    void add_coroutine(std::weak_ptr<Coroutine> coro);
    void add_timed_coroutine(std::weak_ptr<Coroutine> coro, std::chrono::system_clock::duration wait_time);
    void stop();
    void join();
    std::tuple<unsigned, size_t, size_t> get_stats();

private:
    void thread_fn();
    void timed_fn();
    void enqueue_coroutine(CoroutineRuntimeData &&coro, bool reenqueueing);
    bool dequeue_coroutine(CoroutineRuntimeData &coro);

    std::atomic< bool > running;
    std::mutex obj_mutex;
    std::condition_variable can_go;
    std::list< CoroutineRuntimeData > coros;
    std::atomic< unsigned > running_coros;
    std::vector< std::thread > threads;

    std::mutex timed_mutex;
    std::condition_variable timed_cond;
    std::priority_queue< std::pair< std::chrono::system_clock::time_point, CoroutineRuntimeData >, std::vector< std::pair< std::chrono::system_clock::time_point, CoroutineRuntimeData > >, CTMComp > timed_coros;
    std::unique_ptr< std::thread > timed_thread;
};
