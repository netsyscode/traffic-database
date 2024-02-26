#include "extractor.hpp"
#include <arpa/inet.h>

//#define ETH_LEN 14
#define ETH_LEN 0
#define PCAP_LEN 24
#define HEADER_LEN 16

void Extractor::changeFileName(string filename){
    this->filename = filename;
}

void Extractor::openFile(){
    this->file = ifstream(filename,ios::binary);
    if(!file){
        printf("File open fail!\n");
        return;
    }
    this->offset = 0;
    this->offset += PCAP_LEN;
    file.seekg(0, ios_base::end);  
    this->filelength = file.tellg();
    file.seekg(0,ios::beg);
    //printf("len:%u\n",this->filelength);
}

void Extractor::closeFile(){
    if(!file.is_open())
        return;
    //printf("offset:%u\n",offset);
    file.close();
    //printf("File closed.\n");
}

PacketMetadata* Extractor::readCurrentPacketAndChangeOffset(){
    if(!file.is_open()){
        printf("File not open!\n");
        return nullptr;
    }
    if(this->offset >= this->filelength){
        return nullptr;
    }

    PacketMetadata* meta = new PacketMetadata();
    
    int ip_header_length; 
    int t_header_length;
    struct in_addr srcip;
    struct in_addr dstip;
    u_int8_t ip_prot;
    u_int16_t sport;
    u_int16_t dport;
    u_int32_t ack;
    u_int32_t length;
    u_int32_t ts_h;
    u_int32_t ts_l;

    u_int32_t data_header_offset = offset;
    struct data_header data_head;
    file.seekg(data_header_offset, ios::beg);
    file.read((char*)&data_head,sizeof(struct data_header));
    length = data_head.caplen;
    ts_h = data_head.ts_h;
    ts_l = data_head.ts_l;

    u_int32_t ip_header_offset = offset + HEADER_LEN + ETH_LEN;
    struct ip_header ip_protocol;
    file.seekg(ip_header_offset, ios::beg);
    file.read((char*)&ip_protocol,sizeof(struct ip_header));
    ip_header_length = ip_protocol.ip_header_length;
    ip_prot = ip_protocol.ip_protocol;
    srcip = ip_protocol.ip_source_address;
    dstip = ip_protocol.ip_destination_address;

    //printf("ip_header_length:%d\n",ip_header_length);
    //printf("ip_prot:%u\n",ip_prot);

    u_int32_t l4_header_offset = offset + HEADER_LEN + ETH_LEN + ip_header_length*4;
    file.seekg(l4_header_offset, ios::beg);
    if(ip_prot == 6){
        struct tcp_header tcp_protocol;
        file.read((char*)&tcp_protocol,sizeof(struct tcp_header));
        sport = tcp_protocol.tcp_source_port;
        dport = tcp_protocol.tcp_destination_port;
        ack = tcp_protocol.tcp_ack;
    }
    else if(ip_prot == 17){
        struct udp_header udp_protocol;
        file.read((char*)&udp_protocol,sizeof(struct udp_header));
        sport = udp_protocol.udp_source_port;
        dport = udp_protocol.udp_destination_port;
        ack = 0;
    }
    sport = htons(sport);
    dport = htons(dport);
    ack = htonl(ack);

    // printf("srcIP:%s\n",inet_ntoa(srcip));          
    // printf("dstIP:%s\n",inet_ntoa(dstip));
    // printf("sport:%hu\n",sport);
    // printf("dport:%hu\n",dport);
    // printf("ack:%u\n",ack);
    // printf("len:%u\n",length);
    // printf("ts:%u.%u\n",ts_h,ts_l);

    meta->sourceAddress = srcip;
    meta->destinationAddress = dstip;
    meta->sourcePort = sport;
    meta->destinationPort = dport;
    meta->ack = ack;
    meta->length = length;
    meta->ts_h = ts_h;
    meta->ts_l = ts_l;
    meta->filename = this->filename;
    meta->offset = this->offset;

    offset += HEADER_LEN + length;
    return meta;
}

PacketMetadata* Extractor::readPacketByOffset(u_int32_t offset){
    u_int32_t old_offset = this->offset;
    this->offset = offset;
    PacketMetadata* meta = this->readCurrentPacketAndChangeOffset();
    this->offset = offset;
    return meta;
}

void Extractor::changeOffset(u_int32_t offset){
    this->offset = offset;
}

u_int32_t Extractor::getCurrentOffset(){
    return this->offset;
}