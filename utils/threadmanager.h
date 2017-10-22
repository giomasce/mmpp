#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>

class ThreadManager
{
public:
    ThreadManager();
private:
    std::vector< std::thread > threads;
};

#endif // THREADMANAGER_H
