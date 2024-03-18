#ifndef PACKETAGGREGATOR_HPP_
#define PACKETAGGREGATOR_HPP_
#include "../lib/shareBuffer.hpp"
#include "../lib/arrayList.hpp"
#include "../lib/ringBuffer.hpp"
#include "../lib/util.hpp"
#include <unordered_map>
#include <unordered_set>
#define FLOW_META_NUM 4

struct FlowMetadata{
    std::string sourceAddress;
    std::string destinationAddress;
    u_int16_t sourcePort;
    u_int16_t destinationPort;
    struct hash {
        size_t operator()(const FlowMetadata& f) const {
            return std::hash<u_int16_t>()(f.sourcePort) ^ std::hash<u_int16_t>()(f.destinationPort) ^
                    std::hash<std::string>()(f.sourceAddress) ^ std::hash<std::string>()(f.destinationAddress);
        }
    };
    bool operator==(const FlowMetadata& f) const {
        return sourceAddress == f.sourceAddress && destinationAddress == f.destinationAddress && 
                sourcePort == f.sourcePort && destinationPort == f.destinationPort;
    }
};

struct Flow{
    u_int32_t head;
    u_int32_t tail;
};

class PacketAggregator{
    const u_int32_t eth_header_len;

    //hash table
    std::unordered_map<FlowMetadata, Flow, FlowMetadata::hash> aggMap;
    std::unordered_set<u_int8_t> IDSet;

    // shared memory
    // read only
    ShareBuffer* packetBuffer;
    // read and write to next
    ArrayList<u_int32_t>* packetPointer;
    // write only, default should be srcip(4B), dstip(4B), srcport(2B), dstport(2B)
    std::vector<RingBuffer*>* flowMetaIndexBuffers;

    ShareBuffer* newPacketBuffer;
    ArrayList<u_int32_t>* newPacketPointer;
    std::vector<RingBuffer*>* newFlowMetaIndexBuffers; 

    //for old flows, util "next" of oldPacketPointers are filled
    std::unordered_map<FlowMetadata, Flow, FlowMetadata::hash> oldAggMap;
    ArrayList<u_int32_t>* oldPacketPointer;

    // thread member
    u_int32_t readPos;
    u_int32_t threadID;

    std::atomic_bool stop;
    std::atomic_bool pause;
    ThreadPointer* selfPointer;
    std::condition_variable* monitor_cv;

    u_int32_t readFromPacketPointer();
    std::string readFromPacketBuffer(u_int32_t offset);
    FlowMetadata parsePacket(const char* packet);
    //return <last packet(max for first packet), is_in_old_packet_pointer>
    std::pair<u_int32_t,bool> addPacketToMap(FlowMetadata meta, u_int32_t pos);
    bool writeNextToOldPacketPointer(u_int32_t last, u_int32_t now);
    bool writeNextToPacketPointer(u_int32_t last, u_int32_t now);
    bool writeFlowMetaIndexToIndexBuffer(FlowMetadata meta, u_int32_t pos);

    void truncate();
public:
    PacketAggregator(u_int32_t eth_header_len, ShareBuffer* packetBuffer, ArrayList<u_int32_t>* packetPointer, std::vector<RingBuffer*>* flowMetaIndexBuffers, std::condition_variable* monitor_cv):eth_header_len(eth_header_len){
        this->oldAggMap = std::unordered_map<FlowMetadata, Flow, FlowMetadata::hash>();
        this->oldPacketPointer = nullptr;
        this->aggMap = std::unordered_map<FlowMetadata, Flow, FlowMetadata::hash>();
        this->IDSet = std::unordered_set<u_int8_t>();
        this->packetBuffer = packetBuffer;
        this->packetPointer = packetPointer;
        this->flowMetaIndexBuffers = flowMetaIndexBuffers;
        if(this->flowMetaIndexBuffers->size()!=FLOW_META_NUM){
            std::cout << "Packet aggregator warning: flow meta number not equal." <<std::endl;
        }
        this->threadID = std::numeric_limits<uint32_t>::max();
        this->readPos = 0;
        this->monitor_cv = monitor_cv;
        this->stop = true;
        this->pause = false;
    }
    ~PacketAggregator(){}
    void setThreadID(u_int32_t threadID);
    void addID(u_int8_t id);
    void ereaseID(u_int8_t id);
    void run();
    void asynchronousStop();
    void asynchronousPause(ShareBuffer* newPacketBuffer, ArrayList<u_int32_t>* newPacketPointer, std::vector<RingBuffer*>* newFlowMetaIndexBuffers, ThreadPointer* pointer);
    bool getPause()const;
};

#endif