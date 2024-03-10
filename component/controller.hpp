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

struct InitData{
    u_int32_t buffer_len;
    u_int32_t packet_num;
    u_int32_t buffer_warn;
    u_int32_t packet_warn;
    u_int32_t pcap_header_len;
    u_int32_t eth_header_len;
    std::string filename;
    u_int32_t flow_capacity;
    u_int32_t threadCount;
};

struct OutputData{
    ShareBuffer* packetBuffer;
    ArrayList<u_int32_t>* packetPointer;
    std::vector<RingBuffer*>* flowMetaIndexBuffers;
};

class MultiThreadController{
    //memory
    ShareBuffer* packetBuffer;
    ArrayList<u_int32_t>* packetPointer;
    std::vector<RingBuffer*>* flowMetaIndexBuffers;

    //component
    PcapReader* traceCatcher;
    std::vector<PacketAggregator*> packetAggregators;
    std::vector<ThreadReadPointer*> threadReadPointers;

    //thread
    std::thread* traceCatcherThread;
    std::vector<std::thread*> packetAggregatorThreads;

    void makePacketBuffer(u_int32_t maxLength, u_int32_t warningLength);
    void makePacketPointer(u_int32_t maxLength, u_int32_t warningLength);
    void makeFlowMetaIndexBuffers(u_int32_t capacity, std::vector<u_int32_t> ele_lens);
    void makeTraceCatcher(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename);
    void pushPacketAggregatorInit(u_int32_t eth_header_len);
    void allocateID();
    void threadsRun();

    void pushPacketAggregatorRunning(u_int32_t eth_header_len);
    void popPacketAggregatorRunning();

    void threadsStop();
    void threadsClear();
public:
    MultiThreadController(){
        this->packetBuffer = nullptr;
        this->packetPointer = nullptr;
        this->flowMetaIndexBuffers = nullptr;
        this->traceCatcher = nullptr;
        this->packetAggregators = std::vector<PacketAggregator*>();
        this->traceCatcherThread = nullptr;
        this->packetAggregatorThreads = std::vector<std::thread*>();
    }
    ~MultiThreadController(){
        if(this->traceCatcherThread!=nullptr){
            this->traceCatcher->asynchronousStop();
            this->traceCatcherThread->join();
            delete this->traceCatcherThread;
        }
        for(int i=0;i<this->packetAggregatorThreads.size();++i){
            this->packetAggregators[i]->asynchronousStop();
            this->packetPointer->asynchronousStop(this->threadReadPointers[i]->id);
            for(auto rb:*(this->flowMetaIndexBuffers)){
                rb->asynchronousStop(this->threadReadPointers[i]->id);
            }
            this->packetAggregatorThreads[i]->join();
            delete this->packetAggregatorThreads[i];
        }
        this->packetAggregatorThreads.clear();

        for(auto t:this->threadReadPointers){
            this->packetPointer->ereaseReadThread(t);
            for(auto rb:*(this->flowMetaIndexBuffers)){
                rb->ereaseWriteThread(t);
            }
            delete t;
        }
        this->threadReadPointers.clear();
        for(auto p:this->packetAggregators){
            delete p;
        }
        this->packetAggregators.clear();

        if(this->traceCatcher!=nullptr){
            this->packetPointer->ereaseWriteThread();
            delete this->traceCatcher;
            this->traceCatcher = nullptr;
        }

        if(this->packetBuffer!=nullptr){
            delete this->packetBuffer;
        }
        if(this->packetPointer!=nullptr){
            delete this->packetPointer;
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