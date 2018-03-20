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
        this->threads.emplace_back([this,i]() {
            set_thread_name(std::string("CTM-") + std::to_string(i));
            this->thread_fn();
        });
    }
    this->timed_thread = std::make_unique< std::thread >([this]() {
        set_thread_name("CTM-timed");
        this->timed_fn();
    });
}

CoroutineThreadManager::~CoroutineThreadManager() {
    this->stop();
}

void CoroutineThreadManager::add_coroutine(std::weak_ptr<Coroutine> coro) {
    CoroutineRuntimeData coro_rd;
    coro_rd.coroutine = coro;
    this->enqueue_coroutine(std::move(coro_rd));
}

void CoroutineThreadManager::add_timed_coroutine(std::weak_ptr<Coroutine> coro, std::chrono::system_clock::duration wait_time)
{
    CoroutineRuntimeData coro_rd;
    coro_rd.coroutine = coro;
    std::unique_lock< std::mutex > lock(this->timed_mutex);
    this->timed_coros.push(std::make_pair(std::chrono::system_clock::now() + wait_time, coro_rd));
    this->timed_cond.notify_one();
}

void CoroutineThreadManager::stop() {
    this->running = false;
    this->can_go.notify_all();
    this->timed_cond.notify_all();
    this->join();
}

void CoroutineThreadManager::join() {
    for (auto &t : this->threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    if (this->timed_thread->joinable()) {
        this->timed_thread->join();
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
                this->enqueue_coroutine(std::move(tmp));
            }
        }
    }
}

void CoroutineThreadManager::timed_fn()
{
    std::unique_lock< std::mutex > lock(this->timed_mutex);
    while (this->running) {
        while (this->timed_coros.empty()) {
            this->timed_cond.wait(lock);
            if (!this->running) {
                return;
            }
        }
        auto now = std::chrono::system_clock::now();
        auto &top = this->timed_coros.top();
        if (top.first <= now) {
            auto data = top.second;
            this->enqueue_coroutine(std::move(data));
            this->timed_coros.pop();
        } else {
            this->timed_cond.wait_until(lock, top.first);
            if (!this->running) {
                return;
            }
        }
    }
}

void CoroutineThreadManager::enqueue_coroutine(CoroutineThreadManager::CoroutineRuntimeData &&coro) {
    std::unique_lock< std::mutex > lock(this->obj_mutex);
    this->coros.push_back(std::move(coro));
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

bool CoroutineThreadManager::CTMComp::operator()(const std::pair<std::chrono::system_clock::time_point, CoroutineThreadManager::CoroutineRuntimeData> &x, const std::pair<std::chrono::system_clock::time_point, CoroutineThreadManager::CoroutineRuntimeData> &y) const {
    return x.first > y.first;
}
