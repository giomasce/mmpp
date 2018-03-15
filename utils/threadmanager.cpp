#include "threadmanager.h"

#include <iostream>

#include "utils/utils.h"
#include "platform.h"

std::vector< std::shared_ptr< Coroutine > > coroutines;

std::shared_ptr< Coroutine > number_generator(int x, int z=-1) {
    auto body = [x,z](Yielder &yield) {
        for (int i = 0; i != z; i++) {
            acout() << x << " " << i << std::endl;
            yield();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    };
    auto shared_body = std::make_shared< decltype(body) >(body);
    auto coro = std::make_shared< Coroutine >(shared_body);
    coroutines.push_back(coro);
    return coro;
}

class Test {
public:
    void operator()(Yielder &yield) {
        yield();
    }
};

int thread_test_main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    Test test;
    Coroutine coro(std::make_shared< decltype(test) >(test));

    CoroutineThreadManager th(2);
    CoroutineThreadManager th2(2);
    th.add_coroutine(number_generator(0, 2));
    th.add_coroutine(number_generator(1));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    th.add_coroutine(number_generator(2));
    th2.add_coroutine(number_generator(3));
    th2.add_coroutine(number_generator(4));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    th.add_coroutine(number_generator(10));
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
static_block {
    register_main_function("thread_test", thread_test_main);
}

CoroutineThreadManager::CoroutineThreadManager(size_t thread_num) : running(true) {
    for (size_t i = 0; i < thread_num; i++) {
        this->threads.emplace_back([this]() { this->thread_fn(); });
        set_thread_name(this->threads.back(), std::string("CTM-") + std::to_string(i));
    }
}

CoroutineThreadManager::~CoroutineThreadManager() {
    this->stop();
}

void CoroutineThreadManager::add_coroutine(std::weak_ptr<Coroutine> coro) {
    CoroutineRuntimeData coro_rd;
    coro_rd.coroutine = coro;
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
    set_current_thread_low_priority();
    while (this->running) {
        CoroutineRuntimeData tmp;
        if (dequeue_coroutine(tmp)) {
            bool reenqueue = true;
            auto strong_coro = tmp.coroutine.lock();
            if (strong_coro == nullptr) {
                continue;
            }
            tmp.budget += BUDGET_QUANTUM;
            while (tmp.budget > std::chrono::seconds(0)) {
                auto start_time = std::chrono::steady_clock::now();
                reenqueue = strong_coro->execute();
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

/*Coroutine::Coroutine(Coroutine &&other) : coro_impl(std::move(other.coro_impl)) {}

void Coroutine::operator=(Coroutine &&other) {
    this->coro_impl.swap(other.coro_impl);
}*/

bool Coroutine::execute() {
    if (this->coro_impl) {
        this->coro_impl();
        return true;
    } else {
        return false;
    }
}

Yielder::Yielder(boost::coroutines::asymmetric_coroutine< void >::push_type &base_yield) : yield_impl(base_yield) {
}
