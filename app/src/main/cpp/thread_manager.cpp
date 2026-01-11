#include "thread_manager.h"
#include <algorithm>
#include <chrono>

ThreadManager::ThreadManager() 
    : threadCounter_(0), stopPool_(false), activeTasks_(0) {
}

ThreadManager::~ThreadManager() {
    cleanup();
}

size_t ThreadManager::createThread(const std::string& name, std::function<void()> task) {
    std::lock_guard<std::mutex> lock(managerMutex_);
    
    auto threadInfo = std::make_unique<ThreadInfo>();
    threadInfo->name = name.empty() ? "Thread-" + std::to_string(threadCounter_) : name;
    threadInfo->state = ThreadState::CREATED;
    
    // Create thread
    threadInfo->thread = std::make_unique<std::thread>([this, task, threadInfo = threadInfo.get()]() {
        threadInfo->threadId = std::this_thread::get_id();
        threadInfo->state = ThreadState::RUNNING;
        
        try {
            task();
        } catch (...) {
            // Handle exceptions
        }
        
        threadInfo->state = ThreadState::TERMINATED;
    });
    
    threadInfo->threadId = threadInfo->thread->get_id();
    size_t index = threads_.size();
    threads_.push_back(std::move(threadInfo));
    threadCounter_++;
    
    return index;
}

bool ThreadManager::joinThread(size_t threadIndex) {
    std::lock_guard<std::mutex> lock(managerMutex_);
    
    if (threadIndex >= threads_.size() || !threads_[threadIndex]) {
        return false;
    }
    
    auto& threadInfo = threads_[threadIndex];
    if (threadInfo->thread && threadInfo->thread->joinable()) {
        threadInfo->thread->join();
        threadInfo->state = ThreadState::TERMINATED;
        return true;
    }
    
    return false;
}

bool ThreadManager::detachThread(size_t threadIndex) {
    std::lock_guard<std::mutex> lock(managerMutex_);
    
    if (threadIndex >= threads_.size() || !threads_[threadIndex]) {
        return false;
    }
    
    auto& threadInfo = threads_[threadIndex];
    if (threadInfo->thread && threadInfo->thread->joinable()) {
        threadInfo->thread->detach();
        return true;
    }
    
    return false;
}

bool ThreadManager::terminateThread(size_t threadIndex) {
    std::lock_guard<std::mutex> lock(managerMutex_);
    
    if (threadIndex >= threads_.size() || !threads_[threadIndex]) {
        return false;
    }
    
    auto& threadInfo = threads_[threadIndex];
    if (threadInfo->thread && threadInfo->thread->joinable()) {
        // Note: std::thread doesn't support forced termination
        // This is a limitation of C++11 threads
        // In practice, you would need to signal the thread to exit gracefully
        threadInfo->state = ThreadState::TERMINATED;
        return true;
    }
    
    return false;
}

void ThreadManager::initializeThreadPool(size_t poolSize) {
    // Shutdown existing pool first (if any) - this acquires its own lock
    if (!poolThreads_.empty()) {
        shutdownThreadPool();
    }
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stopPool_ = false;
        activeTasks_ = 0;
    }
    
    for (size_t i = 0; i < poolSize; ++i) {
        poolThreads_.emplace_back(&ThreadManager::workerFunction, this);
    }
}

void ThreadManager::workerFunction() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            condition_.wait(lock, [this] {
                return stopPool_ || !taskQueue_.empty();
            });
            
            if (stopPool_ && taskQueue_.empty()) {
                return;
            }
            
            task = std::move(taskQueue_.front());
            taskQueue_.pop();
            activeTasks_++;
        }
        
        try {
            task();
        } catch (...) {
            // Handle exceptions
        }
        
        activeTasks_--;
    }
}

void ThreadManager::submitTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (stopPool_) {
            return;
        }
        taskQueue_.push(task);
    }
    condition_.notify_one();
}

void ThreadManager::shutdownThreadPool() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stopPool_ = true;
    }
    
    condition_.notify_all();
    
    for (auto& thread : poolThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    poolThreads_.clear();
    
    // Clear remaining tasks
    std::queue<std::function<void()>> empty;
    taskQueue_.swap(empty);
}

size_t ThreadManager::getActiveThreadCount() const {
    std::lock_guard<std::mutex> lock(managerMutex_);
    
    size_t count = 0;
    for (const auto& threadInfo : threads_) {
        if (threadInfo && threadInfo->state == ThreadState::RUNNING) {
            count++;
        }
    }
    
    return count;
}

size_t ThreadManager::getTotalThreadCount() const {
    std::lock_guard<std::mutex> lock(managerMutex_);
    return threads_.size();
}

ThreadState ThreadManager::getThreadState(size_t threadIndex) const {
    std::lock_guard<std::mutex> lock(managerMutex_);
    
    if (threadIndex >= threads_.size() || !threads_[threadIndex]) {
        return ThreadState::TERMINATED;
    }
    
    return threads_[threadIndex]->state;
}

std::string ThreadManager::getThreadName(size_t threadIndex) const {
    std::lock_guard<std::mutex> lock(managerMutex_);
    
    if (threadIndex >= threads_.size() || !threads_[threadIndex]) {
        return "";
    }
    
    return threads_[threadIndex]->name;
}

void ThreadManager::lock() {
    syncMutex_.lock();
}

void ThreadManager::unlock() {
    syncMutex_.unlock();
}

void ThreadManager::wait() {
    std::unique_lock<std::mutex> lock(syncMutex_);
    syncCondition_.wait(lock);
}

void ThreadManager::notify() {
    syncCondition_.notify_one();
}

void ThreadManager::notifyAll() {
    syncCondition_.notify_all();
}

void ThreadManager::cleanup() {
    shutdownThreadPool();
    joinAll();
    
    std::lock_guard<std::mutex> lock(managerMutex_);
    threads_.clear();
}

void ThreadManager::joinAll() {
    std::lock_guard<std::mutex> lock(managerMutex_);
    
    for (auto& threadInfo : threads_) {
        if (threadInfo && threadInfo->thread && threadInfo->thread->joinable()) {
            threadInfo->thread->join();
            threadInfo->state = ThreadState::TERMINATED;
        }
    }
}
