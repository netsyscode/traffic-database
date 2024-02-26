#ifndef STORAGE_HPP_
#define STORAGE_HPP_
#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include "index.hpp"
using namespace std;

struct StorageMetadata{
    u_int64_t start_time;
    u_int64_t end_time;
    string filename;
    u_int32_t offset;
};

class StorageOperator{
    list<StorageMetadata> storageList;
    //string backupFile;
    string filename;
    u_int32_t fileOffset;
public:
    StorageOperator(string filename){
        this->storageList = list<StorageMetadata>();
        this->filename = filename;
        this->fileOffset = 0;
    }
    ~StorageOperator()=default;
    void changeFile(string filename);
    bool writeIndex(char* buffer);
    bool writeIndex(char* buffer, u_int64_t startTime, u_int64_t endTime);
    char* readIndex(string filename, u_int32_t offset);
    IndexReturn getPacketOffsetByIndex(struct FlowIndex& index, string filename, u_int32_t offset);
    IndexReturn getPacketOffsetByRange(struct FlowIndex& startIndex, struct FlowIndex& endIndex, string filename, u_int32_t offset);
    IndexReturn getPacketOffsetByIndex(KeyMode mode, u_int64_t key, string filename, u_int32_t offset);
    IndexReturn getPacketOffsetByRange(KeyMode mode, u_int64_t startKey, u_int64_t endKey, string filename, u_int32_t offset);
    list<IndexReturn> getPacketOffsetByIndex(struct FlowIndex& index);
    list<IndexReturn> getPacketOffsetByRange(struct FlowIndex& startIndex, struct FlowIndex& endIndex);
    list<IndexReturn> getPacketOffsetByIndex(KeyMode mode, u_int64_t key, u_int64_t startTime, u_int64_t endTime);
    list<IndexReturn> getPacketOffsetByRange(KeyMode mode, u_int64_t startKey, u_int64_t endKey, u_int64_t startTime, u_int64_t endTime);
};

template <typename KeyType>
list<u_int32_t> binarySearch(char* arr, u_int32_t len, KeyType target);
template <typename KeyType>
list<u_int32_t> binarySearchRange(char* arr, u_int32_t len, KeyType begin, KeyType end);
void intersect(list<u_int32_t>& la, list<u_int32_t>& lb);

#endif