#include "../component/controller.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <cstdlib>

#define TIMES 10

const u_int8_t pcap_head_no_eth[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x04,0x00,0x65,0x00,0x00,0x00};
const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0xff,0xff,0x00,0x00,0x01,0x00,0x00,0x00};
const u_int32_t buffer_len = 12*1024*1024;
const u_int32_t packet_num = 3e5;
const u_int32_t buffer_warn = 4*1024*1024;
const u_int32_t packet_warn = 1e5;
const u_int32_t pcap_header_len = 24;
const u_int32_t eth_header_len = 14;
const u_int32_t flow_capacity = 5e3;
// const std::string filename = "./data/source/caida.pcap";
// const std::string out_filename = "./data/output/pcap_multithread.pcap";

int main(int argc, char *argv[]){
    int opt;
    InitData init_data = {
        .buffer_len = buffer_len * TIMES,
        .buffer_warn = buffer_warn * TIMES,
        .packet_num = packet_num * TIMES,
        .packet_warn = packet_warn * TIMES,
        .eth_header_len = eth_header_len,
        .flow_capacity = flow_capacity * TIMES,
        .filename = std::string(),
        .packetAggregatorThreadCount = 16,
        .flowMetaIndexGeneratorThreadCountEach = 4,
        .pcap_header = std::string((char*)pcap_head,pcap_header_len),
    };
    while ((opt = getopt(argc, argv, "e:f:")) != -1) {
        switch (opt) {
            case 'e':
                init_data.eth_header_len = std::stoul(optarg);
                if(init_data.eth_header_len == 0){
                    init_data.pcap_header = std::string((char*)pcap_head_no_eth,pcap_header_len);
                }
                break;
            case 'f':
                init_data.filename = std::string(optarg);
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-p] [-f filename]" << std::endl;
                return 1;
        }
    }
    if(init_data.filename.size()==0){
        std::cerr << "-f is neccesary!" << std::endl;
        return 1;
    }
    MultiThreadController* controller = new MultiThreadController();
    controller->init(init_data);
    controller->run();
    return 0;
}