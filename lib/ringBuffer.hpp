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

    std::vector<RingBufferThreadPointer*> readThreads;
    std::vector<RingBufferThreadPointer*> writeThreads;
public:
    RingBuffer(u_int32_t capacity, u_int32_t dataLen) : capacity_(capacity),dataLen_(dataLen){
        this->buffer_ = new char[capacity*dataLen];
        this->signalBuffer_ = new bool[capacity];
        this->readPos_ = 0;
        this->writePos_ = 0;
        this->readThreads = std::vector<RingBufferThreadPointer*>();
        this->writeThreads = std::vector<RingBufferThreadPointer*>();
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
    bool ereaseWriteThread(RingBufferThreadPointer* thread){
        for(auto it = this->writeThreads.begin();it!=this->writeThreads.end();++it){
            if((*(it))->id == thread->id){
                this->writeThreads.erase(it);
                return true;
            }
        }
        std::cerr << "Ring Buffer error: ereaseWriteThread with non-exist id!" <<std::endl;
        return false;
    }
    bool addReadThread(RingBufferThreadPointer* thread){
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
    bool ereaseReadThread(RingBufferThreadPointer* thread){
        for(auto it = this->readThreads.begin();it!=this->readThreads.end();++it){
            if((*(it))->id == thread->id){
                this->readThreads.erase(it);
                return true;
            }
        }
        std::cerr << "Ring Buffer error: ereaseReadThread with non-exist id!" <<std::endl;
        return false;
    }
    void put(const char* data, u_int32_t thread_id){
        u_int32_t pos = this->writePos_++;// notice writePos is an automic variable
        this->writePos_ = this->writePos_ % this->capacity_;
        pos %= this->capacity_;
        if(!this->signalBuffer_[pos]){//not writed
            memcpy(this->buffer_ + pos*this->dataLen_, data, this->dataLen_);
            this->signalBuffer_[pos] = true;
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
            std::cerr << "Ring Buffer error: getID with non-exist thread id!" <<std::endl;
            return;
        }

        std::unique_lock<std::mutex> lock(thread->mutex_);
        thread->cv_.wait(lock, [this,thread,&pos]{return !(this->signalBuffer_[pos]) || thread->stop_;});
        if(thread->stop_){
            std::cout << "Ring Buffer log: thread " << thread_id << " stop." <<std::endl;
            return;
        }
        memcpy(this->buffer_ + pos*this->dataLen_, data, this->dataLen_);
        this->signalBuffer_[pos] = true;
        for(auto t:this->readThreads){
            t->cv_.notify_one();
        }
        return;
    }
    RingBufferData get(u_int32_t thread_id){
        RingBufferData data = {
            .data = nullptr,
            .err = 0,
        };
        u_int32_t pos = this->readPos_++; // notice readPos is an automic variable
        this->readPos_ = this->readPos_ % this->capacity_;
        pos %= this->capacity_;
        if(this->signalBuffer_[pos]){//writed
            // data.data = this->buffer_[pos];
            data.data = new char[this->dataLen_];
            memcpy(data.data, this->buffer_ + pos*this->dataLen_, this->dataLen_);
            this->signalBuffer_[pos] = false;
            for(auto t:this->writeThreads){
                t->cv_.notify_one();
            }
            return data;
        }

        RingBufferThreadPointer* thread = nullptr;
        for(auto t:this->writeThreads){
            if(t->id == thread_id){
                thread = t;
                break;
            }
        }
        if(thread == nullptr){
            std::cerr << "Ring Buffer error: getID with non-exist thread id!" <<std::endl;
            data.err = 1;
            return data;
        }

        std::unique_lock<std::mutex> lock(thread->mutex_);
        thread->cv_.wait(lock, [this,thread,&pos]{return this->signalBuffer_[pos] || thread->stop_;});
        if(thread->stop_){
            std::cout << "Ring Buffer log: thread " << thread_id << " stop." <<std::endl;
            data.err = 2;
            return data;
        }
        data.data = new char[this->dataLen_];
        memcpy(data.data, this->buffer_ + pos*this->dataLen_, this->dataLen_);
        this->signalBuffer_[pos] = false;
        for(auto t:this->readThreads){
            t->cv_.notify_one();
        }
        return data;
    } 
};

#endif