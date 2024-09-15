#ifndef CONTROLLER_HPP_
#define CONTROLLER_HPP_
#include <iostream>
#include <thread>
#include <vector>
#include "dpdkReader.hpp"
// #include "indexGenerator.hpp"
// #include "storage.hpp"
// #include "truncateChecker.hpp"
// #include "querier.hpp"
#include "directStorage.hpp"

struct InitData{
    u_int32_t index_ring_capacity;
    u_int32_t storage_ring_capacity;
    u_int64_t truncate_interval;
    u_int16_t nb_rx;
    u_int32_t pcap_header_len;
    u_int32_t eth_header_len;
    u_int64_t file_capacity;
    u_int32_t index_thread_num;
    u_int32_t direct_storage_thread_num;
    std::string pcap_header;
    std::string bpf_prog_name;
};

class Controller{
private:
    std::string pcapHeader;
    std::string bpf_prog_name;

    PointerRingBuffer* indexRing;
    // std::vector<Truncator*>* truncators;
    // PointerRingBuffer* storageRing;
    // std::vector<StorageMeta>* storageMetas;
    DPDK* dpdk;
    std::vector<MemoryBuffer*> buffers;
    
    std::vector<DPDKReader*> readers;
    // std::vector<std::vector<IndexGenerator*>> indexGenerators;
    // std::vector<std::vector<std::thread*>> indexGeneratorThreads;
    // Storage* storage;
    // std::thread* storageThread;
    // TruncateChecker* checker;
    // std::thread* checkThread;
    std::vector<DirectStorage*> directStorages;
    std::vector<std::thread*> directStorageThreads;

    // Querier* querier;
    // std::thread* querierThread;

    void threadsRun();
    // void queryThreadRun();
    void threadsStop();
    void clear();

public:
    Controller(){
        this->indexRing = nullptr;
        // this->truncators = new std::vector<Truncator*>(INDEX_NUM,nullptr);
        // this->storageRing = nullptr;
        // this->storageMetas = nullptr;
        this->dpdk = nullptr;
        this->buffers = std::vector<MemoryBuffer*>();
        this->readers = std::vector<DPDKReader*>();
        // this->indexGenerators = std::vector<std::vector<IndexGenerator*>>(INDEX_NUM,std::vector<IndexGenerator*>());
        // this->indexGeneratorThreads = std::vector<std::vector<std::thread*>>(INDEX_NUM,std::vector<std::thread*>());
        // this->storage = nullptr;
        // this->checker = nullptr;
        // this->storageThread = nullptr;
        // this->checkThread = nullptr;
        // this->querier = nullptr;
        // this->querierThread = nullptr;
        this->directStorages = std::vector<DirectStorage*>();
        this->directStorageThreads = std::vector<std::thread*>();
    }
    ~Controller(){
        this->clear();
    }
    void init(InitData init_data);
    void run();
};

#endif