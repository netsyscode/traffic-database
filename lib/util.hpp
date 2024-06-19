#ifndef UTIL_HPP_
#define UTIL_HPP_
#include <cstdlib>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>

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
    u_int32_t pcapReaderThreadCount;
    std::string pcap_header;
};

struct PacketPointerData{
    std::string pcap;
    u_int32_t file_id;
    u_int64_t offset;
};

#endif