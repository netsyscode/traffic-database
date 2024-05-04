#include "pcapReader.hpp"
#include <thread>
#include <pthread.h>

const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x04,0x00,0x65,0x00,0x00,0x00};

// std::hash<u_int32_t> hasher_32;
// std::hash<u_int16_t> hasher_16;

u_int8_t hasher_32(u_int32_t value){
    u_int8_t hashValue = 0;
    for(int i = 0;i<sizeof(u_int32_t)/sizeof(u_int8_t);++i){
        hashValue ^= value & 0xff;
        value >>= sizeof(u_int8_t);
    }
    return hashValue;
}

u_int8_t hasher_16(u_int16_t value){
    u_int8_t hashValue = 0;
    for(int i = 0;i<sizeof(u_int16_t)/sizeof(u_int8_t);++i){
        hashValue ^= value & 0xff;
        value >>= sizeof(u_int8_t);
    }
    return hashValue;
}

struct PacketMetaTurple{
    u_int32_t srcIP;
    u_int32_t dstIP;
    u_int16_t srcPort;
    u_int16_t dstPort;
};

bool PcapReader::openFile(){
    // this->file = std::ifstream(filename,std::ios::binary);
    // if(!file){
    //     std::cerr<<"Pcap reader error: fail to open file: " <<this->filename << " !" << std::endl;
    //     return false;
    // }
    // file.seekg(0, std::ios_base::end);  
    // this->fileLen = file.tellg();
    // // file.seekg(this->offset,std::ios::beg);
    // return true;
    this->packetBuffer->changeFileName(this->filename);
    return this->packetBuffer->openFile();
}

PacketMeta PcapReader::readPacket(){
    PacketMeta meta = {
        .data = nullptr,
        .len = 0,
    };
    // if(this->offset >= this->fileLen){
    //     return meta;
    // }
    meta.data = this->packetBuffer->getPointer(this->offset);
    if(meta.data == nullptr){
        return meta;
    }

    data_header* header = (data_header*)(meta.data);
    // data_header* header = (data_header*)(this->file_buffer+this->offset);
    // memcpy((char*)&header,this->file_buffer+this->offset,sizeof(struct data_header));
    // file.seekg(this->offset, std::ios::beg);
    // file.read((char*)&header,sizeof(struct data_header));
    // this->offset += sizeof(struct data_header);

    meta.len = header->caplen + sizeof(struct data_header);
    // meta.data = (char*)(header);
    // meta.data = new char[meta.len];
    // memcpy(meta.data, (char*)&header, sizeof(struct data_header));
    
    // memcpy(meta.data + sizeof(struct data_header),this->file_buffer+this->offset,header.caplen);
    // file.seekg(this->offset, std::ios::beg);
    // file.read(meta.data + sizeof(struct data_header), header.caplen);
    // this->offset += header->caplen;
    // this->offset += meta.len;
    return meta;
}

// u_int32_t PcapReader::writePacketToPacketBuffer(PacketMeta& meta){
//     return this->packetBuffer->writeOneThread(meta.data,meta.len);
// }

u_int8_t PcapReader::calPacketID(PacketMeta& meta){
    PacketMetaTurple meta_turple;
    ip_header* ip_protocol = (ip_header*)(meta.data + sizeof(struct data_header) + this->eth_header_len);
    int ip_header_length = ip_protocol->ip_header_length;
    int ip_prot = ip_protocol->ip_protocol;
    meta_turple.srcIP = htonl(ip_protocol->ip_source_address);
    meta_turple.dstIP = htonl(ip_protocol->ip_destination_address);

    if(ip_prot == 6){
        tcp_header* tcp_protocol = (tcp_header*)(meta.data + sizeof(struct data_header) + this->eth_header_len + ip_header_length * 4);
        meta_turple.srcPort = htons(tcp_protocol->tcp_source_port);
        meta_turple.dstPort = htons(tcp_protocol->tcp_destination_port);
    }else if(ip_prot == 17){
        udp_header* udp_protocol = (udp_header*)(meta.data + sizeof(struct data_header) + this->eth_header_len + ip_header_length * 4);
        meta_turple.srcPort = htons(udp_protocol->udp_source_port);
        meta_turple.dstPort = htons(udp_protocol->udp_destination_port);
    }else{
        return 0;
    }

    u_int8_t id = hasher_32(meta_turple.srcIP)^hasher_32(meta_turple.dstIP)^
                    hasher_16(meta_turple.srcPort)^hasher_16(meta_turple.dstPort);
    return id;
}
    
