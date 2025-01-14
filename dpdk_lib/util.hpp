#ifndef UTIL_HPP_
#define UTIL_HPP_
#include <vector>
#include <iostream>
#include <string>
#include "header.hpp"
#include "packetAggregator.hpp"

#define INDEX_NUM IndexType::TOTAL

enum IndexType{
    SRCIP = 0,
    DSTIP,
    SRCPORT,
    DSTPORT,
    SRCIPv6,
    DSTIPv6,
    QUARTURPLEIPv4,
    QUARTURPLEIPv6,
    TOTAL,
};

struct IPv6Address{
    u_int64_t low;
    u_int64_t high;

    bool operator<(const IPv6Address& other) const {
        if (high < other.high) {
            return true;
        } else if (high > other.high) {
            return false;
        }
        // 如果高位相等，比较低位
        return low < other.low;
    }
    // 重载比较运算符 ">"
    bool operator>(const IPv6Address& other) const {
        if (high > other.high) {
            return true;
        } else if (high < other.high) {
            return false;
        }
        // 如果高位相等，比较低位
        return low > other.low;
    }
    // 重载比较运算符 "=="
    bool operator==(const IPv6Address& other) const {
        return high == other.high && low == other.low;
    }
    // 重载比较运算符 "!="
    bool operator!=(const IPv6Address& other) const {
        return !(*this == other);
    }
    // 重载比较运算符 "<="
    bool operator<=(const IPv6Address& other) const {
        return !(*this > other);
    }
    // 重载比较运算符 ">="
    bool operator>=(const IPv6Address& other) const {
        return !(*this < other);
    }
};

struct QuarTurpleIPv4{
    u_int16_t dstport;
    u_int16_t srcport;
    u_int32_t dstip;
    u_int32_t srcip;

    bool operator<(const QuarTurpleIPv4& other) const {
        if (srcip < other.srcip) {
            return true;
        } else if (srcip > other.srcip) {
            return false;
        }

        if (dstip < other.dstip) {
            return true;
        } else if (dstip > other.dstip) {
            return false;
        }

        if (srcport < other.srcport) {
            return true;
        } else if (srcport > other.srcport) {
            return false;
        }
        // 如果高位相等，比较低位
        return dstport < other.dstport;
    }
    // 重载比较运算符 ">"
     bool operator>(const QuarTurpleIPv4& other) const {
        if (srcip > other.srcip) {
            return true;
        } else if (srcip < other.srcip) {
            return false;
        }

        if (dstip > other.dstip) {
            return true;
        } else if (dstip < other.dstip) {
            return false;
        }

        if (srcport > other.srcport) {
            return true;
        } else if (srcport < other.srcport) {
            return false;
        }
        // 如果高位相等，比较低位
        return dstport > other.dstport;
    }
    // 重载比较运算符 "=="
    bool operator==(const QuarTurpleIPv4& other) const {
        return srcip == other.srcip && dstip == other.dstip && srcport == other.srcport && dstport == other.dstport;
    }
    // 重载比较运算符 "!="
    bool operator!=(const QuarTurpleIPv4& other) const {
        return !(*this == other);
    }
    // 重载比较运算符 "<="
    bool operator<=(const QuarTurpleIPv4& other) const {
        return !(*this > other);
    }
    // 重载比较运算符 ">="
    bool operator>=(const QuarTurpleIPv4& other) const {
        return !(*this < other);
    }
};

struct QuarTurpleIPv6{
    u_int16_t dstport;
    u_int16_t srcport;
    IPv6Address dstip;
    IPv6Address srcip;

