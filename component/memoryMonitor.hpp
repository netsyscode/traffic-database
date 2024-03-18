#ifndef MEMORYMONITOR_HPP_
#define MEMORYMONITOR_HPP_
#include <iostream>
#include <thread>
#include <vector>
#include <condition_variable>
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

struct TruncateGroup{
    // delayed truncate
    ShareBuffer* oldPacketBuffer;
    ArrayList<u_int32_t>* oldPacketPointer;
    ShareBuffer* newPacketBuffer;
    ArrayList<u_int32_t>* newPacketPointer;

    // monitor util stop
    std::vector<RingBuffer*>* flowMetaIndexBuffers;
    std::vector<SkipList*>* flowMetaIndexCaches;
    std::vector<std::vector<IndexGenerator*>>* flowMetaIndexGenerators;
    std::vector<std::vector<ThreadPointer*>>* flowMetaIndexGeneratorPointers;
    std::vector<std::vector<std::thread*>>* flowMetaIndexGeneratorThreads;
};

class MemoryMonitor{
    const u_int32_t memoryPoolSize = 3;
    const std::vector<u_int32_t> flowMetaEleLens = {4, 4, 2, 2};

    InitData init_data;

    std::atomic_bool stop;
    u_int32_t threadId;
    std::condition_variable* trace_catcher_cv;
    std::condition_variable* packet_aggregator_cv;
    std::mutex mutex;

    // shared memory pool
    std::vector<MemoryGroup> memoryPool; // 0 for use

    // static component
    PcapReader* traceCatcher;
    std::vector<PacketAggregator*> packetAggregators;
    std::vector<ThreadPointer*> packetAggregatorPointers;
    std::thread* traceCatcherThread;
    std::vector<std::thread*> packetAggregatorThreads;
    

    // dynamic component
    std::vector<std::vector<IndexGenerator*>>* flowMetaIndexGenerators;
    std::vector<std::vector<ThreadPointer*>>* flowMetaIndexGeneratorPointers;
    std::vector<std::vector<std::thread*>>* flowMetaIndexGeneratorThreads;

    // pipe to storage
    RingBuffer* truncatePipe;

    ShareBuffer* makePacketBuffer(u_int32_t maxLength, u_int32_t warningLength);
    ArrayList<u_int32_t>* makePacketPointer(u_int32_t maxLength, u_int32_t warningLength);
    std::vector<RingBuffer*>* makeFlowMetaIndexBuffers(u_int32_t capacity, const std::vector<u_int32_t>& ele_lens);
    std::vector<SkipList*>* makeFlowMetaIndexCaches(const std::vector<u_int32_t>& ele_lens);

    void makeTraceCatcher(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename);
    void pushPacketAggregatorInit(u_int32_t eth_header_len);
    void allocateID();

    // void pushFlowMetaIndexGenerator(const std::vector<u_int32_t>& ele_lens);
    void pushFlowMetaIndexGeneratorInit(const std::vector<u_int32_t>& ele_lens);

    void putTruncateGroupToPipe(TruncateGroup group);

    void makeMemoryPool();
    void makeStaticComponent();
    void makeDynamicComponent();

    void staticThreadsRun();
    void dynamicThreadsRun();

    void truncate();
    void threadsRun();
    void monitor();

    void threadsStop();
    void threadsClear();
    void memoryClear();
public:
    MemoryMonitor(RingBuffer* truncatePipe, u_int32_t threadId){
        this->stop = true;
        this->threadId = threadId;
        this->trace_catcher_cv = new std::condition_variable();
        this->packet_aggregator_cv = new std::condition_variable();

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