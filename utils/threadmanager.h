#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <vector>
#include <list>
#include <atomic>

#define BOOST_COROUTINE_NO_DEPRECATION_WARNING
#define BOOST_COROUTINES_NO_DEPRECATION_WARNING
#include <boost/coroutine/all.hpp>

#include "utils.h"

class Yielder {
public:
    Yielder(boost::coroutines::asymmetric_coroutine< void >::push_type &base_yield);
    void operator()() {
        this->yield_impl();
    }

private:
    boost::coroutines::asymmetric_coroutine< void >::push_type &yield_impl;
};

class Coroutine {
public:
    Coroutine() : coro_impl() {}
    //template< typename T >
    //Coroutine(T &&body) : coro_impl(make_coroutine(std::move(body))) {}
    template< typename T >
    Coroutine(std::shared_ptr< T > body) : coro_impl(make_coroutine(body)) {}
    //Coroutine(Coroutine &&other);
    //void operator=(Coroutine &&other);
    bool execute();
    template< typename T >
    void set_body(std::shared_ptr< T > body) {
        this->coro_impl = make_coroutine(body);
    }

private:
    /*template< typename T >
    static boost::coroutines::asymmetric_coroutine< void >::pull_type make_coroutine(T &&body) {
        return boost::coroutines::asymmetric_coroutine< void >::pull_type([body{std::move(body)}](boost::coroutines::asymmetric_coroutine< void >::push_type &yield_impl) mutable {
            Yield yield(yield_impl);
            yield();
            body(yield);
        });
    }*/

    template< typename T >
    static boost::coroutines::asymmetric_coroutine< void >::pull_type make_coroutine(std::shared_ptr< T > body) {
        return boost::coroutines::asymmetric_coroutine< void >::pull_type([body](boost::coroutines::asymmetric_coroutine< void >::push_type &yield_impl) mutable {
            Yielder yield(yield_impl);
            yield();
            (*body)(yield);
        });
    }

    boost::coroutines::asymmetric_coroutine< void >::pull_type coro_impl;
};

template< typename T >
std::shared_ptr< Coroutine > make_auto_coroutine(std::shared_ptr< T > body) {
    auto coro = std::make_shared< Coroutine >();
    auto new_body = [coro,body](Yielder &yield) mutable {
        Finally f([&coro]() {
           coro = NULL;
        });
        (*body)(yield);
    };
    auto shared_new_body = std::make_shared< decltype(new_body) >(new_body);
    coro->set_body(shared_new_body);
    return coro;
}

class CoroutineThreadManager {
public:
    struct CoroutineRuntimeData {
        CoroutineRuntimeData() : coroutine(), running_time(0), budget(0) {}

        std::weak_ptr< Coroutine > coroutine;
        std::chrono::steady_clock::duration running_time;
        std::chrono::steady_clock::duration budget;
    };

    const std::chrono::steady_clock::duration BUDGET_QUANTUM = std::chrono::milliseconds(100);

    CoroutineThreadManager(size_t thread_num);
    ~CoroutineThreadManager();
    void add_coroutine(std::weak_ptr<Coroutine> coro);
    void stop();
    void join();

private:
    void thread_fn();
    void enqueue_coroutine(CoroutineRuntimeData &coro);
    bool dequeue_coroutine(CoroutineRuntimeData &coro);

    std::atomic< bool > running;
    std::mutex obj_mutex;
    std::condition_variable can_go;
    std::list< CoroutineRuntimeData > coros;
    std::vector< std::thread > threads;
};
