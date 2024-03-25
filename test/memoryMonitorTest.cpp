#include "../component/controller.hpp"
#include <iostream>
#include <string>
#include <fstream>

#define TIMES 2

const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x04,0x00,0x65,0x00,0x00,0x00};
// const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                             0xff,0xff,0x00,0x00,0x01,0x00,0x00,0x00};
const u_int32_t buffer_len = 12*1024*1024;
const u_int32_t packet_num = 1e5;
const u_int32_t buffer_warn = 4*1024*1024;
const u_int32_t packet_warn = 8e4;
const u_int32_t pcap_header_len = 24;
const u_int32_t eth_header_len = 0;
const u_int32_t flow_capacity = 5e3;
const std::string filename = "./data/source/caida.pcap";
// const std::string out_filename = "./data/output/pcap_multithread.pcap";

int main(){
    MultiThreadController* controller = new MultiThreadController();
    InitData init_data = {
        .buffer_len = buffer_len * TIMES,
        .buffer_warn = buffer_warn * TIMES,
        .packet_num = packet_num * TIMES,
        .packet_warn = packet_warn * TIMES,
        .eth_header_len = eth_header_len,
        .filename = filename,
        .flow_capacity = flow_capacity * TIMES,
        .packetAggregatorThreadCount = 1,
        .flowMetaIndexGeneratorThreadCountEach = 1,
        .pcap_header = std::string((char*)pcap_head,pcap_header_len),
    };
    controller->init(init_data);
    controller->run();
}