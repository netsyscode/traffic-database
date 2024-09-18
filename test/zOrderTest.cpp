#include "../dpdk_lib/skipList.hpp"
#include "../dpdk_lib/zOrderTree.hpp"
#include "../dpdk_lib/memoryBuffer.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <arpa/inet.h> // for inet_pton
#include <regex>
#include <random>

struct TestIndex{
    u_int32_t srcip;
    u_int32_t dstip;
    u_int16_t srcport;
    u_int16_t dstport;
};

// 将IPv4地址转换为32位无符号整型
uint32_t ipToUint32(const std::string& ip) {
    uint32_t result = 0;
    inet_pton(AF_INET, ip.c_str(), &result); // 将IP转换为无符号整型
    return ntohl(result); // 将网络字节序转换为主机字节序
}

void readFile(SkipList* skipList){
    std::string filename = "./data/source/flow.txt";
    std::ifstream infile(filename);
    std::string line;

    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::regex flowRegex(R"(\('([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)',\s*(\d+),\s*'([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)',\s*(\d+)\):\s*(\d+))");
    std::smatch match;

    u_int64_t count = 0;

    while (std::getline(infile, line)) {
        if (std::regex_search(line, match, flowRegex) && match.size() == 6) {
            std::string srcIp = match[1];
            uint16_t srcPort = static_cast<uint16_t>(std::stoi(match[2]));
            std::string dstIp = match[3];
            uint16_t dstPort = static_cast<uint16_t>(std::stoi(match[4]));
            size_t len = static_cast<size_t>(std::stoull(match[5]));

            uint32_t srcIpInt = ipToUint32(srcIp);
            uint32_t dstIpInt = ipToUint32(dstIp);

            Index index = {
                .meta = {
                    .sourceAddress = std::string((char*)(&srcIpInt),sizeof(srcIpInt)),
                    .destinationAddress = std::string((char*)(&dstIpInt),sizeof(dstIpInt)),
                    .sourcePort = srcPort,
                    .destinationPort = dstPort,
                },
                .ts = 0,
                .value = count,
            };
            ZOrderIPv4 zorder(&index);
            skipList->insert(std::string((char*)&zorder,sizeof(zorder)),count,std::numeric_limits<uint64_t>::max());

            // std::cout << "srcIp: " << srcIp << " (" << srcIpInt << "), "
            //           << "srcPort: " << srcPort << ", "
            //           << "dstIp: " << dstIp << " (" << dstIpInt << "), "
            //           << "dstPort: " << dstPort << ", "
            //           << "len: " << len << std::endl;
            count++;
            if(count >=1024){
                break;
            }
        }else {
            std::cerr << "Line format error: " << line << std::endl;
        }
    }

    infile.close();
}

void writeFile(SkipList* skipList){
    auto zorder_list = skipList->outputToZOrderIPv4();
    u_int64_t num = skipList->getNodeNum();
    ZOrderTreeIPv4 tree(zorder_list,num);
    tree.buildTree();

    // printf("%lu\n",zorder_list[1631761].value);

    std::string fileName = "./data/index/test_" + std::to_string(num) + ".offset";
    printf("%s\n",fileName.c_str());

    u_int64_t block_size = num * sizeof(ZOrderIPv4Meta);

    int fileFD = open(fileName.c_str(), O_WRONLY | O_CREAT, 0644);
    if (fileFD == -1) {
        printf("indexStorage error: failed to open file of zorder_list!\n");
    }
    ssize_t bytes_written = write(fileFD, (char*)zorder_list , block_size);
    // ssize_t bytes_written = pwrite(fileFD, (char*)zorder_list , block_size, 0);
    if (bytes_written == -1) {
        printf("indexStorage error: failed to pwrite zorder_list!\n");
    }
    close(fileFD);

    u_int64_t buffer_size = tree.getBufferSize();
    fileName = "./data/index/test_" + std::to_string(buffer_size) + "_" + std::to_string(tree.getMaxLevel()) + ".idx";
    const char* data = tree.outputToChar();
    block_size = buffer_size * sizeof(ZOrderTreeNode);
    fileFD = open(fileName.c_str(), O_WRONLY | O_CREAT, 0644);
    if (fileFD == -1) {
        printf("Index Storage error: failed to open file of rtree!\n");
    }
    bytes_written = write(fileFD, data, block_size);
    if (bytes_written == -1) {
        printf("IndexStorage error: failed to pwrite rtree!\n");
    }
    close(fileFD);
}