    bool operator<(const QuarTurpleIPv6& other) const {
        if (srcip < other.srcip) {
            return true;
        } else if (srcip > other.srcip) {
            return false;
        }

        if (dstip < other.dstip) {
            return true;
        } else if (dstip > other.dstip) {
            return false;
        }

        if (srcport < other.srcport) {
            return true;
        } else if (srcport > other.srcport) {
            return false;
        }
        // 如果高位相等，比较低位
        return dstport < other.dstport;
    }
    // 重载比较运算符 ">"
     bool operator>(const QuarTurpleIPv6& other) const {
        if (srcip > other.srcip) {
            return true;
        } else if (srcip < other.srcip) {
            return false;
        }

        if (dstip > other.dstip) {
            return true;
        } else if (dstip < other.dstip) {
            return false;
        }

        if (srcport > other.srcport) {
            return true;
        } else if (srcport < other.srcport) {
            return false;
        }
        // 如果高位相等，比较低位
        return dstport > other.dstport;
    }
    // 重载比较运算符 "=="
    bool operator==(const QuarTurpleIPv6& other) const {
        return srcip == other.srcip && dstip == other.dstip && srcport == other.srcport && dstport == other.dstport;
    }
    // 重载比较运算符 "!="
    bool operator!=(const QuarTurpleIPv6& other) const {
        return !(*this == other);
    }
    // 重载比较运算符 "<="
    bool operator<=(const QuarTurpleIPv6& other) const {
        return !(*this > other);
    }
    // 重载比较运算符 ">="
    bool operator>=(const QuarTurpleIPv6& other) const {
        return !(*this < other);
    }
};

const std::vector<u_int32_t> flowMetaEleLens = {sizeof(u_int32_t), sizeof(u_int32_t), sizeof(u_int16_t), sizeof(u_int16_t), sizeof(IPv6Address), sizeof(IPv6Address),sizeof(QuarTurpleIPv4),sizeof(QuarTurpleIPv6)};

struct IndexTMP{
    FlowMetadata meta;
    u_int64_t ts;
    u_int64_t value;
};

struct Index{
    std::string key;
    u_int64_t value;
    u_int64_t ts;
    u_int8_t id;
    u_int8_t len;
};

struct HilbertMeta{
    Index* index;
    std::string hilbertValue;
};

const std::string index_name[INDEX_NUM] = {
    "./data/index/pcap.pcap_srcip_idx",
    "./data/index/pcap.pcap_dstip_idx",
    "./data/index/pcap.pcap_srcport_idx",
    "./data/index/pcap.pcap_dstport_idx",
};

// srcip dstip srcport dstport
// inline std::vector<Index*> get_index(PacketMeta meta, u_int32_t eth_header_len,u_int64_t value){
//     std::vector<Index*> ret = std::vector<Index*>();
//     for(int i=0;i<INDEX_NUM;++i){
//         Index* index = new Index();
//         index->value = value;
//         // data_header* d_header = (data_header*)(meta.data);
//         data_header* d_header = meta.header;
//         index->ts = d_header->ts_h;
//         index->ts <<= sizeof(u_int32_t)*8;
//         index->ts += d_header->ts_l;
//         ret.push_back(index);
//     }

//     ip_header* ip_protocol = (ip_header*)(meta.data + eth_header_len);
//     int ip_header_length = ip_protocol->ip_header_length;
//     int ip_prot = ip_protocol->ip_protocol;
//     u_int32_t srcip = htonl(ip_protocol->ip_source_address);
//     u_int32_t dstip = htonl(ip_protocol->ip_destination_address);
//     ret[IndexType::SRCIP]->key = std::string((char*)&srcip,4);
//     ret[IndexType::DSTIP]->key = std::string((char*)&dstip,4);

//     u_int16_t srcport,dstport;

//     if(ip_prot == 6){
//         tcp_header* tcp_protocol = (tcp_header*)(meta.data + eth_header_len + ip_header_length * 4);
//         srcport = htons(tcp_protocol->tcp_source_port);
//         dstport = htons(tcp_protocol->tcp_destination_port);
//     }else if(ip_prot == 17){
//         udp_header* udp_protocol = (udp_header*)(meta.data + eth_header_len + ip_header_length * 4);
//         srcport = htons(udp_protocol->udp_source_port);
//         dstport = htons(udp_protocol->udp_destination_port);
//     }else{
//         std::cout << "Packet aggregator warning: parsePacket with unknown protocol:" << ip_prot << "!" << std::endl;
//         srcport = 0;
//         dstport = 0;
//     }

//     ret[IndexType::SRCPORT]->key = std::string((char*)&srcport,2);
//     ret[IndexType::DSTPORT]->key = std::string((char*)&dstport,2);

//     return ret;
// }

#endif