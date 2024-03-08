#include "../component/pcapReader.hpp"
#include "../lib/shareBuffer.hpp"
#include "../lib/arrayList.hpp"
#include "../lib/util.hpp"
#include "../component/controller.hpp"
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
const std::string out_filename = "./data/output/pcap_multithread.pcap";

void singleThreadTest(){
    ShareBuffer* packetBuffer = new ShareBuffer(buffer_len,buffer_warn);
    ArrayList<u_int32_t>* packetPointer = new ArrayList<u_int32_t>(packet_num,packet_warn);
    PcapReader* reader = new PcapReader(pcap_header_len,eth_header_len,filename,packetBuffer,packetPointer);
    reader->run();
    CharData buffer_data = packetBuffer->outputToChar();
    CharData pointer_data = packetPointer->outputToChar();

    std::ofstream outputFile(out_filename, std::ios::binary);
    if (!outputFile.is_open()) {
        printf("Fail to Create File!\n");
        return;
    }
    outputFile.write((const char*)pcap_head,pcap_header_len);

    ArrayListNode<u_int32_t>* node_list = (ArrayListNode<u_int32_t>*)pointer_data.data;
    u_int32_t pos = 0;
    u_int8_t id = packetPointer->getIDOneThread(pos).data;
    
    for(int i = 0;i<pointer_data.len/sizeof(ArrayListNode<u_int32_t>);++i){
        if(packetPointer->getIDOneThread(i).data!=id){
            continue;
        }
        char* tmp = buffer_data.data+node_list[i].value;
        u_int32_t len =((data_header*)tmp)->caplen+sizeof(data_header);
        outputFile.write(tmp,((data_header*)tmp)->caplen+sizeof(data_header));
    }

    //outputFile.write(buffer_data.data+pcap_header_len,len-pcap_header_len);
    outputFile.close();

    delete packetBuffer;
    delete packetPointer;
    delete reader;
}

void multitest(){
    MultiThreadController* controller = new MultiThreadController();
    InitData init_data = {
        .buffer_len = buffer_len,
        .buffer_warn = buffer_warn,
        .packet_num = packet_num,
        .packet_warn = packet_warn,
        .pcap_header_len = pcap_header_len,
        .eth_header_len = eth_header_len,
        .filename = filename,
        .threadCount = 4,
    };
    controller->init(init_data);
    controller->run();

    OutputData data = controller->outputForTest();
    ShareBuffer* packetBuffer = data.packetBuffer;
    ArrayList<u_int32_t>* packetPointer = data.packetPointer;
    CharData buffer_data = packetBuffer->outputToChar();
    CharData pointer_data = packetPointer->outputToChar();

    delete controller;

    std::ofstream outputFile(out_filename, std::ios::binary);
    if (!outputFile.is_open()) {
        printf("Fail to Create File!\n");
        return;
    }
    outputFile.write((const char*)pcap_head,pcap_header_len);

    ArrayListNode<u_int32_t>* node_list = (ArrayListNode<u_int32_t>*)pointer_data.data;
    u_int32_t pos = 0;
    u_int32_t next = 0;

    // u_int8_t id = packetPointer->getIDOneThread(pos).data;

    while(true){
        if(next == std::numeric_limits<uint32_t>::max()){
            break;
        }
        pos = next;
        char* tmp = buffer_data.data + node_list[pos].value;
        u_int32_t len =((data_header*)tmp)->caplen+sizeof(data_header);
        outputFile.write(tmp,((data_header*)tmp)->caplen+sizeof(data_header));
        next = node_list[pos].next;
    }
    
    // for(int i = 0;i<pointer_data.len/sizeof(ArrayListNode<u_int32_t>);++i){
    //     if(packetPointer->getIDOneThread(i).data!=id){
    //         continue;
    //     }
    //     char* tmp = buffer_data.data+node_list[i].value;
    //     u_int32_t len =((data_header*)tmp)->caplen+sizeof(data_header);
    //     outputFile.write(tmp,((data_header*)tmp)->caplen+sizeof(data_header));
    // }

    //outputFile.write(buffer_data.data+pcap_header_len,len-pcap_header_len);
    outputFile.close();
}

int main(){
    // singleThreadTest();
    multitest();
}