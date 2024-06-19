#ifndef INDEXGENERATOR_HPP_
#define INDEXGENERATOR_HPP_
#include <iostream>
#include "../dpdk_lib/util.hpp"
#include "../dpdk_lib/skipList.hpp"
#include "../dpdk_lib/truncator.hpp"

class IndexGenerator{
    const u_int32_t keyLen;

    // shared memory
    PointerRingBuffer* buffer;
    SkipList* indexCache;
    Truncator* truncator;

    // thread member
    u_int32_t threadID;
    std::atomic_bool stop;

    u_int64_t duration_time;

    // we needn't think about  pause now -- just stop
    // std::atomic_bool pause;

    // RingBuffer* newBuffer;
    // SkipList* newIndexCache;

    // for old flows, but for flow index, NO need to be considered
    // RingBuffer* oldBuffer;
    // SkipList* oldIndexCache;

    Index* readIndexFromBuffer();
    void putIndexToCache(const Index* index);

    // void truncate();
public:
    IndexGenerator(PointerRingBuffer* buffer, Truncator* truncator, u_int32_t keyLen):keyLen(keyLen){
        this->buffer = buffer;
        this->truncator = truncator;
        this->indexCache = truncator->indexMap;
        this->indexCache->addWriteThread();
        this->threadID = std::numeric_limits<uint32_t>::max();
        this->stop = true;
        // this->pause = false;

        this->duration_time = 0;
    }
    ~IndexGenerator(){}
    void setThreadID(u_int32_t threadID);
    void run();
    void asynchronousStop();
    // void asynchronousPause(RingBuffer* newBuffer, SkipList* newIndexCache);
};

#endif