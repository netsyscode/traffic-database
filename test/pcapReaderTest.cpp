#include "../component/pcapReader.hpp"
#include "../lib/shareBuffer.hpp"
#include "../lib/arrayList.hpp"
#include <iostream>
#include <string>
#include <fstream>

// const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                             0x00,0x00,0x04,0x00,0x65,0x00,0x00,0x00};
const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0xff,0xff,0x00,0x00,0x01,0x00,0x00,0x00};
const u_int32_t buffer_len = 4*1024*1024;
const u_int32_t packet_num = 1e4;
const u_int32_t buffer_warn = 3*1024*1024;
const u_int32_t packet_warn = 8000;
const u_int32_t pcap_header_len = 24;
const u_int32_t eth_header_len = 14;
const std::string filename = "./data/source/pcap.pcap";
const std::string out_filename = "./data/output/pcap.pcap";


void outputPacketToNewFile(std::string new_filename, char* data, u_int32_t len){
    std::ofstream outputFile(new_filename, std::ios::binary);
    if (!outputFile.is_open()) {
        printf("Fail to Create File!\n");
        return;
    }
    outputFile.write((const char*)pcap_head,pcap_header_len);
    outputFile.write(data+pcap_header_len,len-pcap_header_len);
    outputFile.close();
}

void singleThreadTest(){
    ShareBuffer* packetBuffer = new ShareBuffer(buffer_len,buffer_warn);
    ArrayList<u_int32_t>* packetPointer = new ArrayList<u_int32_t>(packet_num,packet_warn);
    PcapReader* reader = new PcapReader(pcap_header_len,eth_header_len,filename,packetBuffer,packetPointer);
    reader->run();
    ShareBufferData buffer_data = packetBuffer->outputToChar();
    outputPacketToNewFile(out_filename,buffer_data.data,buffer_data.len);
}

int main(){
    singleThreadTest();
}