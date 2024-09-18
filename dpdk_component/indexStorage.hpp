#ifndef INDEXSTORAGE_HPP_
#define INDEXSTORAGE_HPP_

#include "../dpdk_lib/indexBuffer.hpp"
#include "../dpdk_lib/zOrderTree.hpp"
#include "../dpdk_lib/memoryBuffer.hpp"

class IndexStorage{
private:
    IndexBuffer* buffer;
    u_int32_t checkID;
    u_int32_t cacheCount;
    std::atomic_bool stop;
    void processSkipList(SkipList* list, u_int64_t ts, u_int32_t id);
    bool runUnit();
public:
    IndexStorage(IndexBuffer* buffer){
        this->buffer = buffer;
        this->checkID = 0;
        this->cacheCount = this->buffer->getCacheCount();
        this->stop = false;
    }
    ~IndexStorage() = default;
    void run();
    void asynchronousStop();
};

#endif