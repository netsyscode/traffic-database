#ifndef INDEXGENERATOR_HPP_
#define INDEXGENERATOR_HPP_
#include <iostream>
#include "../lib/ringBuffer.hpp"
#include "../lib/util.hpp"
#include "../lib/skipList.hpp"


class IndexGenerator{
    const u_int32_t keyLen;

    // shared memory
    RingBuffer* buffer;
    void* indexCache; // skipList, static cast when use

    // thread member
    u_int32_t threadID;
    std::atomic_bool stop;

    Index readIndexFromBuffer();
    void putIndexToCache(const Index& index);
public:
    IndexGenerator(RingBuffer* buffer, void* indexCache, u_int32_t keyLen):keyLen(keyLen){
        this->buffer = buffer;
        this->indexCache = indexCache;
        this->threadID = std::numeric_limits<uint32_t>::max();
        this->stop = true;
    }
    ~IndexGenerator(){}
    void setThreadID(u_int32_t threadID);
    void run();
    void asynchronousStop();
};

#endif