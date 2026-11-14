// core/engine/thread_pool.cpp
#include "thread_pool.hpp"
#include <algorithm>

namespace NexusForge::Core {

ThreadPool::ThreadPool() = default;

ThreadPool::~ThreadPool() {
    shutdown();
}

bool ThreadPool::initialize(int numThreads) {
    if (!workers_.empty()) {
        shutdown();
    }

    if (numThreads <= 0) {
        numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stop_ = false;
        paused_ = false;
        activeTasks_ = 0;
    }

    workers_.reserve(numThreads);
    workerInfo_.resize(numThreads);

    for (int i = 0; i < numThreads; ++i) {
        workers_.emplace_back(&ThreadPool::workerFunction, this, i);
        workerInfo_[i].id = workers_.back().get_id();
    }

    return true;
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stop_ = true;
        paused_ = false;
    }

    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
    workerInfo_.clear();
}

void ThreadPool::workerFunction(size_t workerIndex) {
    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            condition_.wait(lock, [this]() {
                return stop_ || (!paused_ && !taskQueue_.empty());
            });

            if (stop_ && taskQueue_.empty()) {
                return;
            }

            if (taskQueue_.empty()) {
                continue;
            }

            task = std::move(const_cast<Task&>(taskQueue_.top()));
            taskQueue_.pop();
            // Increment activeTasks_ while holding queueMutex_ so waitForAll() never sees a false idle window
            activeTasks_++;
        }

        workerInfo_[workerIndex].busy = true;
        workerInfo_[workerIndex].currentTask = task.name;

        try {
            if (task.function) {
                task.function();
            }
        } catch (const std::exception&) {
            // Log exception
        } catch (...) {
            // Catch-all so worker threads never terminate unexpectedly
        }

        workerInfo_[workerIndex].currentTask.clear();
        workerInfo_[workerIndex].busy = false;
        workerInfo_[workerIndex].tasksCompleted++;

        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            activeTasks_--;
        }
        completionCondition_.notify_all();
    }
}

void ThreadPool::parallel_for(size_t start, size_t end,
                               std::function<void(size_t)> func,
                               size_t chunkSize) {
    if (start >= end || !func) return;

    size_t total = end - start;
    size_t numThreads = std::max(size_t(1), workers_.size());

    if (chunkSize == 0) {
        chunkSize = std::max(size_t(1), (total + numThreads - 1) / numThreads);
    }

    if (workers_.empty()) {
        for (size_t j = start; j < end; ++j) {
            func(j);
        }
        return;
    }

    std::vector<std::future<void>> futures;

    for (size_t i = start; i < end; i += chunkSize) {
        size_t chunkEnd = std::min(i + chunkSize, end);

        futures.push_back(submit([func, i, chunkEnd]() {
            for (size_t j = i; j < chunkEnd; ++j) {
                func(j);
            }
        }));
    }

    for (auto& future : futures) {
        future.wait();
    }
}

void ThreadPool::pause() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    paused_ = true;
}

void ThreadPool::resume() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        paused_ = false;
    }
    condition_.notify_all();
}

void ThreadPool::waitForAll() {
    std::unique_lock<std::mutex> lock(queueMutex_);
    completionCondition_.wait(lock, [this]() {
        return taskQueue_.empty() && activeTasks_ == 0;
    });
}

int ThreadPool::getActiveThreadCount() const {
    int count = 0;
    for (const auto& info : workerInfo_) {
        if (info.busy) count++;
    }
    return count;
}

size_t ThreadPool::getQueuedTaskCount() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return taskQueue_.size();
}

} // namespace NexusForge::Core
