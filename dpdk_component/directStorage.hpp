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
    bool runUnit();
public:
    DirectStorage(){
        this->buffers = std::vector<MemoryBuffer*>();
        this->checkID = std::vector<u_int32_t>();
        this->stop = false;
    }
    ~DirectStorage()=default;
    void addBuffer(MemoryBuffer* buffer);
    int run();
    void asynchronousStop();
};


#endif