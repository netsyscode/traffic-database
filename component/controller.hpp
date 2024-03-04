#ifndef CONTROLLER_HPP_
#define CONTROLLER_HPP_
#include <iostream>
#include <thread>
#include <vector>
#include "../lib/arrayList.hpp"
#include "../lib/shareBuffer.hpp"
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
    u_int32_t threadCount;
};

class MultiThreadController{
    //memory
    ShareBuffer* packetBuffer;
    ArrayList<u_int32_t>* packetPointer;

    //component
    PcapReader* traceCatcher;
    std::vector<PacketAggregator*> packetAggregators;
    std::vector<ThreadReadPointer*> threadReadPointers;

    //thread
    std::thread* traceCatcherThread;
    std::vector<std::thread*> packetAggregatorThreads;

    void makePacketBuffer(u_int32_t maxLength, u_int32_t warningLength);
    void makePacketPointer(u_int32_t maxLength, u_int32_t warningLength);
    void makeTraceCatcher(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename);
    void pushPacketAggregatorInit(u_int32_t eth_header_len);
    void allocateID();
    void threadsRun();

    void pushPacketAggregatorRunning(u_int32_t eth_header_len);
    void popPacketAggregatorRunning();
public:
    MultiThreadController(u_int32_t buffer_len,u_int32_t packet_num, u_int32_t buffer_warn, u_int32_t packet_warn, u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename){
        this->packetBuffer = nullptr;
        this->packetPointer = nullptr;
        this->traceCatcher = nullptr;
        this->packetAggregators = std::vector<PacketAggregator*>();
        this->traceCatcherThread = nullptr;
        this->packetAggregatorThreads = std::vector<std::thread*>();
    }
    ~MultiThreadController(){

        if(this->traceCatcherThread!=nullptr){
            this->traceCatcher->asynchronousStop();
            delete this->traceCatcherThread;
        }
        for(int i=0;i<this->packetAggregatorThreads.size();++i){
            this->packetAggregators[i]->asynchronousStop();
            this->packetAggregatorThreads[i]->join();
            delete this->packetAggregatorThreads[i];
        }
        this->packetAggregatorThreads.clear();

        for(auto t:this->threadReadPointers){
            this->packetPointer->ereaseReadThread(t);
            delete t;
        }
        this->packetAggregators.clear();
        this->packetPointer->ereaseWriteThread();
        if(this->traceCatcher!=nullptr){
            delete this->traceCatcher;
        }
        for(auto p:this->packetAggregators){
            delete p;
        }
        this->packetAggregators.clear();

        if(this->packetBuffer!=nullptr){
            delete this->packetBuffer;
        }
        if(this->packetPointer!=nullptr){
            delete this->packetPointer;
        }
    }
    void init(InitData init_data);
    void run();
};

#endif