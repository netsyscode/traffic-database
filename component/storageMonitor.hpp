#ifndef STORAGEMONITOR_HPP_
#define STORAGEMONITOR_HPP_
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
#include "storageOperator.hpp"


class StorageMonitor{

    RingBuffer* truncatePipe;

    std::vector<TruncateGroup> truncatedMemory;

    std::vector<StorageOperator*> storageOperators;
    std::vector<std::thread*> storageThread;

    void storageThreadRun(TruncateGroup group);

    void monitor();

    void threadStop();
    void threadClear();
    void memoryClear();

public:
    StorageMonitor(RingBuffer* truncatePipe){
        this->truncatePipe = truncatePipe;
        this->truncatedMemory = std::vector<TruncateGroup>();
        this->storageThread = std::vector<std::thread*>();
    }
    ~StorageMonitor(){
        this->threadStop();
        this->threadClear();
        this->memoryClear();
    }
    void run();
    void asynchronousStop();
};

#endif