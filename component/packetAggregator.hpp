#ifndef PACKETAGGREGATOR_HPP_
#define PACKETAGGREGATOR_HPP_
#include "../lib/shareBuffer.hpp"
#include "../lib/arrayList.hpp"
#include <unordered_map>
#include <unordered_set>

struct FlowMetadata{
    struct in_addr sourceAddress;
    struct in_addr destinationAddress;
    u_int16_t sourcePort;
    u_int16_t destinationPort;
    struct hash {
        size_t operator()(const FlowMetadata& f) const {
            // 通过将 x 和 y 进行哈希运算得到哈希值
            // 这只是一个简单的示例，您可以根据自己的需求编写更复杂的哈希函数
            return std::hash<u_int16_t>()(f.sourcePort) ^ std::hash<u_int16_t>()(f.destinationPort) ^
                   std::hash<u_int32_t>()(ntohl(f.sourceAddress.s_addr)) ^ std::hash<u_int32_t>()(ntohl(f.destinationAddress.s_addr));
        }
    };
    bool operator==(const FlowMetadata& f) const {
        return ntohl(sourceAddress.s_addr) == ntohl(f.sourceAddress.s_addr) && ntohl(destinationAddress.s_addr) == ntohl(f.destinationAddress.s_addr) &&
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

    // thread member
    u_int32_t readPos;
    u_int32_t threadID;
    std::atomic_bool stop;

    u_int32_t readFromPacketPointer();
    char* readFromPacketBuffer(u_int32_t offset);
    FlowMetadata parsePacket(char* packet);
    //return last packet, max for first
    u_int32_t addPacketToMap(FlowMetadata meta, u_int32_t offset);
    bool writeNextToPacketPointer(u_int32_t last, u_int32_t now);
public:
    PacketAggregator(u_int32_t eth_header_len, ShareBuffer* packetBuffer, ArrayList<u_int32_t>* packetPointer):eth_header_len(eth_header_len){
        this->aggMap = std::unordered_map<FlowMetadata, Flow, FlowMetadata::hash>();
        this->IDSet = std::unordered_set<u_int8_t>();
        this->packetBuffer = packetBuffer;
        this->packetPointer = packetPointer;
        this->threadID = std::numeric_limits<uint32_t>::max();
        this->readPos = 0;
        this->stop = true;
    }
    ~PacketAggregator(){}
    void setThreadID(u_int32_t threadID);
    void addID(u_int8_t id);
    void ereaseID(u_int8_t id);
    void run();
    void asynchronousStop();
};

#endif