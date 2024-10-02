#include "../dpdk_lib/header.hpp"
#include "../dpdk_lib/compressor.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <lzo/lzo1x.h>
#include <zlib.h>
#define DROP_LEN 12

#pragma pack(push, 1) // 保证结构体按1字节对齐

// PCAP全局文件头结构（24字节）
struct PcapFileHeader {
    uint32_t magic_number;   // 魔数，用于标识文件格式，通常是0xA1B2C3D4
    uint16_t version_major;  // 主版本号
    uint16_t version_minor;  // 次版本号
    int32_t  thiszone;       // 本地时间偏移
    uint32_t sigfigs;        // 精度
    uint32_t snaplen;        // 最大抓包长度
    uint32_t network;        // 数据链路类型
};

struct Packet{
    pcap_header header;
    std::string data;
    u_int16_t drop;
};

#pragma pack(pop) // 恢复默认对齐

std::vector<Packet> read_packets(std::string fileName){
    u_int32_t file_len = 0;
    std::vector<Packet> ret = std::vector<Packet>();
    std::ifstream file(fileName, std::ios::binary);

    // 读取并解析全局头
    PcapFileHeader fileHeader;
    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));

    while (file) {
        pcap_header packetHeader;
        
        // 读取PCAP包头
        file.read(reinterpret_cast<char*>(&packetHeader), sizeof(packetHeader));

        if (!file) break; // 确保读取成功
        struct Packet packet = {
            .header = packetHeader,
            .data = std::string(),
            .drop = 0,
        };

        packet.data.resize(packetHeader.caplen - 14);
        file.seekg(14,std::ios::cur);
        file.read(reinterpret_cast<char*>(&packet.data[0]), packetHeader.caplen -14);
        struct ip_header* ip = (struct ip_header*)&packet.data[0];
        packet.drop = (u_int16_t)((u_int64_t)&(ip->ip_source_address) - (u_int64_t)ip);

        ret.push_back(packet);
        file_len += sizeof(packetHeader) + packet.data.size();
    }

    file.close();
    printf("ori len: %u\n",file_len);
    return ret;
}

std::string getCompress(std::vector<Packet> packets){
    char* buffer = new char[1024*1024*4];
    Compressor compressor(buffer,65536*4);
    auto start = std::chrono::high_resolution_clock::now();
    for(auto p:packets){
        // compressor.directWriteToBuffer((const char*)(&p.header),sizeof(p.header));
        compressor.compress(&p.header,p.data.c_str(),p.data.size(),p.drop,DROP_LEN);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    printf("time: %lu\n",duration_time);
    return compressor.getBuffer();
}

void LZOCompress(std::string data){
    if (lzo_init() != LZO_E_OK) {
        std::cerr << "LZO 初始化失败!" << std::endl;
        return;
    }
    auto start = std::chrono::high_resolution_clock::now();
    lzo_uint compressed_size = data.size() + data.size() / 16 + 64 + 3; // 公式由 LZO 官方推荐
    unsigned char *compressed_data = new unsigned char[compressed_size];
    unsigned char *wrkmem = new unsigned char[LZO1X_1_MEM_COMPRESS];

    // 压缩数据
    int result = lzo1x_1_compress((const unsigned char *)data.c_str(), data.size(), compressed_data, &compressed_size, wrkmem);
    if (result != LZO_E_OK) {
        std::cerr << "压缩失败: " << result << std::endl;
        delete[] compressed_data;
        delete[] wrkmem;
        return;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    printf("time: %lu\n",duration_time);

    std::cout << "压缩成功! 压缩后大小: " << compressed_size << std::endl;

    // 释放内存
    delete[] compressed_data;
    delete[] wrkmem;
}

void GZIPCompress(std::string data){
    // 输入数据
    const char *input_data = data.c_str();

    uLong input_size = data.size(); // 包括 '\0' 结束符

    // 计算压缩缓冲区大小（通常比输入数据大一点以容纳压缩后的数据）
    uLong compressed_size = compressBound(input_size);
    unsigned char *compressed_data = new unsigned char[compressed_size];

    auto start = std::chrono::high_resolution_clock::now();
    // 压缩数据
    int result = compress(compressed_data, &compressed_size, (const unsigned char *)input_data, input_size);
    if (result != Z_OK) {
        std::cerr << "压缩失败: " << result << std::endl;
        delete[] compressed_data;
        return;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    printf("time: %lu\n",duration_time);

    std::cout << "压缩成功! 压缩后大小: " << compressed_size << std::endl;

    // 释放内存
    delete[] compressed_data;

    return;
}

int main(){
    auto ret = read_packets("./data/source/pcap.pcap");
    int* byte_count = new int[256]();

    auto str = getCompress(ret);

    printf("now len: %zu\n",str.size());

    std::string data = std::string();

    for(auto p:ret){
        data += std::string((char*)(&p.header),sizeof(p.header));
        data += p.data;
    }

    LZOCompress(data);
    GZIPCompress(data);

    LZOCompress(str);
    GZIPCompress(str);


    // for(auto p:ret){
    //     for(auto c:p){
    //         byte_count[c]++;
    //     }
    // }

    // for(u_int32_t i=0;i<256;++i){
    //     printf("%u: %d\n",i,byte_count[i]);
    // }
    
    return 0;
}