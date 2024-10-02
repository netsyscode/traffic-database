#ifndef LOCKRING_HPP_
#define LOCKRING_HPP_

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class CircularBuffer {
public:
    CircularBuffer(size_t size) : buffer(size), head(0), tail(0), count(0) {
        this->stop = false;
    }

    // 写入数据
    void put(void* data) {
        std::unique_lock<std::mutex> lock(mtx);
        not_full.wait(lock, [this]() { return count < buffer.size(); });

        buffer[head] = data;
        head = (head + 1) % buffer.size();
        ++count;

        not_empty.notify_one();
    }

    // 读取数据
    void* get() {
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [this]() { return count > 0 || this->stop; });

        if(count == 0){
            return nullptr;
        }

        auto data = buffer[tail];
        tail = (tail + 1) % buffer.size();
        --count;

        not_full.notify_one();
        return data;
    }

    void asynchronousStop(){
        this->stop = true;
        not_empty.notify_all();
    }

private:
    std::vector<void*> buffer;
    size_t head, tail;
    std::atomic<size_t> count;
    std::mutex mtx;
    std::condition_variable not_empty;
    std::condition_variable not_full;
    bool stop;
};

#endif