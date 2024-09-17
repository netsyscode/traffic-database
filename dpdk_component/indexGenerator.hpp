#ifndef INDEXGENERATOR_HPP_
#define INDEXGENERATOR_HPP_
#include <iostream>
#include "../dpdk_lib/util.hpp"
#include "../dpdk_lib/indexBuffer.hpp"
#include "../dpdk_lib/pointerRingBuffer.hpp"

class IndexGenerator{
    // shared memory
    PointerRingBuffer* buffer;
    IndexBuffer* indexBuffer;

    u_int32_t indexCacheCount;
    u_int32_t cacheID;

    // thread member
    u_int32_t threadID;
    std::atomic_bool stop;

    Index* readIndexFromBuffer();
    void putIndexToCache(Index* index);

public:
    IndexGenerator(PointerRingBuffer* buffer,IndexBuffer* indexBuffer){
        this->buffer = buffer;
        this->indexBuffer = indexBuffer;
        this->indexCacheCount = indexBuffer->getCacheCount();
        this->cacheID = 0;
        this->threadID = std::numeric_limits<uint32_t>::max();
        this->stop = true;
    }
    ~IndexGenerator(){}
    void setThreadID(u_int32_t threadID);
    void run();
    void asynchronousStop();
};

#endif