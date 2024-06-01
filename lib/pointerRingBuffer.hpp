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

    char** pointers;
    bool* signalBuffer_;
    
    std::atomic_bool has_begin;
    std::atomic_uint32_t readThreadCount;
    std::atomic_uint32_t writeThreadCount;

public:
    PointerRingBuffer(u_int32_t capacity):capacity_(capacity){
        this->pointers = new char*[this->capacity_];
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
        for(u_int32_t i = 0;i<this->capacity_;++i){
            if(this->pointers[i] != nullptr){
                delete this->pointers[i];
                this->pointers[i] = nullptr;
            }
        }
        delete this->pointers;
        delete this->signalBuffer_;
    }
    
};

#endif