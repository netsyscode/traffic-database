#ifndef STORAGEMONITOR_HPP_
#define STORAGEMONITOR_HPP_
#include <iostream>
#include <thread>
#include <vector>
#include <list>
#include <chrono>
#include "../lib/arrayList.hpp"
#include "../lib/shareBuffer.hpp"
#include "../lib/ringBuffer.hpp"
#include "../lib/util.hpp"
#include "pcapReader.hpp"
#include "packetAggregator.hpp"
#include "indexGenerator.hpp"
// #include "storageOperator.hpp"
#include "memoryMonitor.hpp"

#define FLOW_META_INDEX_NUM 4

struct StorageMeta{
    u_int32_t index_offset[FLOW_META_INDEX_NUM];
    u_int32_t pointer_offset;
    u_int32_t data_offset;
    u_int32_t index_end[FLOW_META_INDEX_NUM];
    u_int32_t pointer_end;
    u_int32_t data_end;
    u_int64_t time_start;
    u_int64_t time_end;
};


class StorageMonitor{
    const std::string index_name[FLOW_META_INDEX_NUM] = {
        "./data/index/pcap.pcap_srcip_idx",
        "./data/index/pcap.pcap_dstip_idx",
        "./data/index/pcap.pcap_srcport_idx",
        "./data/index/pcap.pcap_dstport_idx",
    };
    const std::string pointer_name = "./data/index/pcap.pcappt";
    const std::string data_name = "./data/index/pcap.pcap";

    u_int32_t index_offset[FLOW_META_INDEX_NUM];
    u_int32_t pointer_offset;
    u_int32_t data_offset;

    u_int32_t pcap_header_len;
    std::string pcap_header;

    u_int32_t threadID;

    RingBuffer* truncatePipe;

    std::vector<TruncateGroup> truncatedMemory;
    std::vector<StorageMeta>* storageMetas;

    u_int64_t duration_time;

    // std::vector<StorageOperator*> storageOperators;
    // std::vector<ThreadPointer*> storageOperatorPointers;
    // std::vector<std::thread*> storageOperatorThread;

    // void storageOperatorThreadRun(TruncateGroup group);

    bool readTruncatePipe();
    void clearTruncateGroup(TruncateGroup& tg);
    void store(TruncateGroup& tg);
    void monitor();

    // void threadStop();
    // void threadClear();
    void runUnit();
    void memoryClear();

public:
    StorageMonitor(RingBuffer* truncatePipe, std::vector<StorageMeta>* storageMetas, u_int32_t threadID){
        this->truncatePipe = truncatePipe;
        this->threadID = threadID;
        this->truncatedMemory = std::vector<TruncateGroup>();
        this->storageMetas = storageMetas;
        this->pcap_header_len = 0;
        // this->storageOperators = std::vector<StorageOperator*>();
        // this->storageOperatorPointers = std::vector<ThreadPointer*>();
        // this->storageOperatorThread = std::vector<std::thread*>();
    }
    ~StorageMonitor(){
        // this->threadStop();
        // this->threadClear();
        this->memoryClear();
    }
    void init(InitData init_data);
    void run();
    // void asynchronousStop();
};

#endif