u_int32_t PcapReader::writePacketToPacketPointer(u_int32_t _offset, u_int8_t id){
    // return this->packetPointer->addNodeOneThread(_offset,id);
    return this->packetPointer->addNodeMultiThread(_offset,id,this->nodeNumber);
}

// void PcapReader::truncate(){
//     if(this->newPacketBuffer == nullptr || this->newpacketPointer == nullptr){
//         std::cerr << "Pcap reader error: trancate without new memory!" << std::endl;
//         this->pause = false;
//         this->monitor_cv->notify_all();
//         return;
//     }
//     this->packetBuffer = this->newPacketBuffer;
//     this->packetPointer = this->newpacketPointer;
//     this->newPacketBuffer = nullptr;
//     this->newpacketPointer = nullptr;
//     // this->packetBuffer->writeOneThread((const char*)pcap_head,this->pcap_header_len);
//     this->pause = false;
//     this->monitor_cv->notify_all();
// }
void PcapReader::truncate(){
    printf("Pcap reader log: begin truncate.\n");
    if(this->newpacketPointer == nullptr){
        std::cerr << "Pcap reader error: trancate without new memory!" << std::endl;
        this->pause = false;
        this->monitor_cv->notify_all();
        return;
    }
    this->packetPointer = this->newpacketPointer;
    this->newpacketPointer = nullptr;
    this->nodeNumber = 0;
    // this->packetBuffer->writeOneThread((const char*)pcap_head,this->pcap_header_len);
    this->pause = false;
    this->monitor_cv->notify_all();
}

