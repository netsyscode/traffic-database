#ifndef POINTERRINGBUFFER_HPP_
#define POINTERRINGBUFFER_HPP_
#include <iostream>
#include <unistd.h>
#include "header.hpp"
#include "util.hpp"

// write Not covered, read covered
class PointerRingBuffer{
private:
    const u_int32_t capacity_;
    std::atomic_uint64_t writePos;
    std::atomic_uint64_t readPos;

    void** pointers;
    std::atomic_bool* signalBuffer_;
    
    // std::atomic_bool has_begin;
    std::atomic_uint32_t readThreadCount;
    std::atomic_uint32_t writeThreadCount;

    std::atomic_bool stop;

    bool isPowerOfTwo(u_int32_t n) {
        return (n & (n - 1)) == 0;
    }
public:
    PointerRingBuffer(u_int32_t capacity):capacity_(capacity){
        if(this->capacity_ & (this->capacity_ - 1)){
            printf("PointerRingBuffer error: capacity %u is not power of 2!\n",capacity);
            this->pointers = nullptr;
            this->signalBuffer_ = nullptr;
            return;
        }
        this->pointers = new void*[this->capacity_];
        for(u_int32_t i = 0;i<this->capacity_;++i){
            this->pointers[i] = nullptr;
        }
        this->signalBuffer_ = new std::atomic_bool[this->capacity_];
        for(u_int32_t i = 0;i<this->capacity_;++i){
            this->signalBuffer_[i] = false;
        }
        this->writePos = 0;
        this->readPos = 0;
        
        this->readThreadCount = 0;
        this->writeThreadCount = 0;
        // this->has_begin = false;
        this->stop = false;
    }
    ~PointerRingBuffer(){
        if(this->readThreadCount || this->writeThreadCount){
            std::cout << "Pointer ring buffer warning: destroy while it is used by certain thread." <<std::endl;
        }
        delete this->pointers;
        delete this->signalBuffer_;
    }
    bool put(void* data){
        if(this->pointers == nullptr){
            printf("Pointer ring buffer error: put with null pointers!\n");
            return false;
        }

        u_int64_t pos = this->writePos++;// notice writePos is an automic variable
        pos %= this->capacity_;

        while(this->signalBuffer_[pos]); // wait util not writed

        this->pointers[pos] = data;
        this->signalBuffer_[pos] = true;

        return true;
    }
    void* get(){
        if(this->pointers == nullptr){
            printf("Pointer ring buffer error: get with null pointers!\n");
            return nullptr;
        }
        
        u_int64_t pos = this->readPos++; // notice readPos is an automic variable
        pos %= this->capacity_;
        // printf("get at %llu.\n",pos);

        while(!this->signalBuffer_[pos]){
            if(this->stop){
                break;
            }
        } // wait util writed

        if(this->stop){
            return nullptr;
        }

        void* data = this->pointers[pos];
        this->signalBuffer_[pos] = false;
        
        return data;
    }
    bool initSucceed()const{
        return this->pointers != nullptr;
    }
    void asynchronousStop(){
        this->stop = true;
    }
};
#endif