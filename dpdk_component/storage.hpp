#ifndef STORAGE_HPP_
#define STORAGE_HPP_
#include <iostream>
#include <fstream>
#include "../dpdk_lib/truncator.hpp"
#include "../dpdk_lib/util.hpp"

struct StorageMeta{
    u_int64_t index_offset;
    u_int64_t index_end;
    u_int64_t time_start;
    u_int64_t time_end;
};

class Storage{
private:
    PointerRingBuffer* ring;
    u_int64_t index_offset[INDEX_NUM];
    std::vector<StorageMeta>* storageMetas;

    bool stop;

    void* readRing();
    void store(TruncateUnit* unit);
public:
    Storage(PointerRingBuffer* ring, std::vector<StorageMeta>* storageMetas){
        this->ring = ring;
        this->storageMetas = storageMetas;
        for(int i=0;i<INDEX_NUM;++i){
            this->index_offset[i]=0;
            std::ofstream indexFile(index_name[i], std::ios::binary);
            if (!indexFile.is_open()) {
                std::cerr << "Storage monitor error: store to non-exist file name " << index_name[i] << "!" << std::endl;
                return;
            }
            indexFile.clear();
            indexFile.close();
        }
        this->stop = true;
    }
    ~Storage(){}
    void run();
    void asynchronousStop();
};

#endif