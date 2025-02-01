#ifndef CONTROLLER_HPP_
#define CONTROLLER_HPP_
#include <iostream>
#include <thread>
#include <vector>
#include "dpdkReader.hpp"
#include "indexGenerator.hpp"
// #include "storage.hpp"
// #include "truncateChecker.hpp"
// #include "querier.hpp"
#include "directStorage.hpp"
#include "indexStorage.hpp"

struct InitData{
    u_int32_t index_ring_capacity;
    // u_int32_t storage_ring_capacity;
    // u_int64_t truncate_interval;
    u_int16_t nb_rx;
    u_int32_t pcap_header_len;
    u_int32_t eth_header_len;
    u_int64_t file_capacity;
    u_int32_t index_thread_num;
    u_int32_t direct_storage_thread_num;
    u_int32_t index_storage_thread_num;
    u_int32_t max_node;
    std::string pcap_header;
    std::string bpf_prog_name;
    bool bind_core;
    u_int32_t controller_core_id;
    std::vector<u_int32_t> dpdk_core_id_list;
    std::vector<u_int32_t> packet_core_id_list;
    std::vector<u_int32_t> indexing_core_id_list;
};

class Controller{
private:
    std::string pcapHeader;
    std::string bpf_prog_name;

    std::vector<PointerRingBuffer*>* indexRings;
    std::vector<IndexBuffer*>* indexBuffers;

    DPDK* dpdk;
    std::vector<MemoryBuffer*> buffers;
    
    std::vector<DPDKReader*> readers;

    std::vector<IndexGenerator*> indexGenerators;
    std::vector<std::thread*> indexGeneratorThreads;

    std::vector<IndexStorage*> indexStorages;
    std::vector<std::thread*> indexStorageThreads;
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

    void bindCore(u_int32_t cpu);

public:
    Controller(){
        this->indexRings = new std::vector<PointerRingBuffer*>();
        this->indexBuffers = new std::vector<IndexBuffer*>();
        // this->truncators = new std::vector<Truncator*>(INDEX_NUM,nullptr);
        // this->storageRing = nullptr;
        // this->storageMetas = nullptr;
        this->dpdk = nullptr;
        this->buffers = std::vector<MemoryBuffer*>();
        this->readers = std::vector<DPDKReader*>();
        this->indexGenerators = std::vector<IndexGenerator*>();
        this->indexGeneratorThreads = std::vector<std::thread*>();
        // this->storage = nullptr;
        // this->checker = nullptr;
        // this->storageThread = nullptr;
        // this->checkThread = nullptr;
        // this->querier = nullptr;
        // this->querierThread = nullptr;
        this->directStorages = std::vector<DirectStorage*>();
        this->directStorageThreads = std::vector<std::thread*>();
        this->indexStorages = std::vector<IndexStorage*>();
        this->indexStorageThreads = std::vector<std::thread*>();
    }
    ~Controller(){
        this->clear();
    }
    void init(InitData init_data);
    void run();
};

#endif