#ifndef PCAPREADER_HPP_
#define PCAPREADER_HPP_
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include "../lib/shareBuffer.hpp"
#include "../lib/header.hpp"
#include "../lib/arrayList.hpp"

struct PacketMeta{
    char* data;
    u_int32_t len;
};

// read packet from pcap, equivalent like extractor, lower substitution of trace catcher
class PcapReader{
    const u_int32_t pcap_header_len;
    const u_int32_t eth_header_len;
    // const u_int32_t packet_buffer_max_len;
    // const u_int32_t packet_pointer_max_len;

    std::string filename;
    u_int32_t offset;
    std::ifstream file;
    u_int32_t fileLen;

    // write only
    ShareBuffer* packetBuffer;
    // write only
    ArrayList<u_int32_t>* packetPointer;

    ShareBuffer* newPacketBuffer;
    ArrayList<u_int32_t>* newpacketPointer;

    std::atomic_bool stop;
    std::atomic_bool pause;

    std::condition_variable* monitor_cv;

    u_int64_t duration_time;

    //opne file
    bool openFile();
    //read packet of offset from file;
    PacketMeta readPacket();
    //write data to packet buffer (pay attention to aligning with file offset)
    u_int32_t writePacketToPacketBuffer(PacketMeta& meta);
    //calculate id of packet
    u_int8_t calPacketID(PacketMeta& meta);
    //write data to packet pointer
    u_int32_t writePacketToPacketPointer(u_int32_t _offset, u_int8_t id);

    void truncate();
public:
    PcapReader(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename, ShareBuffer* buffer, ArrayList<u_int32_t>* packetPointer, std::condition_variable* monitor_cv):
    pcap_header_len(pcap_header_len),eth_header_len(eth_header_len),filename(filename),packetBuffer(buffer),packetPointer(packetPointer){
        this->offset = pcap_header_len;
        this->stop = true;
        this->pause = false;
        this->monitor_cv = monitor_cv;

        this->duration_time = 0;
    }
    ~PcapReader()=default;
    
    void run();
    void asynchronousStop();
    void asynchronousPause(ShareBuffer* newPacketBuffer, ArrayList<u_int32_t>* newpacketPointer);
    bool getPause() const;
};

#endif