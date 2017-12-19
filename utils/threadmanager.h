#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <vector>
#include <list>

#define BOOST_COROUTINE_NO_DEPRECATION_WARNING
#define BOOST_COROUTINES_NO_DEPRECATION_WARNING
#include <boost/coroutine/all.hpp>

class Yield {
public:
    Yield(boost::coroutines::asymmetric_coroutine< void >::push_type &base_yield);
    void operator()() {
        this->yield_impl();
    }

private:
    boost::coroutines::asymmetric_coroutine< void >::push_type &yield_impl;
};

typedef std::function< void(Yield&) > CoroutineBody;

class Coroutine {
public:
    Coroutine();
    Coroutine(CoroutineBody &&body);
    Coroutine(Coroutine &&other);
    void operator=(Coroutine &&other);
    bool run();

private:
    static boost::coroutines::asymmetric_coroutine< void >::pull_type make_coroutine(CoroutineBody &&body);

    boost::coroutines::asymmetric_coroutine< void >::pull_type coro_impl;
};

class CoroutineThreadManager {
public:
    struct CoroutineRuntimeData {
        CoroutineRuntimeData() : coroutine(), running_time(0), budget(0) {}

        Coroutine coroutine;
        std::chrono::steady_clock::duration running_time;
        std::chrono::steady_clock::duration budget;
    };

    const std::chrono::steady_clock::duration BUDGET_QUANTUM = std::chrono::milliseconds(100);

    CoroutineThreadManager(size_t thread_num);
    ~CoroutineThreadManager();
    void add_coroutine(Coroutine &&coro);
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

#endif // THREADMANAGER_H
