#include "../dpdk_lib/skipList.hpp"
#include "../dpdk_lib/zOrderTree.hpp"
#include <fstream>

struct TestIndex{
    u_int32_t srcip;
    u_int32_t dstip;
    u_int16_t srcport;
    u_int16_t dstport;
};

int main(){
    // SkipList* sk = new SkipList(sizeof(u_int32_t)*8,sizeof(u_int32_t),sizeof(u_int64_t));
    // u_int32_t key[3] = {0xcbc2b4b0, 0xcbc2b78a, 0xcbc40a85};
    // u_int64_t value[3] = {0,1,2};
    // sk->insert(std::string((char*)&key[0],sizeof(u_int32_t)),value[0],std::numeric_limits<uint64_t>::max());
    // sk->insert(std::string((char*)&key[2],sizeof(u_int32_t)),value[1],std::numeric_limits<uint64_t>::max());
    // sk->insert(std::string((char*)&key[1],sizeof(u_int32_t)),value[2],std::numeric_limits<uint64_t>::max());
    // sk->insert(std::string((char*)&key[2],sizeof(u_int32_t)),value[2],std::numeric_limits<uint64_t>::max());
    // sk->insert(std::string((char*)&key[1],sizeof(u_int32_t)),value[1],std::numeric_limits<uint64_t>::max());

    // std::string ret = sk->outputToCharCompressed();

    // for(auto c:ret){
    //     printf("%02x",(u_int8_t)c);
    // }
    // printf("\n");

    SkipList* sk = new SkipList(sizeof(ZOrderIPv4)*8,sizeof(ZOrderIPv4),sizeof(u_int64_t));
    // SkipList* sk = new SkipList(sizeof(u_int32_t)*8,sizeof(u_int32_t),sizeof(u_int64_t));

    std::vector<TestIndex> vec = std::vector<TestIndex>();
    vec.resize(1631762);
    std::ifstream infile("./data/index/test.vec", std::ios::binary);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << "./data/index/test.vec" << std::endl;
    }
    infile.read(reinterpret_cast<char*>(vec.data()), vec.size()*sizeof(ZOrderIPv4));
    infile.close();
    std::cout << "read done" << std::endl;

    u_int64_t count = 0;
    for(auto t:vec){
        IndexTMP index = {
            .meta = {
                .sourceAddress = std::string((char*)(&t.srcip),sizeof(t.srcip)),
                .destinationAddress = std::string((char*)(&t.dstip),sizeof(t.dstip)),
                .sourcePort = t.srcport,
                .destinationPort = t.dstport,
            },
            .ts = 0,
            .value = count,
        };
        count++;
        ZOrderIPv4 zorder(&index);
        std::string id = std::string();
        id += index.meta.destinationAddress;
        id += index.meta.sourceAddress;
        id += std::string((char*)&index.meta.sourcePort,sizeof(index.meta.sourcePort));
        id += std::string((char*)&index.meta.destinationPort,sizeof(index.meta.destinationPort));
        sk->insert(std::string((char*)&zorder,sizeof(zorder)),index.value,std::numeric_limits<uint64_t>::max());
        // sk->insert(id,index.value,std::numeric_limits<uint64_t>::max());
        // sk->insert(std::string((char*)&index.meta.sourcePort,sizeof(index.meta.sourcePort)),index.value,std::numeric_limits<uint64_t>::max());
    }
    printf("insert done.\n");

    auto start = std::chrono::high_resolution_clock::now();
    std::string ret = sk->outputToCharCompressed();
    // std::string ret = sk->outputToChar();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    printf("time: %lu\n",duration_time);
    printf("%lu\n",ret.size());

    return 0;
}