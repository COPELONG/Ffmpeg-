#ifndef QUEUE_H
#define QUEUE_H
#include <mutex>
#include <condition_variable>
#include <queue>

template <typename T>
class Queue
{
public:
    Queue() {}
    ~ Queue() {}
    void Abort() {
        abort_ = 1;
        cond_.notify_all();
    }

    int Push(T val) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(1 == abort_) {
            return -1;
        }
        queue_.push(val);
        cond_.notify_one();
        return 0;
    }

    int Pop(T &val, const int timeout = 0) {
        std::unique_lock<std::mutex> lock(mutex_);
        if(queue_.empty()) {
            // 等待push或者超时唤醒
            cond_.wait_for(lock, std::chrono::milliseconds(timeout), [this] {
                return !queue_.empty() | abort_;
            });
        }
        if(1 == abort_) {
            return -1;
        }
        if(queue_.empty()) {
            return -2;
        }
        val = queue_.front();
        queue_.pop();
        return 0;
    }

    int Front(T &val) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(1 == abort_) {
            return -1;
        }
        if(queue_.empty()) {
            return -2;
        }
        val = queue_.front();
        return 0;
    }

    int Size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }


private:
    int abort_ = 0;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<T> queue_;
};

#endif // QUEUE_H
