#ifndef CONTROLLER_HPP_
#define CONTROLLER_HPP_
#include <iostream>
#include <thread>
#include <vector>
#include "../lib/arrayList.hpp"
#include "../lib/shareBuffer.hpp"
#include "../lib/ringBuffer.hpp"
#include "pcapReader.hpp"
#include "packetAggregator.hpp"
#include "indexGenerator.hpp"
#include "querier.hpp"

struct InitData{
    u_int32_t buffer_len;
    u_int32_t packet_num;
    u_int32_t buffer_warn;
    u_int32_t packet_warn;
    // u_int32_t pcap_header_len;
    u_int32_t eth_header_len;
    std::string filename;
    u_int32_t flow_capacity;
    u_int32_t packetAggregatorThreadCount;
    u_int32_t flowMetaIndexGeneratorThreadCountEach;
    std::string pcap_header;
};

struct OutputData{
    ShareBuffer* packetBuffer;
    ArrayList<u_int32_t>* packetPointer;
    std::vector<RingBuffer*>* flowMetaIndexBuffers;
    std::vector<SkipList*>* flowMetaIndexCaches;
    std::vector<u_int32_t> flowMetaEleLens;
};

class MultiThreadController{
    const std::vector<u_int32_t> flowMetaEleLens = {4, 4, 2, 2};
    std::string pcapHeader;

    //memory
    ShareBuffer* packetBuffer;
    ArrayList<u_int32_t>* packetPointer;
    std::vector<RingBuffer*>* flowMetaIndexBuffers;
    std::vector<SkipList*>* flowMetaIndexCaches; // SkipList*

    //component
    PcapReader* traceCatcher;
    std::vector<PacketAggregator*> packetAggregators;
    std::vector<ThreadPointer*> packetAggregatorPointers;
    std::vector<std::vector<IndexGenerator*>> flowMetaIndexGenerators;
    std::vector<std::vector<ThreadPointer*>> flowMetaIndexGeneratorPointers;
    Querier* querier;

    //thread
    std::thread* traceCatcherThread;
    std::vector<std::thread*> packetAggregatorThreads;
    std::vector<std::vector<std::thread*>> flowMetaIndexGeneratorThreads;
    std::thread* queryThread;

    void makePacketBuffer(u_int32_t maxLength, u_int32_t warningLength);
    void makePacketPointer(u_int32_t maxLength, u_int32_t warningLength);
    void makeFlowMetaIndexBuffers(u_int32_t capacity, const std::vector<u_int32_t>& ele_lens);
    bool makeFlowMetaIndexCaches(const std::vector<u_int32_t>& ele_lens);

    void makeTraceCatcher(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename);
    void pushPacketAggregatorInit(u_int32_t eth_header_len);
    void pushFlowMetaIndexGeneratorInit();
    void allocateID();

    void threadsRun();

    void truncate();

    // run as a new thread
    void queryThreadRun();

    void pushPacketAggregatorRunning(u_int32_t eth_header_len);
    void popPacketAggregatorRunning();

    void threadsStop();
    void threadsClear();
public:
    MultiThreadController(){
        this->pcapHeader = std::string();

        this->packetBuffer = nullptr;
        this->packetPointer = nullptr;
        this->flowMetaIndexBuffers = nullptr;
        this->flowMetaIndexCaches = nullptr;
        // this->flowMetaIndexCaches = std::vector<SkipList*>();

        this->traceCatcher = nullptr;
        this->packetAggregators = std::vector<PacketAggregator*>();
        this->packetAggregatorPointers = std::vector<ThreadPointer*>();
        this->flowMetaIndexGenerators = std::vector<std::vector<IndexGenerator*>>();
        this->flowMetaIndexGeneratorPointers = std::vector<std::vector<ThreadPointer*>>();
        this->querier = nullptr;

        this->traceCatcherThread = nullptr;
        this->packetAggregatorThreads = std::vector<std::thread*>();
        this->flowMetaIndexGeneratorThreads = std::vector<std::vector<std::thread*>>();
        this->queryThread = nullptr;
    }
    ~MultiThreadController(){
        //thread
        this->threadsStop();

        //component
        this->threadsClear();

        //memory
        if(this->packetBuffer!=nullptr){
            delete this->packetBuffer;
        }

        if(this->packetPointer!=nullptr){
            delete this->packetPointer;
        }

        if(this->flowMetaIndexCaches!=nullptr){
            for(auto ic:*(this->flowMetaIndexCaches)){
                if(ic==nullptr){
                    continue;
                }
                delete ic;
            }
            this->flowMetaIndexCaches->clear();
            delete this->flowMetaIndexCaches;
        }

        if(this->flowMetaIndexBuffers!=nullptr){
            for(auto rb:*(this->flowMetaIndexBuffers)){
                delete rb;
            }
            this->flowMetaIndexBuffers->clear();
            delete this->flowMetaIndexBuffers;
        }
    }
    void init(InitData init_data);
    void run();
    // just output shared memory for test, the pointers in OutputData should NOT be modified outside Controller!
    const OutputData outputForTest();
};

#endif