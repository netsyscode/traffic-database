#ifndef INDEX_HPP_
#define INDEX_HPP_
#include <string>
#include "skipList.hpp"
#include "flow.hpp"

#define ADDRESS_LEVEL 16
#define PORT_LEVEL 16
#define TIME_LEVEL 16
#define INDEX_NUM 4
#define META_LEN (2 + INDEX_NUM) * sizeof(u_int32_t)

enum KeyMode{
    SRCIP = 0,
    DSTIP,
    SRCPORT,
    DSTPORT,
    KEYNUM,
};

struct IndexReturn{
    string filename;
    list<u_int32_t> packetList;
};

struct FlowIndex{
    u_int8_t flags;
    struct in_addr sourceAddress;
    struct in_addr destinationAddress;
    u_int16_t sourcePort;
    u_int16_t destinationPort;
    u_int64_t startTime;    //required
    u_int64_t endTime;      //required
};

class IndexGenerator{
    SkipList<u_int32_t,u_int32_t>* sourceAddressIndex;
    SkipList<u_int32_t,u_int32_t>* destinationAddressIndex;
    SkipList<u_int16_t,u_int32_t>* sourcePortIndex;
    SkipList<u_int16_t,u_int32_t>* destinationPortIndex;
    // SkipList<u_int64_t,u_int32_t>* startTimeIndex;
    // SkipList<u_int64_t,u_int32_t>* endTimeIndex;
    char* buffer;
    u_int32_t buffer_len;
    u_int32_t offset;
    string filename;
    u_int64_t startTime;
    u_int64_t endTime;

    void intersect(list<u_int32_t>& la, list<u_int32_t>& lb);
    //list<u_int32_t> readPacketOffset(u_int32_t index);
public:
    IndexGenerator(u_int32_t buffer_len, string filename){
        this->sourceAddressIndex = new SkipList<u_int32_t,u_int32_t>(ADDRESS_LEVEL);
        this->destinationAddressIndex = new SkipList<u_int32_t,u_int32_t>(ADDRESS_LEVEL);
        this->sourcePortIndex = new SkipList<u_int16_t,u_int32_t>(PORT_LEVEL);
        this->destinationPortIndex = new SkipList<u_int16_t,u_int32_t>(PORT_LEVEL);
        // this->startTimeIndex = new SkipList<u_int64_t, u_int32_t>(TIME_LEVEL);
        // this->endTimeIndex = new SkipList<u_int64_t, u_int32_t>(TIME_LEVEL);
        this->buffer = new char[buffer_len];
        this->buffer_len = buffer_len;
        this->offset = 0;
        this->filename = filename;
        this->startTime = 0;
        this->endTime = 0;
    }
    ~IndexGenerator(){ 
        delete this->sourceAddressIndex;
        this->sourceAddressIndex = nullptr;
        delete this->destinationAddressIndex;
        this->destinationAddressIndex = nullptr;
        delete this->sourcePortIndex;
        this->sourcePortIndex = nullptr;
        delete this->destinationPortIndex;
        this->destinationPortIndex = nullptr;
        // delete this->startTimeIndex;
        // this->startTimeIndex = nullptr;
        // delete this->endTimeIndex;
        // this->endTimeIndex = nullptr;
        delete [] this->buffer;
        this->buffer = nullptr;
    }
    string getFilenName();
    bool putFlow(const Flow* flow);
    //bool putFlowList(list<Flow*>* flow_list);
    list<u_int32_t> getPacketOffsetByIndex(struct FlowIndex& index);
    list<u_int32_t> getPacketOffsetByRange(struct FlowIndex& startIndex, struct FlowIndex& endIndex);
    list<IndexReturn> getPacketOffsetByIndex(KeyMode mode, u_int64_t key);
    list<IndexReturn> getPacketOffsetByRange(KeyMode mode, u_int64_t startKey, u_int64_t endKey);
    char* outputToStorage();
    void readStorage(char* storage_buffer);
    u_int64_t getStartTime();
    u_int64_t getEndTime();
    void clear();
};

#endif