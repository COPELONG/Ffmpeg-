#ifndef THREAD_H
#define THREAD_H

#include <thread>

class Thread
{
public:
    Thread() {}
    ~Thread() {
        if(thread_) {
            Thread::Stop();
        }
    }
    int Start() {}
    int Stop() {
        abort_ = 1;
        if(thread_) {
            thread_->join();
            delete thread_;
            thread_ = NULL;
        }
    }
    virtual void Run() = 0;
protected:
    int abort_ = 0;
    std::thread *thread_ = NULL;
};

#endif // THREAD_H
