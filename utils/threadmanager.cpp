#include "threadmanager.h"

#include <iostream>
#include <thread>

#include "utils/utils.h"

#define BOOST_COROUTINE_NO_DEPRECATION_WARNING
#define BOOST_COROUTINES_NO_DEPRECATION_WARNING
#include <boost/coroutine/all.hpp>

using namespace std;
using namespace boost::coroutines;

class Yield {
public:
    Yield(asymmetric_coroutine< void >::push_type &base_yield) : yield_impl(base_yield) {}
    void operator()() {
        this->yield_impl();
    }
private:
    asymmetric_coroutine< void >::push_type &yield_impl;
};

typedef function< void(Yield&) > CoroutineBody;

class Coroutine {
public:
    Coroutine() : coro_impl() {}
    Coroutine(CoroutineBody &&body) : coro_impl(make_coroutine(move(body))) {}
    void run() {
        this->coro_impl();
    }

    void swap(Coroutine &other) {
        this->coro_impl.swap(other.coro_impl);
    }

private:
    static asymmetric_coroutine< void >::pull_type make_coroutine(CoroutineBody &&body) {
        return asymmetric_coroutine< void >::pull_type([body](asymmetric_coroutine< void >::push_type &yield_impl) {
            Yield yield(yield_impl);
            yield();
            body(yield);
        });
    }

    asymmetric_coroutine< void >::pull_type coro_impl;
};

class CoroutineThread {
public:
    CoroutineThread() : running(true), thread_impl([this]() { this->thread_fn(); }) {}
    ~CoroutineThread() {
        this->stop();
    }

    void add_coroutine(Coroutine &&coro) {
        unique_lock< mutex > lock(this->obj_mutex);
        this->coros.push_back(move(coro));
        this->can_go.notify_one();
    }

    void stop() {
        this->running = false;
        this->can_go.notify_one();
        if (this->thread_impl.joinable()) {
            this->thread_impl.join();
        }
    }

private:
    void thread_fn() {
        while (true) {
            Coroutine tmp;
            {
                unique_lock< mutex > lock(this->obj_mutex);
                while (this->coros.empty()) {
                    this->can_go.wait(lock);
                    if (!this->running) {
                        return;
                    }
                }
                if (!this->running) {
                    return;
                }
                tmp.swap(this->coros.front());
                this->coros.pop_front();
            }
            tmp.run();
            {
                unique_lock< mutex > lock(this->obj_mutex);
                this->coros.emplace_back();
                tmp.swap(this->coros.back());
            }
        }
    }

    atomic< bool > running;
    mutex obj_mutex;
    condition_variable can_go;
    list< Coroutine > coros;

    // The thread must be the last one, otherwise it starts before the other members are initialized
    thread thread_impl;
};

Coroutine number_generator(int x) {
    return Coroutine([x](Yield &yield) {
        int i = 0;
        while (true) {
            cout << x << " " << i << endl;
            i++;
            yield();
            this_thread::sleep_for(1s);
        }
    });
}

void thread_run() {
    Coroutine gen = number_generator(22);
    cout << "Beginning..." << endl;
    while (true) {
        gen.run();
    }
}

int thread_test_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    CoroutineThread th;
    th.add_coroutine(number_generator(0));
    th.add_coroutine(number_generator(1));
    this_thread::sleep_for(5s);
    th.add_coroutine(number_generator(2));
    this_thread::sleep_for(5s);

    return 0;
}
static_block {
    register_main_function("thread_test", thread_test_main);
}

ThreadManager::ThreadManager()
{
}
