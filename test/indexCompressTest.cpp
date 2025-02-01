#include "../dpdk_lib/skipList.hpp"
#include "../dpdk_lib/zOrderTree.hpp"
#include "../dpdk_lib/indexBlock.hpp"
#include <fstream>
#include <unordered_set>

#define IPV4_SIZE 1631762
#define IPV6_SIZE 93501
#define ISCX_SIZE 28338

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

    // SkipList* sk = new SkipList(sizeof(QuarTurpleIPv4)*8,sizeof(QuarTurpleIPv4),sizeof(u_int64_t));
    // SkipList* sk = new SkipList(sizeof(u_int32_t)*8,sizeof(u_int32_t),sizeof(u_int64_t));
    // SkipList* sk = new SkipList(sizeof(u_int16_t)*8,sizeof(u_int16_t),sizeof(u_int64_t));
    SkipList* sk = new SkipList(sizeof(u_int64_t)*8,sizeof(u_int64_t),sizeof(u_int64_t));

    std::vector<u_int64_t> vec = std::vector<u_int64_t>();
    vec.resize(IPV4_SIZE);
    std::ifstream infile("./data/index/wide_flags.vec", std::ios::binary);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << "./data/index/test.vec" << std::endl;
    }
    infile.read(reinterpret_cast<char*>(vec.data()), vec.size()*sizeof(u_int64_t));
    infile.close();
    std::cout << "read done" << std::endl;

    u_int64_t count = 0;
    std::vector<QuarTurpleIPv4> keys = std::vector<QuarTurpleIPv4>();
    // std::unordered_set<uint32_t> unique_dst_ips;
    // QuarTurpleIPv4 key;
    for(auto t:vec){
        // unique_dst_ips.insert(t.srcport);
        // if(count == 10){
        //     break;
        // }
        // IndexTMP index = {
        //     .meta = {
        //         .sourceAddress = std::string((char*)(&t.srcip),sizeof(t.srcip)),
        //         .destinationAddress = std::string((char*)(&t.dstip),sizeof(t.dstip)),
        //         .sourcePort = t.srcport,
        //         .destinationPort = t.dstport,
        //     },
        //     .ts = 0,
        //     .value = count,
        // };
        count++;
        // QuarTurpleIPv4 zorder(&index);
        // keys.push_back(zorder);
        // if (count == 10000){
        //     key = zorder;
        // }

        // QuarTurpleIPv4 quar = {
        //     .srcip = t.srcip,
        //     .dstip = t.dstip,
        //     .srcport = t.srcport,
        //     .dstport = t.dstport,
        // };

        //keys.push_back(quar);

        // std::string id = std::string();
        // id += std::string((char*)&index.meta.sourcePort,sizeof(index.meta.sourcePort));
        // id += std::string((char*)&index.meta.destinationPort,sizeof(index.meta.destinationPort));
        // id += index.meta.destinationAddress;
        // id += index.meta.sourceAddress;
        
        // sk->insert(std::string((char*)&quar,sizeof(quar)),index.value,std::numeric_limits<uint64_t>::max());
        // sk->insert(id,index.value,std::numeric_limits<uint64_t>::max());
        // sk->insert(index.meta.destinationAddress,index.value,std::numeric_limits<uint64_t>::max());
        // sk->insert(std::string((char*)&index.meta.destinationAddress,sizeof(index.meta.destinationAddress)),index.value,std::numeric_limits<uint64_t>::max());
        // sk->insert(std::string((char*)&index.meta.destinationPort,sizeof(index.meta.sourcePort)),index.value,std::numeric_limits<uint64_t>::max());
        sk->insert(std::string((char*)&t,sizeof(t)),count,std::numeric_limits<uint64_t>::max());
    }
    printf("insert done.\n");
    // std::cout << "不同的目的 IP 数量: " << unique_dst_ips.size() << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    // std::string ret = sk->outputToCharCompressed();

    std::string ret = sk->outputToCharCompressedInt();
    std::string ret_com = sk->outputToCharCompact();
    std::string ret_ori = sk->outputToChar();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    printf("time: %lu\n",duration_time);
    printf("compressed %lu, compacted %lu, Origin %lu\n",ret.size(),ret_com.size(),ret_ori.size());

    // IndexBlock block(sizeof(QuarTurpleIPv4),&ret[0],ret.size());
    // duration_time = 0;

    // // auto res = block.query(std::string((char*)&keys[311927],sizeof(QuarTurpleIPv4)));
    // // printf("len of ret: %zu, res:%lu.\n",res.size(),res.front());

    // count = 0;
    // for(auto key:keys){
    //     start = std::chrono::high_resolution_clock::now();
    //     auto res = block.query(std::string((char*)&key,sizeof(key)));
    //     end = std::chrono::high_resolution_clock::now();
    //     duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //     // if(res.size()!=1){
    //     //     printf("id:%lu, len of ret: %zu, res:%lu.\n",count,res.size(),res.front());
    //     // }
    //     count++;
    //     if(count == 10000){
    //         printf("end\n");
    //         break;
    //     }
    // }
    // printf("time: %lu\n",duration_time);
    return 0;
}