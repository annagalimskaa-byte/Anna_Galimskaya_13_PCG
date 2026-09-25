#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    void enqueue(std::function<void()> task);

    void waitAll();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable finishedCondition;

    std::atomic<int> activeTasks{0};
    std::atomic<int> pendingTasks{0};
    bool stop = false;

    void workerLoop();
};