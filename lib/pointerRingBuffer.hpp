#ifndef POINTERRINGBUFFER_HPP
#define POINTERRINGBUFFER_HPP_
#include <iostream>
#include <unistd.h>
#include "header.hpp"
#include "util.hpp"

// write Not covered, read covered
class PointerRingBuffer{
    const u_int32_t capacity_;
    std::atomic_uint64_t writePos;
    std::atomic_uint64_t readPos;

    void** pointers;
    bool* signalBuffer_;
    
    std::atomic_bool has_begin;
    std::atomic_uint32_t readThreadCount;
    std::atomic_uint32_t writeThreadCount;

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
        this->signalBuffer_ = new bool[this->capacity_];
        this->writePos = 0;
        
        this->readThreadCount = 0;
        this->writeThreadCount = 0;
        this->has_begin = false;
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
            return nullptr;
        }
        
        u_int64_t pos = this->readPos++; // notice readPos is an automic variable
        pos %= this->capacity_;

        while(!this->signalBuffer_[pos]); // wait util writed

        void* data = this->pointers[pos];
        this->signalBuffer_[pos] = false;
        
        return data;
    }
    bool initSucceed()const{
        return this->pointers != nullptr;
    }
};
#endif