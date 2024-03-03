#ifndef SHAREBUFFER_HPP_
#define SHAREBUFFER_HPP_
#include <iostream>
#include "header.hpp"

// for packet buffer
class ShareBuffer{
    char* buffer;
    const u_int32_t maxLength;
    const u_int32_t warningLength; //once exceed warningLength, warning = true
    std::atomic_bool warning;
    std::atomic_uint32_t writePos;
public:
    ShareBuffer(u_int32_t maxLength, u_int32_t warningLength):maxLength(maxLength),warningLength(warningLength){
        this->buffer = new char[maxLength];
        this->warning = false;
        this->writePos = 0;
    }
    ~ShareBuffer(){
        delete[] this->buffer;
    }
    //Non parallelizable
    u_int32_t writeOneThread(char* data, u_int32_t len){
        u_int32_t pos = writePos;
        if(pos + len >= this->maxLength){
            std::cerr << "Share buffer error: wite overflow the buffer!" <<std::endl;
            return std::numeric_limits<uint32_t>::max();
        }
        writePos += len;
        memcpy(this->buffer + pos, data, len);
        if(pos + len >= this->warningLength){
            this->warning = true;
        }
        return pos;
    }
    //parallelizable, the data should be: pcap header, pcap data
    char* readPcap(u_int32_t pos){
        if(pos >= this->maxLength){
            std::cerr << "Share buffer error: read pos overflow the buffer!" <<std::endl;
            return nullptr;
        }
        data_header* pcap_header = (data_header*)(this->buffer+pos);
        u_int32_t len = pcap_header->caplen;
        char* ret = new char[len+sizeof(data_header)];
        memcpy(ret,this->buffer+pos,len+sizeof(data_header));
        return ret;
    }
};

#endif