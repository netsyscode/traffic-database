#include "pcapReader.hpp"

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
    this->file = std::ifstream(filename,std::ios::binary);
    if(!file){
        std::cerr<<"Pcap reader error: fail to open file: " <<this->filename << " !" << std::endl;
        return false;
    }
    file.seekg(0, std::ios_base::end);  
    this->fileLen = file.tellg();
    file.seekg(this->offset,std::ios::beg);
    return true;
}

PacketMeta PcapReader::readPacket(){
    PacketMeta meta = {
        .data = nullptr,
        .len = 0,
    };
    if(this->offset >= this->fileLen){
        return meta;
    }
    data_header header;
    file.seekg(this->offset, std::ios::beg);
    file.read((char*)&header,sizeof(struct data_header));
    this->offset += sizeof(struct data_header);

    meta.len = header.caplen + sizeof(struct data_header);
    meta.data = new char[meta.len];
    memcpy(meta.data, (char*)&header, sizeof(struct data_header));
    
    file.seekg(this->offset, std::ios::beg);
    file.read(meta.data + sizeof(struct data_header), header.caplen);
    this->offset += header.caplen;

    return meta;
}

u_int32_t PcapReader::writePacketToPacketBuffer(PacketMeta& meta){
    return this->packetBuffer->writeOneThread(meta.data,meta.len);
}

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
    return this->packetPointer->addNodeOneThread(_offset,id);
}

void PcapReader::truncate(){
    if(this->newPacketBuffer == nullptr || this->newpacketPointer == nullptr){
        std::cerr << "Pcap reader error: trancate without new memory!" << std::endl;
        this->pause = false;
        this->monitor_cv->notify_all();
        return;
    }
    this->packetBuffer = this->newPacketBuffer;
    this->packetPointer = this->newpacketPointer;
    this->newPacketBuffer = nullptr;
    this->newpacketPointer = nullptr;
    this->packetBuffer->writeOneThread((const char*)pcap_head,this->pcap_header_len);
    this->pause = false;
    this->monitor_cv->notify_all();
}

void PcapReader::run(){
    if(!this->openFile()){
        return;
    }
    std::cout << "Pcap reader log: thread run." << std::endl;
    this->stop = false;
    this->pause = false;
    //align
    this->packetBuffer->writeOneThread((const char*)pcap_head,this->pcap_header_len);
    while(true){
        PacketMeta meta = this->readPacket();
        if(meta.data == nullptr){
            std::cout << "Pcap reader log: read over." << std::endl;
            break;
        }
        u_int32_t _offset = this->writePacketToPacketBuffer(meta);
        if(_offset == std::numeric_limits<uint32_t>::max()){
            std::cerr << "Pcap reader error: packet buffer overflow!" << std::endl;
            break;
        }
        u_int8_t id = this->calPacketID(meta);
        u_int32_t nodeNum = this->writePacketToPacketPointer(_offset,id);
        if(nodeNum == std::numeric_limits<uint32_t>::max()){
            std::cerr << "Pcap reader error: packet pointer overflow!" << std::endl;
            break;
        }
        if(this->stop){
            std::cout << "Pcap reader log: asynchronous stop." << std::endl;
            break;
        }
        if(this->pause){
            this->truncate();
        }
        if(this->packetBuffer->getWarning()){
            this->monitor_cv->notify_all();
        }
    }
    while(true){// wait
        if(this->stop){
            std::cout << "Pcap reader log: asynchronous stop." << std::endl;
            break;
        }
        if(this->pause){
            this->truncate();
        }
    }
    std::cout << "Pcap reader log: thread quit." << std::endl;
}

void PcapReader::asynchronousStop(){
    this->stop = true;
}

void PcapReader::asynchronousPause(ShareBuffer* newPacketBuffer, ArrayList<u_int32_t>* newpacketPointer){
    this->newPacketBuffer = newPacketBuffer;
    this->newpacketPointer = newpacketPointer;
    this->pause = true;
}

bool PcapReader::getPause() const{
    return this->pause;
}