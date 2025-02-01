#ifndef DPDKREADER_HPP_
#define DPDKREADER_HPP_
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <atomic>

#include "../dpdk_lib/memoryBuffer.hpp"
#include "../dpdk_lib/header.hpp"
#include "../dpdk_lib/util.hpp"
#include "../dpdk_lib/dpdk.hpp"
#include "../dpdk_lib/pointerRingBuffer.hpp"
#include "../dpdk_lib/packetAggregator.hpp"
#include "../dpdk_lib/tagAggregator.hpp"

#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_malloc.h>

#include <stdint.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

// #define RX_RING_SIZE 512
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 512
#define BURST_SIZE 32
#define MAX_TAG_NUM 9
#define MAX_TAG_TYPE 16

struct Tag{
	u_int8_t id:4,
       agg:4;
	uint8_t length;
	uint16_t offset;
};

struct PacketMeta{
    array_list_header* header;
    const char* data;
    u_int32_t len;
    u_int8_t tag_num;
    Tag* tags;
};

// read packet from pcap, equivalent like extractor, lower substitution of trace catcher
class DPDKReader{
    const u_int32_t pcap_header_len;
    const u_int32_t eth_header_len;

    // std::string filename;
    u_int64_t offset;
    u_int64_t capacityUnit;

    // write only
    MemoryBuffer* packetBuffer;
    u_int16_t port_id;
    u_int16_t rx_id;
    std::string fileName;
    DPDK* dpdk;

    PacketAggregator* packetAggregator;
    TagAggregator* tagAggregator;

    // write only
    std::vector<PointerRingBuffer*>* indexRings;
    // PointerRingBuffer* indexRing;

    std::atomic_bool stop;

    u_int64_t duration_time;

    u_int64_t byteLen;

    u_int64_t tagIndexCount;

    //read packet of offset from file;
    void readPacket(struct rte_mbuf *buf,u_int64_t ts,PacketMeta* meta);
    //write data to packet buffer (pay attention to aligning with file offset)
    u_int64_t writePacketToPacketBuffer(PacketMeta& meta);
    FlowMetadata getFlowMetaData(PacketMeta& meta);

    u_int64_t calValue(u_int64_t _offset);

    u_int64_t getField(const char* data, u_int8_t offset, u_int8_t len);

    bool writeIndexToRing(u_int64_t value, FlowMetadata meta, u_int64_t ts);
    bool writeTagToRing(const char* data, Tag* tags, u_int8_t tag_num, FlowMetadata meta, u_int64_t ts, u_int64_t offset, u_int64_t last);
    bool writeAllTagsToRing(u_int64_t ts);
    void bindCore(u_int32_t cpu);

public:
    DPDKReader(u_int32_t pcap_header_len, u_int32_t eth_header_len, DPDK* dpdk, std::vector<PointerRingBuffer*>* rings, u_int16_t port_id, u_int16_t rx_id, u_int64_t capacity, MemoryBuffer* buffer):
    pcap_header_len(pcap_header_len),eth_header_len(eth_header_len),dpdk(dpdk),indexRings(rings),port_id(port_id),rx_id(rx_id),capacityUnit(capacity){
        this->offset = pcap_header_len;
        this->stop = true;
        this->duration_time = 0;
        this->tagIndexCount = 0;
        this->byteLen = 0;
        this->packetBuffer = buffer;
        this->fileName = "./data/input/" + std::to_string(port_id) + "-" + std::to_string(rx_id) + ".pcap";
        this->packetAggregator = new PacketAggregator(capacity, std::numeric_limits<uint64_t>::max());
        this->tagAggregator = new TagAggregator(MAX_TAG_TYPE);
    }
    ~DPDKReader(){
        delete this->packetAggregator;
        delete this->tagAggregator;
    }
    
    int run();
    void asynchronousStop();
    static int launch(void *arg){
        return static_cast<DPDKReader*>(arg)->run();
    }
};

#endif