void buildOneDim(){
    SkipList srcip_list(sizeof(u_int32_t)*8,sizeof(u_int32_t),sizeof(u_int64_t));
    SkipList dstip_list(sizeof(u_int32_t)*8,sizeof(u_int32_t),sizeof(u_int64_t));
    SkipList srcport_list(sizeof(u_int16_t)*8,sizeof(u_int16_t),sizeof(u_int64_t));
    SkipList dstport_list(sizeof(u_int16_t)*8,sizeof(u_int16_t),sizeof(u_int64_t));

    std::string filename = "./data/source/flow.txt";
    std::ifstream infile(filename);
    std::string line;

    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::regex flowRegex(R"(\('([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)',\s*(\d+),\s*'([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)',\s*(\d+)\):\s*(\d+))");
    std::smatch match;

    u_int32_t count = 0;

    while (std::getline(infile, line)) {
        if (std::regex_search(line, match, flowRegex) && match.size() == 6) {
            std::string srcIp = match[1];
            uint16_t srcPort = static_cast<uint16_t>(std::stoi(match[2]));
            std::string dstIp = match[3];
            uint16_t dstPort = static_cast<uint16_t>(std::stoi(match[4]));
            size_t len = static_cast<size_t>(std::stoull(match[5]));

            uint32_t srcIpInt = ipToUint32(srcIp);
            uint32_t dstIpInt = ipToUint32(dstIp);

            srcip_list.insert(std::string((char*)(&srcIpInt),sizeof(srcIpInt)),count,std::numeric_limits<uint64_t>::max());
            dstip_list.insert(std::string((char*)(&dstIpInt),sizeof(dstIpInt)),count,std::numeric_limits<uint64_t>::max());
            srcport_list.insert(std::string((char*)(&srcPort),sizeof(srcPort)),count,std::numeric_limits<uint64_t>::max());
            dstport_list.insert(std::string((char*)(&dstPort),sizeof(dstPort)),count,std::numeric_limits<uint64_t>::max());

            count++;
        }else {
            std::cerr << "Line format error: " << line << std::endl;
        }
    }

    infile.close();

    std::cout << "read done" << std::endl;

    std::string fileName = "./data/index/test.pcap_srcip_idx";
    std::string data = srcip_list.outputToChar();
    int fileFD = open(fileName.c_str(), O_WRONLY | O_CREAT, 0644);
    ssize_t bytes_written = write(fileFD,data.c_str(),data.size());
    close(fileFD);

    fileName = "./data/index/test.pcap_dstip_idx";
    data = srcip_list.outputToChar();
    fileFD = open(fileName.c_str(), O_WRONLY | O_CREAT, 0644);
    bytes_written = write(fileFD,data.c_str(),data.size());
    close(fileFD);

    fileName = "./data/index/test.pcap_srcport_idx";
    data = srcport_list.outputToChar();
    fileFD = open(fileName.c_str(), O_WRONLY | O_CREAT, 0644);
    bytes_written = write(fileFD,data.c_str(),data.size());
    close(fileFD);

    fileName = "./data/index/test.pcap_dstport_idx";
    data = dstport_list.outputToChar();
    fileFD = open(fileName.c_str(), O_WRONLY | O_CREAT, 0644);
    bytes_written = write(fileFD,data.c_str(),data.size());
    close(fileFD);
}

