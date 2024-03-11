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
const u_int32_t flow_capacity = 5e3;
const std::string filename = "./data/source/pcap.pcap";
const std::string out_filename = "./data/output/pcap_multithread.pcap";

void singleThreadTest(){
    ShareBuffer* packetBuffer = new ShareBuffer(buffer_len,buffer_warn);
    ArrayList<u_int32_t>* packetPointer = new ArrayList<u_int32_t>(packet_num,packet_warn);
    PcapReader* reader = new PcapReader(pcap_header_len,eth_header_len,filename,packetBuffer,packetPointer);
    reader->run();
    std::string buffer_data = packetBuffer->outputToChar();
    std::string pointer_data = packetPointer->outputToChar();

    std::ofstream outputFile(out_filename, std::ios::binary);
    if (!outputFile.is_open()) {
        printf("Fail to Create File!\n");
        return;
    }
    outputFile.write((const char*)pcap_head,pcap_header_len);

    ArrayListNode<u_int32_t>* node_list = (ArrayListNode<u_int32_t>*)&pointer_data[0];
    u_int32_t pos = 0;
    u_int8_t id = packetPointer->getIDOneThread(pos).data;
    
    for(int i = 0;i<pointer_data.size()/sizeof(ArrayListNode<u_int32_t>);++i){
        if(packetPointer->getIDOneThread(i).data!=id){
            continue;
        }
        char* tmp = &buffer_data[node_list[i].value];
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
        .flow_capacity = flow_capacity,
        .packetAggregatorThreadCount = 4,
        .flowMetaIndexGeneratorThreadCountEach = 2,
    };
    controller->init(init_data);
    controller->run();

    OutputData data = controller->outputForTest();
    ShareBuffer* packetBuffer = data.packetBuffer;
    ArrayList<u_int32_t>* packetPointer = data.packetPointer;
    std::vector<RingBuffer*>* flowMetaIndexBuffers = data.flowMetaIndexBuffers;
    std::string buffer_data = packetBuffer->outputToChar();
    std::string pointer_data = packetPointer->outputToChar();
    std::vector<int> index_datas;
    for(auto rb:*flowMetaIndexBuffers){
        index_datas.push_back(rb->getPosDiff());
    }
    std::vector<std::string> cache_datas;
    for(int i=0;i<data.flowMetaEleLens.size();++i){
        if(data.flowMetaEleLens[i]==2){
            SkipList<u_int16_t,u_int32_t>* cache = (SkipList<u_int16_t,u_int32_t>*)data.flowMetaIndexCaches[i];
            cache_datas.push_back(cache->outputToChar());
        }else if(data.flowMetaEleLens[i]==4){
            SkipList<u_int32_t,u_int32_t>* cache = (SkipList<u_int32_t,u_int32_t>*)data.flowMetaIndexCaches[i];
            cache_datas.push_back(cache->outputToChar());
        }
    }
    
    delete controller;

    std::ofstream outputFile(out_filename, std::ios::binary);
    if (!outputFile.is_open()) {
        printf("Fail to Create File!\n");
        return;
    }
    outputFile.write((const char*)pcap_head,pcap_header_len);

    ArrayListNode<u_int32_t>* node_list = (ArrayListNode<u_int32_t>*)&pointer_data[0];
    u_int32_t pos = 0;
    u_int32_t next = 0;

    while(true){
        if(next == std::numeric_limits<uint32_t>::max()){
            break;
        }
        pos = next;
        char* tmp = &buffer_data[node_list[pos].value];
        u_int32_t len =((data_header*)tmp)->caplen+sizeof(data_header);
        outputFile.write(tmp,((data_header*)tmp)->caplen+sizeof(data_header));
        next = node_list[pos].next;
    }
    outputFile.close();

    for(auto i:index_datas){
        std::cout<< "Ring buffer diff " << i << " with thread count each " << init_data.flowMetaIndexGeneratorThreadCountEach <<std::endl;
    }

    for(auto s:cache_datas){
        std::cout<< "Index cache size " << s.size() <<std::endl;
    }

    u_int32_t offset = 0;
    while (offset < cache_datas[3].size()){
        u_int16_t dport;
        u_int32_t value;
        memcpy(&dport,&(cache_datas[3][offset]),sizeof(dport));
        offset += sizeof(dport);
        memcpy(&value,&(cache_datas[3][offset]),sizeof(value));
        offset += sizeof(value);
        
        if(dport==9999){
            std::cout<< "Dstport " << dport << " with value " << value <<std::endl;
        }
    }
}

int main(){
    // singleThreadTest();
    multitest();
}