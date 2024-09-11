#ifndef DIRECTSTORAGE_HPP_
#define DIRECTSTORAGE_HPP_

#include <vector>
#include <atomic>
#include "../dpdk_lib/memoryBuffer.hpp"

class DirectStorage{
private:
    std::vector<MemoryBuffer*> buffers;
    std::vector<u_int32_t> checkID;
    std::atomic_bool stop;
    u_int32_t testID;
    bool runUnit();
public:
    DirectStorage(u_int32_t id){
        this->buffers = std::vector<MemoryBuffer*>();
        this->checkID = std::vector<u_int32_t>();
        this->stop = false;
        this->testID = id;
    }
    ~DirectStorage()=default;
    void addBuffer(MemoryBuffer* buffer);
    int run();
    void asynchronousStop();
};


#endif