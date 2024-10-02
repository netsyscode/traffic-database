#ifndef COMPRESSOR_HPP_
#define COMPRESSOR_HPP_
#include "memoryBuffer.hpp"
#include "header.hpp"
#include <bitset>

#define BITSET_LEN 65536
#define TAG_LEN 8
#define MAX_GAP 65536
#define COMPRESS_FLAG 0xfe

struct CompressSegment{
    u_int8_t flag = COMPRESS_FLAG;
    u_int8_t len;
    u_int16_t diff;
};

class Compressor{
private:
    u_int32_t* dict;
    char* buffer;
    u_int64_t bufferOffset;
    // std::bitset<BITSET_LEN> seen;

    // u_int16_t getUnSeen(const char* data, u_int32_t data_len){
    //     if(data_len == 0){
    //         return 0;
    //     }

    //     this->seen.reset();

    //     for (size_t i = 0; i < data_len - 1; ++i) {
    //         seen.set(data[i]);
    //         unsigned short substr = (data[i] << 8) | data[i + 1];
    //         seen.set(substr);
    //     }
    //     seen.set(data[data_len-1]);
    //     for (unsigned short i = 0; i < BITSET_LEN; ++i) {
    //         if (!seen.test(i)) {
    //             return i;
    //         }
    //     }
    //     return BITSET_LEN - 1;
    // }
public:
    Compressor(char* _buffer, u_int32_t dict_len){
        this->dict = new u_int32_t[dict_len]();
        this->buffer = _buffer;
        this->bufferOffset = 0;
        // this->seen = std::bitset<BITSET_LEN>(0x0);
    }
    ~Compressor(){
        delete[] this->dict;
    }
    void directWriteToBuffer(const char* data, u_int32_t data_len){
        memcpy(this->buffer+this->bufferOffset,data,data_len);
        this->bufferOffset += data_len;
    }
    inline void directWriteToBuffer(char c){
        *(this->buffer + this->bufferOffset) = c;
        this->bufferOffset ++;
        // to escape
        if(c==(char)COMPRESS_FLAG){
            *(this->buffer + this->bufferOffset) = 0;
            this->bufferOffset ++;
        }
    }
    void compress(struct pcap_header* header, const char* data, u_int32_t data_len, u_int16_t drop_offset, u_int16_t drop_len){
        // u_int32_t cap_len = hearder->caplen;
        // struct compress_header* new_header = (struct compress_header*)(hearder);
        // new_header->caplen = (u_int16_t)cap_len;
        // new_header->compress_flag = this->getUnSeen(data,data_len);
        // this->directWriteToBuffer((char*)new_header,sizeof(struct compress_header));

        // uint8_t flag_len = new_header->compress_flag <= 256 ? 1 : 2;

        struct pcap_header* new_header = (struct pcap_header*)(this->buffer + this->bufferOffset);
        this->directWriteToBuffer((const char*)header, sizeof(struct pcap_header));

        auto origin = this->bufferOffset;

        u_int32_t now = 0;

        u_int32_t dindex;
        u_int32_t m_off;
        u_int32_t m_pos;
        const char* pointer;

        if(data_len < TAG_LEN){
            this->directWriteToBuffer(data,data_len);
        }

        while (now < data_len - TAG_LEN){
            pointer = data + now;
            if(now + TAG_LEN == drop_offset){
                for(u_int8_t i=0;i<TAG_LEN;++i){
                    this->directWriteToBuffer(pointer[i]);
                }
                now += TAG_LEN;
                now += drop_len;
            }
            // Hash
            dindex = (((0x21 * ((((((unsigned)(pointer[3]) << 6) ^ pointer[2]) << 5) ^ pointer[1]) << 5) ^ pointer[0])) >> 5) & 0x3ffff;
            m_pos = dict[dindex];
            if ((m_pos >= this->bufferOffset) || (m_off = (unsigned)(this->bufferOffset + now - m_pos)) > MAX_GAP || m_off < TAG_LEN){
                this->dict[dindex] = now + this->bufferOffset;
                this->directWriteToBuffer(*pointer);
                now++;
                continue;
            }  
            if(!(this->buffer[m_pos] == pointer[0] && this->buffer[m_pos + 1] == pointer[1] && this->buffer[m_pos + 2] == pointer[2] && this->buffer[m_pos + 3] == pointer[3])){
                this->dict[dindex] = now + this->bufferOffset;
                this->directWriteToBuffer(*pointer);
                now++;
                continue;
            }
            u_int8_t len = TAG_LEN;
            while(this->buffer[m_pos+len]==pointer[len]){
                len++;
                if(len + now == data_len || len==0xff){
                    break;
                }
            }
            struct CompressSegment cs = {
                .len = len,
                .diff = (u_int16_t)m_off,
            };
            this->directWriteToBuffer((const char*)(&cs),sizeof(cs));
            this->dict[dindex] = now + this->bufferOffset;
            now += len;
        }
        // auto tmp = new_header->caplen;
        new_header->caplen = (u_int32_t)(this->bufferOffset - origin);
        // printf("origin: %u, now: %u\n",tmp,new_header->caplen);
    }
    // just for test
    std::string getBuffer(){
        std::string ret = std::string(this->buffer,this->bufferOffset);
        return ret;
    }
};

#endif