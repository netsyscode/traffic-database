#ifndef RINGBUFFER_HPP_
#define RINGBUFFER_HPP_
#include <iostream>
#include <atomic>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <cstring>
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

    std::atomic_bool has_begin;

    std::atomic_uint64_t readPos_;
    std::atomic_uint64_t writePos_;

    std::vector<ThreadPointer*> readThreads;
    std::shared_mutex readThreadsMutex;

    std::vector<ThreadPointer*> writeThreads;
    std::shared_mutex writeThreadsMutex;
public:
    RingBuffer(u_int32_t capacity, u_int32_t dataLen) : capacity_(capacity),dataLen_(dataLen){
        this->buffer_ = new char[capacity*dataLen];
        this->signalBuffer_ = new bool[capacity]();
        this->readPos_ = 0;
        this->writePos_ = 0;
        this->readThreads = std::vector<ThreadPointer*>();
        this->writeThreads = std::vector<ThreadPointer*>();
        this->has_begin = false;
    }
    ~RingBuffer(){
        if(this->writeThreads.size() || this->readThreads.size()){
            std::cout << "Ring buffer warning: it is used by certain thread." <<std::endl;
        }
        delete[] buffer_;
    }
    bool addWriteThread(ThreadPointer* thread){
        std::unique_lock<std::shared_mutex> lock(this->writeThreadsMutex);
        for(auto t:this->writeThreads){
            if(t->id == thread->id){
                std::cerr << "Ring Buffer error: addWriteThread with exist id!" <<std::endl;
                lock.unlock();
                return false;
            }
        }
        thread->stop_ = false;
        thread->pause_ = false;
        this->writeThreads.push_back(thread);
        lock.unlock();
        this->has_begin = true;
        return true;
    }
    bool ereaseWriteThread(ThreadPointer* thread){
        std::unique_lock<std::shared_mutex> lock(this->writeThreadsMutex);
        for(auto it = this->writeThreads.begin();it!=this->writeThreads.end();++it){
            if((*(it))->id == thread->id){
                this->writeThreads.erase(it);
                for(auto t:this->readThreads){
                    t->cv_.notify_one();
                }
                // printf("Ring Buffer log: ereaseWriteThread %u.\n",thread->id);
                // std::cout << "Ring Buffer log: ereaseWriteThread " << thread->id <<std::endl;
                lock.unlock();
                return true;
            }
        }
        std::cerr << "Ring Buffer error: ereaseWriteThread with non-exist id " << thread->id << "!" <<std::endl;
        lock.unlock();
        return false;
    }
    bool addReadThread(ThreadPointer* thread){
        std::unique_lock<std::shared_mutex> lock(this->readThreadsMutex);
        for(auto t:this->readThreads){
            if(t->id == thread->id){
                std::cerr << "Ring Buffer error: addReadThread with exist id " << thread->id << "!" <<std::endl;
                lock.unlock();
                return false;
            }
        }
        thread->stop_ = false;
        thread->pause_ = false;
        this->readThreads.push_back(thread);
        lock.unlock();
        return true;
    }
    bool ereaseReadThread(ThreadPointer* thread){
        std::unique_lock<std::shared_mutex> lock(this->readThreadsMutex);
        for(auto it = this->readThreads.begin();it!=this->readThreads.end();++it){
            if((*(it))->id == thread->id){
                this->readThreads.erase(it);
                lock.unlock();
                return true;
            }
        }
        std::cerr << "Ring Buffer error: ereaseReadThread with non-exist id!" <<std::endl;
        lock.unlock();
        return false;
    }
    u_int32_t getWriteThreadNum() const{
        return this->writeThreads.size();
    }
    u_int32_t getReadThreadNum() const{
        return this->readThreads.size();
    }
    bool put(const void* data, u_int32_t thread_id, u_int32_t len){
        if(len!= this->dataLen_){
            std::cerr << "Ring Buffer error: put with error len!" <<std::endl;
            return false;
        }

        u_int64_t pos = this->writePos_++;// notice writePos is an automic variable
        
        // this->writePos_ = this->writePos_ % this->capacity_;
        pos %= this->capacity_;
        if(!this->signalBuffer_[pos]){//not writed
            memcpy(this->buffer_ + pos * this->dataLen_, data, this->dataLen_);
            this->signalBuffer_[pos] = true;
            std::shared_lock<std::shared_mutex> rlock(this->readThreadsMutex);
            for(auto t:this->readThreads){
                t->cv_.notify_one();
            }
            rlock.unlock();
            // std::cout << this->dataLen_ << std::endl;
            return true;
        }
        
        ThreadPointer* thread = nullptr;
        for(auto t:this->writeThreads){
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
        // thread->cv_.wait(lock, [this,thread,&pos]{return !(this->signalBuffer_[pos]) || thread->stop_;});
        // thread should not stop while putting elements
        thread->cv_.wait(lock, [this,thread,&pos]{return !(this->signalBuffer_[pos]);});
        // if(thread->stop_){
        //     std::cout << "Ring Buffer log: write thread " << thread_id << " stop." <<std::endl;
        //     return false;
        // }
        // std::cout << pos <<std::endl;
        memcpy(this->buffer_ + pos*this->dataLen_, data, this->dataLen_);
        this->signalBuffer_[pos] = true;
        std::shared_lock<std::shared_mutex> rlock(this->readThreadsMutex);
        for(auto t:this->readThreads){
            t->cv_.notify_one();
        }
        rlock.unlock();
        return true;
        // std::cout << pos <<std::endl;
        // return false;
    }
    std::string get(u_int32_t thread_id){
        std::string data = std::string();
        u_int64_t pos = this->readPos_++; // notice readPos is an automic variable
        // this->readPos_ = this->readPos_ % this->capacity_;
        pos %= this->capacity_;
        if(this->signalBuffer_[pos]){//writed
            data = std::string(this->buffer_ + pos*this->dataLen_, this->dataLen_);
            // memcpy(data_addr, this->buffer_ + pos*this->dataLen_, this->dataLen_);
            this->signalBuffer_[pos] = false;
            std::shared_lock<std::shared_mutex> rlock(this->writeThreadsMutex);
            for(auto t:this->writeThreads){
                t->cv_.notify_one();
            }
            rlock.unlock();
            return data;
        }
        if((!this->getWriteThreadNum()) && this->has_begin){
            // std::cout << "Ring Buffer log: read thread " << thread_id << " finish." <<std::endl;
            return std::string();
        }

        ThreadPointer* thread = nullptr;
        for(auto t:this->readThreads){
            if(t->id == thread_id){
                thread = t;
                break;
            }
        }
        if(thread == nullptr){
            std::cerr << "Ring Buffer error: get with non-exist thread id!" <<std::endl;
            return data;
        }
        std::unique_lock<std::mutex> lock(thread->mutex_);
        thread->cv_.wait(lock, [this,thread,&pos]{return this->signalBuffer_[pos] || thread->stop_ || (!this->getWriteThreadNum() && this->has_begin);});
        if(thread->stop_ ){
            std::cout << "Ring Buffer log: read thread " << thread_id << " stop." <<std::endl;
            return std::string();
        }
        if(this->signalBuffer_[pos]){
            data = std::string(this->buffer_ + pos*this->dataLen_, this->dataLen_);
            this->signalBuffer_[pos] = false;
            std::shared_lock<std::shared_mutex> rlock(this->writeThreadsMutex);
            for(auto t:this->writeThreads){
                t->cv_.notify_one();
            }
            rlock.unlock();
            return data;
        }
        // std::cout << "Ring Buffer log: read thread " << thread_id << " finish." <<std::endl;
        return std::string();
    } 
    void asynchronousStop(u_int32_t threadID){
        for(auto it = this->readThreads.begin();it!=this->readThreads.end();++it){
            if((*(it))->id == threadID){
                (*(it))->stop_ = true;
                (*(it))->cv_.notify_one();
                return;
            }
        }
        std::cerr << "Ring buffer error: asynchronousStop with non-exist id!" <<std::endl;
    }
    // void asynchronousPause(u_int32_t threadID){
    //     for(auto it = this->readThreads.begin();it!=this->readThreads.end();++it){
    //         if((*(it))->id == threadID){
    //             (*(it))->pause_ = true;
    //             (*(it))->cv_.notify_one();
    //             return;
    //         }
    //     }
    //     std::cerr << "Ring buffer error: asynchronousStop with non-exist id!" <<std::endl;
    // }
    // readPos - writePos, just for test
    int getPosDiff()const{
        if(this->readThreads.size() || this->writeThreads.size()){
            std::cerr << "Ring buffer error: outputToChar while it is used by certain thread!" <<std::endl;
            return -1;
        }
        return (int)this->readPos_ - (int)this->writePos_;
    }
};

#endif