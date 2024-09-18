#ifndef INDEXGENERATOR_HPP_
#define INDEXGENERATOR_HPP_
#include <iostream>
#include "../dpdk_lib/util.hpp"
#include "../dpdk_lib/indexBuffer.hpp"
#include "../dpdk_lib/pointerRingBuffer.hpp"

class IndexGenerator{
    // shared memory
    PointerRingBuffer* buffer;
    std::vector<IndexBuffer*>* indexBuffers;

    u_int32_t indexCacheCount;
    u_int32_t cacheID;

    // thread member
    u_int32_t threadID;
    std::atomic_bool stop;

    u_int64_t duration_time;

    Index* readIndexFromBuffer();
    void putIndexToCache(Index* index);

public:
    IndexGenerator(PointerRingBuffer* buffer,std::vector<IndexBuffer*>* indexBuffers, u_int32_t indexCacheCount, u_int32_t threadID){
        this->buffer = buffer;
        this->indexBuffers = indexBuffers;
        this->indexCacheCount = indexCacheCount;
        this->cacheID = 0;
        this->threadID = threadID;
        this->stop = true;
        this->duration_time = 0;
    }
    ~IndexGenerator(){}
    void setThreadID(u_int32_t threadID);
    void run();
    void asynchronousStop();
};

#endif