#ifndef UTIL_HPP_
#define UTIL_HPP_
#include <cstdlib>

struct ThreadPointer{
    u_int32_t id;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic_bool stop_;
    std::atomic_bool pause_;
};

struct Index{
    std::string key;
    u_int32_t value;
};

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

struct TruncateGroup{
    // delayed truncate
    ShareBuffer* oldPacketBuffer;
    ArrayList<u_int32_t>* oldPacketPointer;
    ShareBuffer* newPacketBuffer;
    ArrayList<u_int32_t>* newPacketPointer;

    // monitor util stop
    std::vector<RingBuffer*>* flowMetaIndexBuffers;
    std::vector<SkipList*>* flowMetaIndexCaches;
    std::vector<std::vector<IndexGenerator*>>* flowMetaIndexGenerators;
    std::vector<std::vector<ThreadPointer*>>* flowMetaIndexGeneratorPointers;
    std::vector<std::vector<std::thread*>>* flowMetaIndexGeneratorThreads;
};

#endif