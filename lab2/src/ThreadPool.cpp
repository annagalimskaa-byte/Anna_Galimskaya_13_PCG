#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; i++) {
        workers.emplace_back([this] { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (auto& worker : workers) {
        if (worker.joinable()) worker.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.push(std::move(task));
        pendingTasks++;
    }
    condition.notify_one();
}

void ThreadPool::waitAll() {
    std::unique_lock<std::mutex> lock(queueMutex);
    finishedCondition.wait(lock, [this] {
        return tasks.empty() && activeTasks.load() == 0;
    });
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] {
                return stop || !tasks.empty();
            });

            if (stop && tasks.empty()) return;

            task = std::move(tasks.front());
            tasks.pop();
            activeTasks++;
        }

        task();

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            activeTasks--;
        }

        finishedCondition.notify_all();
    }
}