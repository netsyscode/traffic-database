#ifndef RINGBUFFER_HPP_
#define RINGBUFFER_HPP_
#include <iostream>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "util.hpp"

// template <class T>
struct RingBufferData{
    char* data;
    u_int8_t err;
};

// template <class T>
class RingBuffer{
    char* buffer_;
    bool* signalBuffer_; // record if writed
    const u_int32_t capacity_;
    const u_int32_t dataLen_; // length of each data

    std::atomic_uint32_t readPos_;
    std::atomic_uint32_t writePos_;

    std::vector<ThreadReadPointer*> readThreads;
    std::vector<ThreadReadPointer*> writeThreads;
public:
    RingBuffer(u_int32_t capacity, u_int32_t dataLen) : capacity_(capacity),dataLen_(dataLen){
        this->buffer_ = new char[capacity*dataLen];
        this->signalBuffer_ = new bool[capacity];
        this->readPos_ = 0;
        this->writePos_ = 0;
        this->readThreads = std::vector<ThreadReadPointer*>();
        this->writeThreads = std::vector<ThreadReadPointer*>();
    }
    ~RingBuffer(){
        if(this->writeThreads.size() || this->readThreads.size()){
            std::cout << "Ring buffer warning: it is used by certain thread." <<std::endl;
        }
        delete[] buffer_;
    }
    bool addWriteThread(ThreadReadPointer* thread){
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
    bool ereaseWriteThread(ThreadReadPointer* thread){
        for(auto it = this->writeThreads.begin();it!=this->writeThreads.end();++it){
            if((*(it))->id == thread->id){
                this->writeThreads.erase(it);
                return true;
            }
        }
        std::cerr << "Ring Buffer error: ereaseWriteThread with non-exist id!" <<std::endl;
        return false;
    }
    bool addReadThread(ThreadReadPointer* thread){
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
    bool ereaseReadThread(ThreadReadPointer* thread){
        for(auto it = this->readThreads.begin();it!=this->readThreads.end();++it){
            if((*(it))->id == thread->id){
                this->readThreads.erase(it);
                return true;
            }
        }
        std::cerr << "Ring Buffer error: ereaseReadThread with non-exist id!" <<std::endl;
        return false;
    }
    bool put(const void* data, u_int32_t thread_id, u_int32_t len){
        if(len!= this->dataLen_){
            std::cerr << "Ring Buffer error: put with error len!" <<std::endl;
            return false;
        }

        u_int32_t pos = this->writePos_++;// notice writePos is an automic variable
        this->writePos_ = this->writePos_ % this->capacity_;
        pos %= this->capacity_;
        if(!this->signalBuffer_[pos]){//not writed
            memcpy(this->buffer_ + pos*this->dataLen_, data, this->dataLen_);
            this->signalBuffer_[pos] = true;
            for(auto t:this->readThreads){
                t->cv_.notify_one();
            }
            return true;
        }
        
        ThreadReadPointer* thread = nullptr;
        for(auto t:this->readThreads){
            if(t->id == thread_id){
                thread = t;
                break;
            }
        }
        if(thread == nullptr){
            std::cerr << "Ring Buffer error: put with non-exist thread id!" <<std::endl;
            return false;
        }

        std::unique_lock<std::mutex> lock(thread->mutex_);
        thread->cv_.wait(lock, [this,thread,&pos]{return !(this->signalBuffer_[pos]) || thread->stop_;});
        if(thread->stop_){
            std::cout << "Ring Buffer log: write thread " << thread_id << " stop." <<std::endl;
            return false;
        }
        memcpy(this->buffer_ + pos*this->dataLen_, data, this->dataLen_);
        this->signalBuffer_[pos] = true;
        for(auto t:this->readThreads){
            t->cv_.notify_one();
        }
        return true;
    }
    bool get(u_int32_t thread_id, void* data_addr, u_int32_t len){
        if(len!= this->dataLen_){
            std::cerr << "Ring Buffer error: get with error len!" <<std::endl;
            return false;
        }
        u_int32_t pos = this->readPos_++; // notice readPos is an automic variable
        this->readPos_ = this->readPos_ % this->capacity_;
        pos %= this->capacity_;
        if(this->signalBuffer_[pos]){//writed
            memcpy(data_addr, this->buffer_ + pos*this->dataLen_, this->dataLen_);
            this->signalBuffer_[pos] = false;
            for(auto t:this->writeThreads){
                t->cv_.notify_one();
            }
            return true;
        }

        ThreadReadPointer* thread = nullptr;
        for(auto t:this->writeThreads){
            if(t->id == thread_id){
                thread = t;
                break;
            }
        }
        if(thread == nullptr){
            std::cerr << "Ring Buffer error: getID with non-exist thread id!" <<std::endl;
            return false;
        }

        std::unique_lock<std::mutex> lock(thread->mutex_);
        thread->cv_.wait(lock, [this,thread,&pos]{return this->signalBuffer_[pos] || thread->stop_;});
        if(thread->stop_){
            std::cout << "Ring Buffer log: thread " << thread_id << " stop." <<std::endl;
            return false;
        }

        memcpy(data_addr, this->buffer_ + pos*this->dataLen_, this->dataLen_);
        this->signalBuffer_[pos] = false;
        for(auto t:this->readThreads){
            t->cv_.notify_one();
        }
        return true;
    } 
};

#endif