void PcapReader::run(){
    // pthread_t threadId = pthread_self();

    // // 创建 CPU 集合，并将指定核心加入集合中
    // cpu_set_t cpuset;
    // CPU_ZERO(&cpuset);
    // CPU_SET(1, &cpuset);

    // // 设置线程的 CPU 亲和性
    // int result = pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpuset);
    // if (result != 0) {
    //     std::cerr << "Failed to set thread affinity: " << result << std::endl;
    // } else {
    //     std::cout << "Thread successfully bound to CPU core " << std::endl;
    // }

    // if(!this->openFile()){
    //     return;
    // }

    // this->file_buffer = new char[60*1024*1024];

    std::cout << "Pcap reader log: thread run." << std::endl;
    this->stop = false;
    this->pause = false;
    //align
    // this->packetBuffer->writeOneThread((const char*)pcap_head,this->pcap_header_len);

    // this->file.seekg(0,std::ios::beg);
    // this->file.read(this->file_buffer,this->fileLen);

    auto start = std::chrono::high_resolution_clock::now();
    u_int64_t truncate_time = 0;
    u_int64_t read_time = 0;
    u_int64_t write_buffer_time = 0;
    u_int64_t write_cal_time = 0;
    u_int64_t write_pointer_time = 0;

    for(int i=0;i<1;++i){
        this->offset = pcap_header_len;
        while(true){
            if(this->nodeNumber > this->pointer_limit){
                auto write_buffer_start = std::chrono::high_resolution_clock::now();
                if(this->pause){
                    auto start_truncate = std::chrono::high_resolution_clock::now();
                    this->truncate();
                    auto end_truncate = std::chrono::high_resolution_clock::now();
                    truncate_time += std::chrono::duration_cast<std::chrono::microseconds>(end_truncate - start_truncate).count();
                }
                auto write_buffer_end = std::chrono::high_resolution_clock::now();
                write_buffer_time += std::chrono::duration_cast<std::chrono::microseconds>(write_buffer_end - write_buffer_start).count();
                continue;
            }
            auto read_start = std::chrono::high_resolution_clock::now();
            PacketMeta meta = this->readPacket();
            if(meta.data == nullptr){
                printf("Pcap reader log: read over at %llu.\n",this->offset);
                // std::cout << "Pcap reader log: read over at." << std::endl;
                break;
            }
            auto read_end = std::chrono::high_resolution_clock::now();
            read_time += std::chrono::duration_cast<std::chrono::microseconds>(read_end - read_start).count();

            
            // u_int32_t _offset = this->writePacketToPacketBuffer(meta);

            // if(_offset == std::numeric_limits<uint32_t>::max()){
            //     std::cerr << "Pcap reader error: packet buffer overflow!" << std::endl;
            //     break;
            // }
            auto write_cal_start = std::chrono::high_resolution_clock::now();
            u_int8_t id = this->calPacketID(meta);
            auto write_cal_end = std::chrono::high_resolution_clock::now();
            auto write_pointer_start = std::chrono::high_resolution_clock::now();

            this->nodeNumber = this->writePacketToPacketPointer(this->offset,id);
            if(this->nodeNumber == std::numeric_limits<uint32_t>::max()){
                std::cerr << "Pcap reader error: packet pointer overflow!" << std::endl;
                break;
            }
            this->offset += meta.len;
            auto write_pointer_end = std::chrono::high_resolution_clock::now();
            write_cal_time += std::chrono::duration_cast<std::chrono::microseconds>(write_cal_end - write_cal_start).count();
            write_pointer_time += std::chrono::duration_cast<std::chrono::microseconds>(write_pointer_end - write_pointer_start).count();
            if(this->stop){
                // std::cout << "Pcap reader log: asynchronous stop." << std::endl;
                break;
            }
            // if(this->pause){
            //     auto start_truncate = std::chrono::high_resolution_clock::now();
            //     this->truncate();
            //     auto end_truncate = std::chrono::high_resolution_clock::now();
            //     truncate_time += std::chrono::duration_cast<std::chrono::microseconds>(end_truncate - start_truncate).count();
            // }
            if(this->packetPointer->getWarning()){
                // printf("Pcap reader log: pointer warning.\n");
                this->monitor_cv->notify_all();
            }
        }
        if(this->stop){
            std::cout << "Pcap reader log: asynchronous stop." << std::endl;
            break;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    // std::cout << "Pcap reader log: read over." << std::endl;
    this->duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    while(true){// wait
        if(this->stop){
            std::cout << "Pcap reader log: asynchronous stop." << std::endl;
            break;
        }
        if(this->pause){
            this->truncate();
        }
    }
    // delete[] this->file_buffer;
    // this->packetBuffer->closeFile();
    printf("Pcap reader log: thread quit, during %llu us and %llu is truncate time.\n",this->duration_time,truncate_time);
    printf("Pcap reader log: read time: %llu us, write buffer time: %llu us, write cal time: %llu us, write pointer time: %llu us.\n",read_time,write_buffer_time,write_cal_time,write_pointer_time);
}

void PcapReader::asynchronousStop(){
    this->stop = true;
}

// void PcapReader::asynchronousPause(ShareBuffer* newPacketBuffer, ArrayList<u_int32_t>* newpacketPointer){
//     this->newPacketBuffer = newPacketBuffer;
//     this->newpacketPointer = newpacketPointer;
//     this->pause = true;
// }
void PcapReader::asynchronousPause(ArrayList<u_int32_t>* newpacketPointer){
    // this->newPacketBuffer = newPacketBuffer;
    this->newpacketPointer = newpacketPointer;
    this->pause = true;
}

bool PcapReader::getPause() const{
    return this->pause;
}