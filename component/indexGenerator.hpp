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
    SkipList* indexCache;

    // thread member
    u_int32_t threadID;
    std::atomic_bool stop;
    // we needn't think about  pause now -- just stop
    // std::atomic_bool pause;

    // RingBuffer* newBuffer;
    // SkipList* newIndexCache;

    // for old flows, but for flow index, NO need to be considered
    // RingBuffer* oldBuffer;
    // SkipList* oldIndexCache;

    Index readIndexFromBuffer();
    void putIndexToCache(const Index& index);

    // void truncate();
public:
    IndexGenerator(RingBuffer* buffer, SkipList* indexCache, u_int32_t keyLen):keyLen(keyLen){
        this->buffer = buffer;
        this->indexCache = indexCache;
        this->threadID = std::numeric_limits<uint32_t>::max();
        this->stop = true;
        // this->pause = false;
    }
    ~IndexGenerator(){}
    void setThreadID(u_int32_t threadID);
    void run();
    void asynchronousStop();
    // void asynchronousPause(RingBuffer* newBuffer, SkipList* newIndexCache);
};

#endif