#ifndef STORAGEOPERATOR_HPP_
#define STORAGEOPERATOR_HPP_
#include <iostream>
#include <string>
#include "../lib/arrayList.hpp"
#include "../lib/shareBuffer.hpp"
#include "../lib/ringBuffer.hpp"
#include "../lib/skipList.hpp"
#include "../lib/util.hpp"

struct StorageGroup{
    ShareBuffer* oldPacketBuffer;
    ArrayList<u_int32_t>* oldPacketPointer;
    ShareBuffer* newPacketBuffer;
    ArrayList<u_int32_t>* newPacketPointer;
    std::vector<SkipList*>* flowMetaIndexCaches;
};

class StorageOperator{
    StorageGroup group;
public:
    StorageOperator(StorageGroup group){
        this->group = group;
    }
    ~StorageOperator(){
        delete this->group.oldPacketBuffer;
        delete this->group.oldPacketPointer;
        for(auto ic:*(this->group.flowMetaIndexCaches)){
            if(ic!=nullptr){
                delete ic;
            }
        }
        this->group.flowMetaIndexCaches->clear();
        delete this->group.flowMetaIndexCaches;
    }
    void run();
};

#endif