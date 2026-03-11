// core/engine/thread_pool.hpp
#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <string>

namespace NexusForge::Core {

// Task priority levels
enum class TaskPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

// Task wrapper
struct Task {
    std::function<void()> function;
    TaskPriority priority;
    std::string name;

    bool operator<(const Task& other) const {
        return priority < other.priority;
    }
};

// Worker thread info
struct WorkerInfo {
    std::thread::id id;
    std::string currentTask;
    std::atomic<bool> busy{false};
    uint64_t tasksCompleted{0};
};

class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();

    bool initialize(int numThreads = 0);
    void shutdown();

    // Submit tasks
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    template<typename F, typename... Args>
    auto submitWithPriority(TaskPriority priority, const std::string& name,
                            F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    // Async patterns
    void parallel_for(size_t start, size_t end,
                      std::function<void(size_t)> func, size_t chunkSize = 0);

    template<typename Iterator, typename Func>
    void parallel_foreach(Iterator begin, Iterator end, Func func);

    // Control
    void pause();
    void resume();
    void waitForAll();

    // Statistics
    int getThreadCount() const { return static_cast<int>(workers_.size()); }
    int getActiveThreadCount() const;
    size_t getQueuedTaskCount() const;
    const std::vector<WorkerInfo>& getWorkerInfo() const { return workerInfo_; }

private:
    std::vector<std::thread> workers_;
    std::vector<WorkerInfo> workerInfo_;
    std::priority_queue<Task> taskQueue_;

    mutable std::mutex queueMutex_;
    std::condition_variable condition_;
    std::condition_variable completionCondition_;

    std::atomic<bool> stop_{false};
    std::atomic<bool> paused_{false};
    std::atomic<int> activeTasks_{0};

    void workerFunction(size_t workerIndex);
};

// Template implementations
template<typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {
    return submitWithPriority(TaskPriority::Normal, "",
                              std::forward<F>(f), std::forward<Args>(args)...);
}

template<typename F, typename... Args>
auto ThreadPool::submitWithPriority(TaskPriority priority, const std::string& name,
                                    F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {

    using ReturnType = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<ReturnType> result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(queueMutex_);

        if (stop_) {
            throw std::runtime_error("ThreadPool is stopped");
        }

        taskQueue_.push(Task{
            [task]() { (*task)(); },
            priority,
            name
        });
    }

    condition_.notify_one();
    return result;
}

template<typename Iterator, typename Func>
void ThreadPool::parallel_foreach(Iterator begin, Iterator end, Func func) {
    std::vector<std::future<void>> futures;

    for (auto it = begin; it != end; ++it) {
        futures.push_back(submit([func, it]() { func(*it); }));
    }

    for (auto& future : futures) {
        future.wait();
    }
}

} // namespace NexusForge::Core