void queryZOrder(std::vector<TestIndex>& vec){
    std::string idx_filename = "./data/index/test_13627283_24.idx";
    std::string offset_filename = "./data/index/test_1631762.offset";
    // std::string idx_filename = "./data/index/test_24_24.idx";
    // std::string offset_filename = "./data/index/test_1.offset";
    // std::string idx_filename = "./data/index/test_198_24.idx";
    // std::string offset_filename = "./data/index/test_1024.offset";

    std::vector<ZOrderIPv4Meta> zorder_list = std::vector<ZOrderIPv4Meta>();
    zorder_list.resize(1631762);
    // zorder_list.resize(1);
    // zorder_list.resize(1024);
    std::ifstream infile(offset_filename, std::ios::binary);

    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << offset_filename << std::endl;
        return;
    }

    infile.read(reinterpret_cast<char*>(zorder_list.data()), zorder_list.size()*sizeof(ZOrderIPv4Meta));
    infile.close();

    // printf("%u\n",zorder_list[1631761].value);

    // std::random_device rd; // 用于生成种子
    // std::mt19937 generator(rd());

    ZOrderTreeIPv4 tree(zorder_list.data(),zorder_list.size());
    // printf("%llu\n",zorder_list.size());
    tree.buildTree();

    // std::uniform_int_distribution<uint32_t> distribution(0, UINT32_MAX);

    auto start = std::chrono::high_resolution_clock::now();
    for(auto id:vec){
        // u_int32_t srcip = distribution(generator);
        // u_int32_t dstip = distribution(generator);
        // u_int16_t srcport = distribution(generator) % UINT16_MAX;
        // u_int16_t dstport = distribution(generator) % UINT16_MAX;

        u_int64_t src = ((u_int64_t)id.srcip << 16) + id.srcport;
        u_int64_t dst = ((u_int64_t)id.dstip << 16) + id.dstport;

        // printf("%llu, %u\n",tree.getBufferSize(),tree.getMaxLevel());
        auto ret = tree.search(src,dst,src,dst);
        // printf("%lu\n",*ret.begin());
        // printf("%lu\n",zorder_list[*ret.begin()].value);
        // break;
    }

    auto end = std::chrono::high_resolution_clock::now();
    
}

std::vector<TestIndex> readIndex(){
    std::string filename = "./data/source/flow.txt";
    std::ifstream infile(filename);
    std::string line;

    std::vector<TestIndex> vec = std::vector<TestIndex>();

    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return vec;
    }

    std::regex flowRegex(R"(\('([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)',\s*(\d+),\s*'([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)',\s*(\d+)\):\s*(\d+))");
    std::smatch match;

    u_int32_t count = 0;

    while (std::getline(infile, line)) {
        if (std::regex_search(line, match, flowRegex) && match.size() == 6) {
            std::string srcIp = match[1];
            uint16_t srcPort = static_cast<uint16_t>(std::stoi(match[2]));
            std::string dstIp = match[3];
            uint16_t dstPort = static_cast<uint16_t>(std::stoi(match[4]));
            size_t len = static_cast<size_t>(std::stoull(match[5]));

            uint32_t srcIpInt = ipToUint32(srcIp);
            uint32_t dstIpInt = ipToUint32(dstIp);

            TestIndex id = {
                .srcip = srcIpInt,
                .dstip = dstIpInt,
                .srcport = srcPort,
                .dstport = dstPort,
            };
            vec.push_back(id);

        }else {
            std::cerr << "Line format error: " << line << std::endl;
        }
    }

    infile.close();
    return vec;
}

void writeVec(std::vector<TestIndex>& vec){
    std::string filename = "./data/index/test.vec";
    std::ofstream outfile(filename, std::ios::binary); // 以二进制模式打开文件

    if (!outfile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    // 写入 vector 数据到文件
    outfile.write(reinterpret_cast<const char*>(vec.data()), vec.size() * sizeof(TestIndex));

    outfile.close();
}

int main(){
    // SkipList* skipList = new SkipList(sizeof(ZOrderIPv4)*8,sizeof(ZOrderIPv4),sizeof(u_int64_t));
    // readFile(skipList);
    // std::cout << "read done" << std::endl;
    // writeFile(skipList);

    std::vector<TestIndex> vec = std::vector<TestIndex>();
    vec.resize(1631762);
    std::ifstream infile("./data/index/test.vec", std::ios::binary);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << "./data/index/test.vec" << std::endl;
    }
    infile.read(reinterpret_cast<char*>(vec.data()), vec.size()*sizeof(ZOrderIPv4));
    infile.close();
    std::cout << "read done" << std::endl;
    queryZOrder(vec);
    return 0;
}
