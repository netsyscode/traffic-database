#include "../dpdk_component/controller.hpp"
#include <iostream>
#include <string>
#include <fstream>

#define TIMES 2

const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x04,0x00,0x65,0x00,0x00,0x00};
// const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                             0xff,0xff,0x00,0x00,0x01,0x00,0x00,0x00};
const u_int32_t index_ring_capacity = 1024*1024;
const u_int32_t storage_ring_capacity = 1024;
// const u_int64_t truncate_interval = ((u_int64_t)1)<<32;
const u_int32_t pcap_header_len = 24;
const u_int32_t eth_header_len = 14;
const u_int64_t file_capacity = 1024*1024*1024;
const u_int16_t nb_rx = 1;
const u_int32_t index_thread_num = 1;
const u_int32_t direct_storage_thread_num = 1;
const u_int32_t index_storage_thread_num = 1;
const u_int32_t max_node = 1024*1024;
const std::string bpf_prog_name = "./bpf/info.o";

int main(){
    Controller* controller = new Controller();
    InitData init_data = {
       .index_ring_capacity = index_ring_capacity,
    //    .storage_ring_capacity = storage_ring_capacity,
    //    .truncate_interval = truncate_interval,
       .nb_rx = nb_rx,
       .pcap_header_len = pcap_header_len,
       .eth_header_len = eth_header_len,
       .file_capacity = file_capacity,
       .index_thread_num = index_thread_num,
       .direct_storage_thread_num = direct_storage_thread_num,
       .index_storage_thread_num = index_storage_thread_num,
       .max_node = max_node,
       .pcap_header = std::string((char*)pcap_head,pcap_header_len),
       .bpf_prog_name = bpf_prog_name,
    };
    controller->init(init_data);
    controller->run();
}