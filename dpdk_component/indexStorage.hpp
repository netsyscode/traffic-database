#ifndef INDEXSTORAGE_HPP_
#define INDEXSTORAGE_HPP_

#include "../dpdk_lib/indexBuffer.hpp"
#include "../dpdk_lib/zOrderTree.hpp"
#include "../dpdk_lib/memoryBuffer.hpp"

class IndexStorage{
private:
    // IndexBuffer* buffer;
    std::vector<IndexBuffer*> buffers;
    std::vector<u_int32_t> bufferIDs;
    std::vector<u_int32_t> checkIDs;
    std::vector<u_int32_t> cacheCounts;
    // u_int32_t bufferID;
    // u_int32_t checkID;
    // u_int32_t cacheCount;
    std::atomic_bool stop;
    void processSkipList(SkipList* list, u_int64_t ts, u_int32_t id);
    bool runUnit();
public:
    // IndexStorage(IndexBuffer* buffer, u_int32_t buffer_id){
    IndexStorage(){
        this->buffers = std::vector<IndexBuffer*>();
        this->bufferIDs = std::vector<u_int32_t>();
        this->checkIDs = std::vector<u_int32_t>();
        // this->cacheCount = this->buffer->getCacheCount();
        this->cacheCounts = std::vector<u_int32_t>();
        this->stop = false;
    }
    ~IndexStorage() = default;
    void addBuffer(IndexBuffer* buffer, u_int32_t buffer_id);
    void run();
    void asynchronousStop();
};

#endif