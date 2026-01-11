#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <queue>
#include <string>

enum class ThreadState {
    CREATED,
    RUNNING,
    WAITING,
    TERMINATED
};

struct ThreadInfo {
    std::thread::id threadId;
    std::string name;
    ThreadState state;
    std::unique_ptr<std::thread> thread;
    
    ThreadInfo() : state(ThreadState::CREATED) {}
};

class ThreadManager {
public:
    ThreadManager();
    ~ThreadManager();
    
    // Disable copy constructor and assignment operator
    ThreadManager(const ThreadManager&) = delete;
    ThreadManager& operator=(const ThreadManager&) = delete;
    
    // Thread creation and management
    size_t createThread(const std::string& name, std::function<void()> task);
    bool joinThread(size_t threadIndex);
    bool detachThread(size_t threadIndex);
    bool terminateThread(size_t threadIndex);
    
    // Thread pool operations
    void initializeThreadPool(size_t poolSize);
    void submitTask(std::function<void()> task);
    void shutdownThreadPool();
    
    // Thread information
    size_t getActiveThreadCount() const;
    size_t getTotalThreadCount() const;
    ThreadState getThreadState(size_t threadIndex) const;
    std::string getThreadName(size_t threadIndex) const;
    
    // Synchronization primitives
    void lock();
    void unlock();
    void wait();
    void notify();
    void notifyAll();
    
    // Cleanup
    void cleanup();
    void joinAll();
    
private:
    mutable std::mutex managerMutex_;
    std::vector<std::unique_ptr<ThreadInfo>> threads_;
    std::atomic<size_t> threadCounter_;
    
    // Thread pool
    std::vector<std::thread> poolThreads_;
    std::queue<std::function<void()>> taskQueue_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stopPool_;
    std::atomic<size_t> activeTasks_;
    
    // Synchronization
    std::mutex syncMutex_;
    std::condition_variable syncCondition_;
    
    // Worker function for thread pool
    void workerFunction();
};

#endif // THREAD_MANAGER_H
