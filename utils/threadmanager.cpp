#include "threadmanager.h"

#include <iostream>

#include "utils/utils.h"

using namespace std;
using namespace std::chrono;
using namespace boost::coroutines;

Coroutine number_generator(int x, int z=-1) {
    return Coroutine([x,z](Yield &yield) {
        for (int i = 0; i != z; i++) {
            acout() << x << " " << i << endl;
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

    CoroutineThreadManager th(2);
    CoroutineThreadManager th2(2);
    th.add_coroutine(number_generator(0, 2));
    th.add_coroutine(number_generator(1));
    this_thread::sleep_for(5s);
    th.add_coroutine(number_generator(2));
    th2.add_coroutine(number_generator(3));
    th2.add_coroutine(number_generator(4));
    this_thread::sleep_for(5s);
    th.add_coroutine(number_generator(10));
    this_thread::sleep_for(5s);

    return 0;
}
static_block {
    register_main_function("thread_test", thread_test_main);
}

CoroutineThreadManager::CoroutineThreadManager(size_t thread_num) : running(true) {
    for (size_t i = 0; i < thread_num; i++) {
        this->threads.emplace_back([this]() { this->thread_fn(); });
    }
}

CoroutineThreadManager::~CoroutineThreadManager() {
    this->stop();
}

void CoroutineThreadManager::add_coroutine(Coroutine &&coro) {
    CoroutineRuntimeData coro_rd;
    coro_rd.coroutine = std::move(coro);
    this->enqueue_coroutine(coro_rd);
}

void CoroutineThreadManager::stop() {
    this->running = false;
    this->can_go.notify_all();
    this->join();
}

void CoroutineThreadManager::join() {
    for (auto &t : this->threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void CoroutineThreadManager::thread_fn() {
    while (this->running) {
        CoroutineRuntimeData tmp;
        if (dequeue_coroutine(tmp)) {
            bool reenqueue = true;
            tmp.budget += BUDGET_QUANTUM;
            while (tmp.budget > std::chrono::seconds(0)) {
                auto start_time = std::chrono::steady_clock::now();
                reenqueue = tmp.coroutine.run();
                auto stop_time = std::chrono::steady_clock::now();
                auto running_time = stop_time - start_time;
                tmp.running_time += running_time;
                tmp.budget -= running_time;
            }
            if (reenqueue) {
                this->enqueue_coroutine(tmp);
            }
        }
    }
}

void CoroutineThreadManager::enqueue_coroutine(CoroutineThreadManager::CoroutineRuntimeData &coro) {
    std::unique_lock< std::mutex > lock(this->obj_mutex);
    this->coros.emplace_back();
    std::swap(coro, this->coros.back());
    this->can_go.notify_one();
}

bool CoroutineThreadManager::dequeue_coroutine(CoroutineThreadManager::CoroutineRuntimeData &coro) {
    std::unique_lock< std::mutex > lock(this->obj_mutex);
    while (this->coros.empty()) {
        this->can_go.wait(lock);
        if (!this->running) {
            return false;
        }
    }
    if (!this->running) {
        return false;
    }
    std::swap(coro, this->coros.front());
    this->coros.pop_front();
    return true;
}

Coroutine::Coroutine() : coro_impl() {}

Coroutine::Coroutine(CoroutineBody &&body) : coro_impl(make_coroutine(move(body))) {}

Coroutine::Coroutine(Coroutine &&other) : coro_impl(std::move(other.coro_impl)) {}

void Coroutine::operator=(Coroutine &&other) {
    this->coro_impl.swap(other.coro_impl);
}

bool Coroutine::run() {
    if (this->coro_impl) {
        this->coro_impl();
        return true;
    } else {
        return false;
    }
}

asymmetric_coroutine< void >::pull_type Coroutine::make_coroutine(CoroutineBody &&body) {
    return boost::coroutines::asymmetric_coroutine< void >::pull_type([body](boost::coroutines::asymmetric_coroutine< void >::push_type &yield_impl) {
        Yield yield(yield_impl);
        yield();
        body(yield);
    });
}

Yield::Yield(asymmetric_coroutine< void >::push_type &base_yield) : yield_impl(base_yield) {
}
