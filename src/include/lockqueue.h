#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

template <typename T>
class LockQueue
{
public:
    void Push(const T &data)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        logqueue_.push(data);
        cond_.notify_one();
    }
    T Pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(logqueue_.empty())
        {
            cond_.wait(lock);
        }
        T data = logqueue_.front();
        logqueue_.pop();
        return data;
    }
private:
    std::queue<T>logqueue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};