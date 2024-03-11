#ifndef INDEXGENERATOR_HPP_
#define INDEXGENERATOR_HPP_
#include <iostream>
#include "../lib/ringBuffer.hpp"
#include "../lib/util.hpp"
#include "../lib/skipList.hpp"

class IndexGenerator{
    // shared memory
    RingBuffer* buffer;

    // thread member
    u_int32_t threadID;
    std::atomic_bool stop;

    Index readIndexFromBuffer();
    
public:
    IndexGenerator(RingBuffer* buffer){
        this->buffer = buffer;
        this->threadID = std::numeric_limits<uint32_t>::max();
        this->stop = true;
    }
    ~IndexGenerator(){}
    void setThreadID(u_int32_t threadID);
    void run();
    void asynchronousStop();
};

#endif