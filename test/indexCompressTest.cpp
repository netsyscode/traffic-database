#include "../dpdk_lib/skipList.hpp"
#include "../dpdk_lib/zOrderTree.hpp"
#include "../dpdk_lib/indexBlock.hpp"
#include <fstream>

struct TestIndex{
    u_int32_t srcip;
    u_int32_t dstip;
    u_int16_t srcport;
    u_int16_t dstport;
};



int main(){
    // SkipList* sk = new SkipList(sizeof(u_int64_t)*8,sizeof(u_int64_t),sizeof(u_int64_t));
    // u_int64_t key[3] = {0xcbc2b4b0, 0xcbc2b78a, 0xcbc40a85};
    // u_int64_t value[3] = {0,1,2};
    // sk->insert(std::string((char*)&key[0],sizeof(u_int64_t)),value[0],std::numeric_limits<uint64_t>::max());
    // sk->insert(std::string((char*)&key[2],sizeof(u_int64_t)),value[1],std::numeric_limits<uint64_t>::max());
    // sk->insert(std::string((char*)&key[1],sizeof(u_int64_t)),value[2],std::numeric_limits<uint64_t>::max());
    // sk->insert(std::string((char*)&key[2],sizeof(u_int64_t)),value[2],std::numeric_limits<uint64_t>::max());
    // sk->insert(std::string((char*)&key[1],sizeof(u_int64_t)),value[1],std::numeric_limits<uint64_t>::max());

    // // std::string ret = sk->outputToCharCompressed();
    // std::string ret = sk->outputToCharCompressedInt();

    // for(auto c:ret){
    //     printf("%02x",(u_int8_t)c);
    // }
    // printf("\n");

    SkipList* sk = new SkipList(sizeof(QuarTurpleIPv4)*8,sizeof(QuarTurpleIPv4),sizeof(u_int64_t));
    // SkipList* sk = new SkipList(sizeof(u_int32_t)*8,sizeof(u_int32_t),sizeof(u_int64_t));

    std::vector<TestIndex> vec = std::vector<TestIndex>();
    vec.resize(1631762);
    std::ifstream infile("./data/index/test_v4.vec", std::ios::binary);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << "./data/index/test.vec" << std::endl;
    }
    infile.read(reinterpret_cast<char*>(vec.data()), vec.size()*sizeof(QuarTurpleIPv4));
    infile.close();
    std::cout << "read done" << std::endl;

    u_int64_t count = 0;
    std::vector<QuarTurpleIPv4> keys = std::vector<QuarTurpleIPv4>();
    // QuarTurpleIPv4 key;
    for(auto t:vec){
        // if(count == 10){
        //     break;
        // }
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
        // QuarTurpleIPv4 zorder(&index);
        // keys.push_back(zorder);
        // if (count == 10000){
        //     key = zorder;
        // }

        QuarTurpleIPv4 quar = {
            .srcip = t.srcip,
            .dstip = t.dstip,
            .srcport = t.srcport,
            .dstport = t.dstport,
        };

        keys.push_back(quar);

        // std::string id = std::string();
        // id += std::string((char*)&index.meta.sourcePort,sizeof(index.meta.sourcePort));
        // id += std::string((char*)&index.meta.destinationPort,sizeof(index.meta.destinationPort));
        // id += index.meta.destinationAddress;
        // id += index.meta.sourceAddress;
        
        sk->insert(std::string((char*)&quar,sizeof(quar)),index.value,std::numeric_limits<uint64_t>::max());
        // sk->insert(id,index.value,std::numeric_limits<uint64_t>::max());
        // sk->insert(index.meta.sourceAddress,index.value,std::numeric_limits<uint64_t>::max());
    }
    printf("insert done.\n");

    auto start = std::chrono::high_resolution_clock::now();
    // std::string ret = sk->outputToCharCompressed();

    std::string ret = sk->outputToCharCompressedInt();
    // std::string ret = sk->outputToCharCompact();
    // std::string ret = sk->outputToChar();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    printf("time: %lu\n",duration_time);
    printf("%lu\n",ret.size());

    IndexBlock block(sizeof(QuarTurpleIPv4),&ret[0],ret.size());
    duration_time = 0;

    // auto res = block.query(std::string((char*)&keys[311927],sizeof(QuarTurpleIPv4)));
    // printf("len of ret: %zu, res:%lu.\n",res.size(),res.front());

    count = 0;
    for(auto key:keys){
        start = std::chrono::high_resolution_clock::now();
        auto res = block.query(std::string((char*)&key,sizeof(key)));
        end = std::chrono::high_resolution_clock::now();
        duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        if(res.size()!=1){
            printf("id:%lu, len of ret: %zu, res:%lu.\n",count,res.size(),res.front());
        }
        count++;
        if(count == 10000){
            printf("end\n");
            break;
        }
    }
    printf("time: %lu\n",duration_time);
    return 0;
}