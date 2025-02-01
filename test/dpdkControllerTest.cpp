#include "../dpdk_component/controller.hpp"
#include <iostream>
#include <string>
#include <fstream>

#define TIMES 2

const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x04,0x00,0x65,0x00,0x00,0x00};
// const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                             0xff,0xff,0x00,0x00,0x01,0x00,0x00,0x00};
const u_int32_t index_ring_capacity = 1024*1024*128;
const u_int32_t storage_ring_capacity = 1024;
// const u_int64_t truncate_interval = ((u_int64_t)1)<<32;
const u_int32_t pcap_header_len = 24;
const u_int32_t eth_header_len = 14;
const u_int64_t file_capacity = 1024*1024*1024;
u_int16_t nb_rx = 8;
u_int32_t index_thread_num = 4;
const u_int32_t direct_storage_thread_num = 2;
const u_int32_t index_storage_thread_num = 1;
const u_int32_t max_node = 1024*1024*4;
const std::string bpf_prog_name = "./bpf/tag.o";

int main(){
   Controller* controller = new Controller();

   InitData init_data;

   std::cout << "Do you want to bind to cores? (y/n)" << std::endl;
   char bind;
   std::cin >> bind;
   if(bind == 'y'){
      init_data.bind_core = true;
      std::cout << "Enter the controller core number (0 is remained)" << std::endl;
      std::cin >> init_data.controller_core_id;
   }else{
      init_data.bind_core = false;
      init_data.controller_core_id = 0;
   }

   std::cout << "Enter number of DPDK packet capture threads" << std::endl;
   std::cin >> nb_rx;
   init_data.nb_rx = nb_rx;

   if(init_data.bind_core){
      std::cout << "Enter the core number for each DPDK packet capture threads (0 is remained)" << std::endl;
      for(int i=0;i<nb_rx;++i){
         u_int32_t core_id;
         std::cin >> core_id;
         init_data.dpdk_core_id_list.push_back(core_id);
      }
      std::cout << "Enter the core number for each packet processing threads (0 is remained)" << std::endl;
      for(int i=0;i<nb_rx;++i){
         u_int32_t core_id;
         std::cin >> core_id;
         init_data.packet_core_id_list.push_back(core_id);
      }
   }

   std::cout << "Enter number of indexing thread" << std::endl;
   std::cin >> index_thread_num;

   init_data.index_thread_num = index_thread_num;

   if(init_data.bind_core){
      std::cout << "Enter the core number for each indexing threads (0 is remained)" << std::endl;
      for(int i=0;i<index_thread_num;++i){
         u_int32_t core_id;
         std::cin >> core_id;
         init_data.indexing_core_id_list.push_back(core_id);
      }
   }

   init_data.index_ring_capacity = index_ring_capacity;
   init_data.pcap_header_len = pcap_header_len;
   init_data.eth_header_len = eth_header_len;
   init_data.file_capacity = file_capacity;
   init_data.direct_storage_thread_num = direct_storage_thread_num;
   init_data.index_storage_thread_num = index_storage_thread_num;
   init_data.max_node = max_node;
   init_data.pcap_header = std::string((char*)pcap_head,pcap_header_len);
   init_data.bpf_prog_name = bpf_prog_name;

   controller->init(init_data);
   controller->run();
}