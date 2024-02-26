#ifndef EXTRACTOR_HPP_
#define EXTRACTOR_HPP_
#include <iostream>
#include <pcap.h>
#include <string>
#include <fstream>
#include "header.hpp"
using namespace std;

struct PacketMetadata{
    struct in_addr sourceAddress;
    struct in_addr destinationAddress;
    u_int16_t sourcePort;
    u_int16_t destinationPort;
    u_int32_t ack; //unuse
    u_int32_t length; 
    u_int32_t ts_h;
    u_int32_t ts_l;
    string filename; //unuse
    u_int32_t offset;
};

class Extractor{
    string filename;
    u_int32_t offset;
    ifstream file;
    u_int32_t filelength;
public:
    Extractor(){}
    Extractor(string filename){
        this->filename = filename;
    }
    ~Extractor(){
        if(file.is_open())
            file.close();
    }
    void changeFileName(string filename);
    void openFile();
    void closeFile();
    PacketMetadata* readCurrentPacketAndChangeOffset();
    PacketMetadata* readPacketByOffset(u_int32_t offset);
    void changeOffset(u_int32_t offset);
    u_int32_t getCurrentOffset();
};

#endif