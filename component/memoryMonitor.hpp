#ifndef MEMORYMONITOR_HPP_
#define MEMORYMONITOR_HPP_
#include <iostream>
#include <thread>
#include <vector>
#include <list>
#include "../lib/arrayList.hpp"
#include "../lib/shareBuffer.hpp"
#include "../lib/ringBuffer.hpp"
#include "../lib/util.hpp"
#include "pcapReader.hpp"
#include "packetAggregator.hpp"
#include "indexGenerator.hpp"

struct MemoryGroup{
    ShareBuffer* packetBuffer;
    ArrayList<u_int32_t>* packetPointer;
    std::vector<RingBuffer*>* flowMetaIndexBuffers;
    std::vector<SkipList*>* flowMetaIndexCaches;
};

class MemoryMonitor{
    const u_int32_t memoryPoolSize = 3;

    // shared memory pool
    std::vector<MemoryGroup> memoryPool; // 0 for use

    // static component
    PcapReader* traceCatcher;
    std::vector<PacketAggregator*> packetAggregators;
    std::vector<ThreadPointer*> packetAggregatorPointers;

    // dynamic component
    std::vector<std::vector<IndexGenerator*>>* flowMetaIndexGenerators;
    std::vector<std::vector<ThreadPointer*>>* flowMetaIndexGeneratorPointers;
    std::vector<std::vector<std::thread*>>* flowMetaIndexGeneratorThreads;

    // pipe to storage
    RingBuffer* truncatePipe;

    void makePacketBuffer(u_int32_t maxLength, u_int32_t warningLength);
    void makePacketPointer(u_int32_t maxLength, u_int32_t warningLength);
    void makeFlowMetaIndexBuffers(u_int32_t capacity, const std::vector<u_int32_t>& ele_lens);
    bool makeFlowMetaIndexCaches(const std::vector<u_int32_t>& ele_lens);

    void makeTraceCatcher(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename);
    void pushPacketAggregatorInit(u_int32_t eth_header_len);
    void allocateID();

    void putTruncateGroupToPipe(TruncateGroup group);

    void makeMemoryPool();
    void makeStaticComponent();
    void makeDynamicComponent();

    void truncate();
    void threadsRun();
    void monitor();

    void threadsStop();
    void threadsClear();
    void memoryClear();
public:
    MemoryMonitor(RingBuffer* truncatePipe){
        this->memoryPool = std::vector<MemoryGroup>();

        this->traceCatcher = nullptr;
        this->packetAggregators = std::vector<PacketAggregator*>();
        this->packetAggregatorPointers = std::vector<ThreadPointer*>();

        this->flowMetaIndexGenerators = nullptr;
        this->flowMetaIndexGeneratorPointers = nullptr;
        this->flowMetaIndexGeneratorThreads = nullptr;

        this->truncatePipe = truncatePipe;
    }
    ~MemoryMonitor(){
        this->threadsStop();
        this->threadsClear();
        this->memoryClear();
    }
    void init(InitData init_data);
    void run();
    void asynchronousStop();
};

#endif