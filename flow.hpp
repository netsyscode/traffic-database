#ifndef FLOW_HPP_
#define FLOW_HPP_
#include <iostream>
#include <list>
#include <unordered_map>
#include <pcap.h>
#include <string>
#include "extractor.hpp"
using namespace std;

enum FlowMode{
    MANUAL = 0,
    TIME,
    PACKETNUM,
    BYTENUM,
    MODECOUNT,
};

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

struct FlowData{
    list<u_int32_t> packet_index;
    u_int32_t start_ts_h;
    u_int32_t start_ts_l;
    u_int32_t end_ts_h;
    u_int32_t end_ts_l;
    u_int32_t packet_num;
    u_int64_t byte_num;//unuse
};

struct Flow{
    string filename;//unuse
    FlowMetadata meta;
    FlowData data;
};

class FlowAggregator{
    string filename;
    unordered_map<struct FlowMetadata,struct FlowData, FlowMetadata::hash> aggMap;
    list<Flow*>* nowFlow;
    enum FlowMode mode;
    void checkNowFlow();
    void truncateCheck();
public:
    void clearFlowList(list<Flow*>* flow_list);
    FlowAggregator(){
        this->aggMap = unordered_map<struct FlowMetadata,struct FlowData, FlowMetadata::hash>();
        this->nowFlow = nullptr;
        this->mode = FlowMode::MANUAL;
    }
    FlowAggregator(string filename, enum FlowMode mode){
        this->filename = filename;
        this->aggMap = unordered_map<struct FlowMetadata,struct FlowData, FlowMetadata::hash>();
        this->nowFlow = nullptr;
        this->mode = mode;
    }
    ~FlowAggregator(){
        if(this->nowFlow!=nullptr){
            this->clearFlowList(this->nowFlow);
            this->nowFlow = nullptr;
        }
    }
    void changeFileName(string filename);
    bool isEmpty();
    bool hasFlow();
    void putPacket(struct PacketMetadata* pkt_meta);
    // void truncateByTime(u_int32_t ts_h, u_int32_t ts_l);
    // void truncateByPacketNum(u_int32_t packet_num);
    // void truncateByByteNum(u_int64_t byte_num);
    void truncateAll();
    list<Flow*>* outputByNow();
};

#endif