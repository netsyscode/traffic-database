#ifndef RINGBUFFER_HPP_
#define RINGBUFFER_HPP_
#include <iostream>
#include <atomic>
#include "./header.hpp"

class RingBuffer{
    char* buffer_;
    const u_int32_t capacity_;
    std::atomic_uint32_t size_;
    std::atomic_uint32_t readPos_;
    std::atomic_uint32_t writePos_;
    
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;

    std::atomic_uint32_t threadCount;
    std::atomic_uint32_t turnCount;
    // std::atomic_bool lowBoundWaring;
    // std::atomic_bool highBoundWaring;
public:
    RingBuffer(u_int32_t capacity) : capacity_(capacity), size_(0), readPos_(0), writePos_(0){
        this->buffer_ = new char[capacity];
        this->threadCount = 0;
        this->turnCount = 0;
    }
    ~RingBuffer(){
        if(this->threadCount){
            std::cout << "Ring buffer warning: it is used by certain thread." <<std::endl;
        }
        delete[] buffer_;
    }
    void addThread(){
        this->threadCount++;
    }
    void ereaseThread(){
        this->threadCount--;
    }
    void produce(char* item, u_int32_t len) {
        if(len > capacity_){
            std::cerr << "Ring buffer error: too long to preduce!" <<std::endl;
            return;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [this]{ return size_ < capacity_;});
        memcpy(buffer_+writePos_,item,len);
        writePos_ = (writePos_ + len) % capacity_;
        size_+=len;
        notEmpty_.notify_one();
    }
    char* consume(){
        std::unique_lock<std::mutex> lock(mutex_);
        notEmpty_.wait(lock, [this]{ return size_ > 0; });
        data_header* header = (data_header*)(buffer_+readPos_);
        u_int32_t len = header->caplen;
        char* item = new char[sizeof(data_header)+len];
        memcpy(item,buffer_+readPos_,sizeof(data_header)+len);
        readPos_ = (readPos_ + sizeof(data_header)+len) % capacity_;
        size_-=sizeof(data_header)+len;
        notFull_.notify_one();
        return item;
    }
};

#endif