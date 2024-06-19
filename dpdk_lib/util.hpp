#ifndef UTIL_HPP_
#define UTIL_HPP_
#include <vector>
#include <iostream>
#include <string>
#include "header.hpp"

#define INDEX_NUM IndexType::TOTAL

enum IndexType{
    SRCIP = 0,
    DSTIP,
    SRCPORT,
    DSTPORT,
    TOTAL,
};

const std::vector<u_int32_t> flowMetaEleLens = {4, 4, 2, 2};

struct PacketMeta{
    char* data;
    u_int32_t len;
};

struct Index{
    std::string key;
    u_int64_t value;
    u_int64_t ts;
};

const std::string index_name[INDEX_NUM] = {
        "./data/index/pcap.pcap_srcip_idx",
        "./data/index/pcap.pcap_dstip_idx",
        "./data/index/pcap.pcap_srcport_idx",
        "./data/index/pcap.pcap_dstport_idx",
    };

// srcip dstip srcport dstport
inline std::vector<Index*> get_index(PacketMeta meta, u_int32_t eth_header_len,u_int64_t value){
    std::vector<Index*> ret = std::vector<Index*>();
    for(int i=0;i<INDEX_NUM;++i){
        Index* index = new Index();
        index->value = value;
        data_header* d_header = (data_header*)(meta.data);
        index->ts = d_header->ts_h;
        index->ts <<= sizeof(u_int32_t)*8;
        index->ts += d_header->ts_l;
        ret.push_back(index);
    }

    ip_header* ip_protocol = (ip_header*)(meta.data + sizeof(struct data_header) + eth_header_len);
    int ip_header_length = ip_protocol->ip_header_length;
    int ip_prot = ip_protocol->ip_protocol;
    u_int32_t srcip = htonl(ip_protocol->ip_source_address);
    u_int32_t dstip = htonl(ip_protocol->ip_destination_address);
    ret[IndexType::SRCIP]->key = std::string((char*)&srcip,4);
    ret[IndexType::DSTIP]->key = std::string((char*)&dstip,4);

    u_int16_t srcport,dstport;

    if(ip_prot == 6){
        tcp_header* tcp_protocol = (tcp_header*)(meta.data + sizeof(struct data_header) + eth_header_len + ip_header_length * 4);
        srcport = htons(tcp_protocol->tcp_source_port);
        dstport = htons(tcp_protocol->tcp_destination_port);
    }else if(ip_prot == 17){
        udp_header* udp_protocol = (udp_header*)(meta.data + sizeof(struct data_header) + eth_header_len + ip_header_length * 4);
        srcport = htons(udp_protocol->udp_source_port);
        dstport = htons(udp_protocol->udp_destination_port);
    }else{
        std::cout << "Packet aggregator warning: parsePacket with unknown protocol:" << ip_prot << "!" << std::endl;
        srcport = 0;
        dstport = 0;
    }

    ret[IndexType::SRCPORT]->key = std::string((char*)&srcport,2);
    ret[IndexType::DSTPORT]->key = std::string((char*)&dstport,2);

    return ret;
}

#endif