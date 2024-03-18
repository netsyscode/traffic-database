#ifndef CONTROLLER_HPP_
#define CONTROLLER_HPP_
#include <iostream>
#include <thread>
#include <vector>
#include "../lib/arrayList.hpp"
#include "../lib/shareBuffer.hpp"
#include "../lib/ringBuffer.hpp"
// #include "pcapReader.hpp"
// #include "packetAggregator.hpp"
// #include "indexGenerator.hpp"
#include "querier.hpp"
#include "memoryMonitor.hpp"
#include "storageMonitor.hpp"

// struct OutputData{
//     ShareBuffer* packetBuffer;
//     ArrayList<u_int32_t>* packetPointer;
//     std::vector<RingBuffer*>* flowMetaIndexBuffers;
//     std::vector<SkipList*>* flowMetaIndexCaches;
//     std::vector<u_int32_t> flowMetaEleLens;
// };

class MultiThreadController{
    const std::vector<u_int32_t> flowMetaEleLens = {4, 4, 2, 2};
    std::string pcapHeader;
    const u_int32_t capacity = 64;


    // //memory
    // ShareBuffer* packetBuffer;
    // ArrayList<u_int32_t>* packetPointer;
    // std::vector<RingBuffer*>* flowMetaIndexBuffers;
    // std::vector<SkipList*>* flowMetaIndexCaches; // SkipList*

    // //component
    // PcapReader* traceCatcher;
    // std::vector<PacketAggregator*> packetAggregators;
    // std::vector<ThreadPointer*> packetAggregatorPointers;
    // std::vector<std::vector<IndexGenerator*>> flowMetaIndexGenerators;
    // std::vector<std::vector<ThreadPointer*>> flowMetaIndexGeneratorPointers;

    RingBuffer* memoryStoragePipe;
    std::vector<StorageMeta>* storageMetas;

    Querier* querier;
    MemoryMonitor* memoryMonitor;
    ThreadPointer* memoryMonitorPointer;
    StorageMonitor* storageMonitor;
    ThreadPointer* storageMonitorPointer;

    std::thread* queryThread;
    std::thread* memoryMonitorThread;
    std::thread* storageMonitorThread;
    

    //thread
    // std::thread* traceCatcherThread;
    // std::vector<std::thread*> packetAggregatorThreads;
    // std::vector<std::vector<std::thread*>> flowMetaIndexGeneratorThreads;
    

    // void makePacketBuffer(u_int32_t maxLength, u_int32_t warningLength);
    // void makePacketPointer(u_int32_t maxLength, u_int32_t warningLength);
    // void makeFlowMetaIndexBuffers(u_int32_t capacity, const std::vector<u_int32_t>& ele_lens);
    // bool makeFlowMetaIndexCaches(const std::vector<u_int32_t>& ele_lens);

    // void makeTraceCatcher(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename);
    // void pushPacketAggregatorInit(u_int32_t eth_header_len);
    // void pushFlowMetaIndexGeneratorInit();
    // void allocateID();

    

    void makeMemoryStoragePipe();
    void makeStorageMetas();
    void makeMemoryMonitor();
    void makeStorageMonitor();
    void makeQuerier();

    // run as a new thread
    void memoryMonitorThreadRun();
    void storageMonitorThreadRun();
    void threadsRun();
    void queryThreadRun();

    // void pushPacketAggregatorRunning(u_int32_t eth_header_len);
    // void popPacketAggregatorRunning();

    void threadsStop();
    void threadsClear();
    void queryThreadStop();
public:
    MultiThreadController(){
        this->pcapHeader = std::string();
        this->memoryStoragePipe = nullptr;
        this->memoryMonitor = nullptr;
        this->memoryMonitorPointer = nullptr;
        this->storageMonitor = nullptr;
        this->storageMonitorPointer = nullptr;
        this->memoryMonitorThread = nullptr;
        this->storageMonitorThread = nullptr;

        // this->packetBuffer = nullptr;
        // this->packetPointer = nullptr;
        // this->flowMetaIndexBuffers = nullptr;
        // this->flowMetaIndexCaches = nullptr;
        // // this->flowMetaIndexCaches = std::vector<SkipList*>();

        // this->traceCatcher = nullptr;
        // this->packetAggregators = std::vector<PacketAggregator*>();
        // this->packetAggregatorPointers = std::vector<ThreadPointer*>();
        // this->flowMetaIndexGenerators = std::vector<std::vector<IndexGenerator*>>();
        // this->flowMetaIndexGeneratorPointers = std::vector<std::vector<ThreadPointer*>>();
        // this->querier = nullptr;

        // this->traceCatcherThread = nullptr;
        // this->packetAggregatorThreads = std::vector<std::thread*>();
        // this->flowMetaIndexGeneratorThreads = std::vector<std::vector<std::thread*>>();
        // this->queryThread = nullptr;
    }
    ~MultiThreadController(){
        //thread
        this->threadsStop();

        //component
        this->threadsClear();

        if(this->memoryStoragePipe != nullptr){
            delete this->memoryStoragePipe;
        }

        //memory
        // if(this->packetBuffer!=nullptr){
        //     delete this->packetBuffer;
        // }

        // if(this->packetPointer!=nullptr){
        //     delete this->packetPointer;
        // }

        // if(this->flowMetaIndexCaches!=nullptr){
        //     for(auto ic:*(this->flowMetaIndexCaches)){
        //         if(ic==nullptr){
        //             continue;
        //         }
        //         delete ic;
        //     }
        //     this->flowMetaIndexCaches->clear();
        //     delete this->flowMetaIndexCaches;
        // }

        // if(this->flowMetaIndexBuffers!=nullptr){
        //     for(auto rb:*(this->flowMetaIndexBuffers)){
        //         delete rb;
        //     }
        //     this->flowMetaIndexBuffers->clear();
        //     delete this->flowMetaIndexBuffers;
        // }
    }
    void init(InitData init_data);
    void run();
    // just output shared memory for test, the pointers in OutputData should NOT be modified outside Controller!
    // const OutputData outputForTest();
};

#endif