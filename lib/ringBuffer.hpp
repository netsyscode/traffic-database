#ifndef RINGBUFFER_HPP_
#define RINGBUFFER_HPP_
#include <iostream>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>

struct RingBufferThreadPointer{
    u_int32_t id;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic_bool stop_;
};

template <class T>
class RingBuffer{
    T* buffer_;
    const u_int32_t capacity_;
    std::atomic_uint32_t size_;
    std::atomic_uint32_t readPos_;
    std::atomic_uint32_t writePos_;
    
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;

    std::vector<RingBufferThreadPointer*> readThreads;
    std::vector<RingBufferThreadPointer*> writeThreads;

    // std::atomic_uint32_t writeThreadCount;
    // std::atomic_uint32_t readThreadCount;
public:
    RingBuffer(u_int32_t capacity) : capacity_(capacity),size_t(0),readPos_(0), writePos_(0){
        this->buffer_ = new T[capacity];
        this->writeThreadCount = 0;
        this->readThreadCount = 0;
    }
    ~RingBuffer(){
        if(this->writeThreads.size() || this->readThreads.size()){
            std::cout << "Ring buffer warning: it is used by certain thread." <<std::endl;
        }
        delete[] buffer_;
    }
    bool addWriteThread(RingBufferThreadPointer* thread){
        for(auto t:this->writeThreads){
            if(t->id == thread->id){
                std::cerr << "Ring Buffer error: addWriteThread with exist id!" <<std::endl;
                return false;
            }
        }
        thread->stop_ = false;
        this->writeThreads.push_back(thread);
        return true;
    }
    void ereaseWriteThread(){
        for(auto it = this->writeThreads.begin();it!=this->writeThreads.end();++it){
            if((*(it))->id == thread->id){
                this->writeThreads.erase(it);
                return true;
            }
        }
        std::cerr << "Ring Buffer error: ereaseWriteThread with non-exist id!" <<std::endl;
        return false;
    }
    void addReadThread(){
        for(auto t:this->readThreads){
            if(t->id == thread->id){
                std::cerr << "Ring Buffer error: addReadThread with exist id!" <<std::endl;
                return false;
            }
        }
        thread->stop_ = false;
        this->readThreads.push_back(thread);
        return true;
    }
    void ereaseReadThread(){
        for(auto it = this->readThreads.begin();it!=this->readThreads.end();++it){
            if((*(it))->id == thread->id){
                this->readThreads.erase(it);
                return true;
            }
        }
        std::cerr << "Ring Buffer error: ereaseReadThread with non-exist id!" <<std::endl;
        return false;
    }
    void put(T data, u_int32_t thread_id){
        u_int32_t size = this->size_++;
        if(this->size_ < this->capacity_ - this->writeThreads.size()){//direct writing
            u_int32_t pos = this->writePos_++;
            this->writePos_ %= this->capacity_;
            pos %= this->capacity_;
            this->buffer_[pos]=data;
            for(auto t:this->readThreads){
                t->cv_.notify_one();
            }
            return;
        }
        
        RingBufferThreadPointer* thread = nullptr;
        for(auto t:this->readThreads){
            if(t->id == thread_id){
                thread = t;
                break;
            }
        }
        if(thread == nullptr){
            std::cerr << "Array list error: getID with non-exist thread id!" <<std::endl;
            return;
        }
        std::unique_lock<std::mutex> lock(this->writeThreads[i]);
    }
    // void (char* item, u_int32_t len) {
    //     if(len > capacity_){
    //         std::cerr << "Ring buffer error: too long to preduce!" <<std::endl;
    //         return;
    //     }
    //     std::unique_lock<std::mutex> lock(mutex_);
    //     notFull_.wait(lock, [this]{ return size_ < capacity_;});
    //     memcpy(buffer_+writePos_,item,len);
    //     writePos_ = (writePos_ + len) % capacity_;
    //     size_+=len;
    //     notEmpty_.notify_one();
    // }
    // char* consume(){
    //     std::unique_lock<std::mutex> lock(mutex_);
    //     notEmpty_.wait(lock, [this]{ return size_ > 0; });
    //     data_header* header = (data_header*)(buffer_+readPos_);
    //     u_int32_t len = header->caplen;
    //     char* item = new char[sizeof(data_header)+len];
    //     memcpy(item,buffer_+readPos_,sizeof(data_header)+len);
    //     readPos_ = (readPos_ + sizeof(data_header)+len) % capacity_;
    //     size_-=sizeof(data_header)+len;
    //     notFull_.notify_one();
    //     return item;
    // }
};

#endif