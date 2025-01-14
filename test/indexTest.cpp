#include "../dpdk_lib/skipList.hpp"
#include "../dpdk_lib/pointerRingBuffer.hpp"
#include "../dpdk_lib/indexBlock.hpp"
#include "../dpdk_lib/TrieTree.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <arpa/inet.h> // for inet_pton
#include <regex>
#include <random>
#include <thread>
#include <map>

#define IPV4_SIZE 1631762
#define IPV6_SIZE 93501

struct TestIndexIPv4{
    u_int32_t srcip;
    u_int32_t dstip;
    u_int16_t srcport;
    u_int16_t dstport;
};

struct TestIndexIPv6{
    IPv6Address srcip;
    IPv6Address dstip;
    u_int16_t srcport;
    u_int16_t dstport;
};

// 将IPv4地址转换为32位无符号整型
uint32_t ipToUint32(const std::string& ip) {
    uint32_t result = 0;
    inet_pton(AF_INET, ip.c_str(), &result); // 将IP转换为无符号整型
    return ntohl(result); // 将网络字节序转换为主机字节序
}

IPv6Address ipToUint128(const std::string& ip) {
    struct in6_addr addr;
    
    // 使用inet_pton将IPv6字符串转换为二进制形式
    inet_pton(AF_INET6, ip.c_str(), &addr);

    // 将前64位和后64位分离为两个64位整数
    IPv6Address ipv6 ={
        .high = 0,
        .low = 0,
    };
    
    // addr.s6_addr 是一个 128 字节数组，按顺序存储 IPv6 地址的每一个字节
    for (int i = 0; i < 8; ++i) {
        ipv6.high = (ipv6.high << 8) | (addr.s6_addr[i]);
    }
    for (int i = 8; i < 16; ++i) {
        ipv6.low = (ipv6.low << 8) | (addr.s6_addr[i]);
    }

    return ipv6;
}


std::vector<TestIndexIPv6> readIndex(){
    std::string filename = "./data/source/flow_ipv6.txt";
    std::ifstream infile(filename);
    std::string line;

    std::vector<TestIndexIPv6> vec = std::vector<TestIndexIPv6>();

    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return vec;
    }

    std::regex flowRegex(R"(\('([0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+)',\s*(\d+),\s*'([0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+:[0-9a-f]+)',\s*(\d+)\):\s*(\d+))");
    std::smatch match;

    u_int32_t count = 0;

    while (std::getline(infile, line)) {
        if (std::regex_search(line, match, flowRegex) && match.size() == 6) {
            std::string srcIp = match[1];
            uint16_t srcPort = static_cast<uint16_t>(std::stoi(match[2]));
            std::string dstIp = match[3];
            uint16_t dstPort = static_cast<uint16_t>(std::stoi(match[4]));
            size_t len = static_cast<size_t>(std::stoull(match[5]));

            IPv6Address srcIpInt = ipToUint128(srcIp);
            IPv6Address dstIpInt = ipToUint128(dstIp);

            TestIndexIPv6 id = {
                .srcip = srcIpInt,
                .dstip = dstIpInt,
                .srcport = srcPort,
                .dstport = dstPort,
            };
            vec.push_back(id);

            // printf("%s\n",srcIp.c_str());
            // printf("%lu,%lu\n",srcIpInt.high,srcIpInt.low);

        }else {
            std::cerr << "Line format error: " << line << std::endl;
        }
    }

    infile.close();
    return vec;
}

void writeVecIPv6(std::vector<TestIndexIPv6>& vec){
    std::string filename = "./data/index/test_v6.vec";
    std::ofstream outfile(filename, std::ios::binary); // 以二进制模式打开文件

    if (!outfile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    // 写入 vector 数据到文件
    outfile.write(reinterpret_cast<const char*>(vec.data()), vec.size() * sizeof(TestIndexIPv6));

    outfile.close();
}

std::vector<TestIndexIPv4> vec_ipv4;
std::vector<TestIndexIPv6> vec_ipv6;

std::vector<PointerRingBuffer*>* rings;
std::vector<SkipList*>* lists;
// std::vector<TrieTree*>* trees;
// std::vector<std::multimap<u_int16_t,u_int64_t>*>* maps;

void thread_write(bool is_v4, u_int32_t begin, u_int32_t num){
    Index* index;
    if(is_v4){
        for(u_int32_t i=begin;i<begin+num;++i){
            // if(i >= begin + 90000){
            //     break;
            // }
            auto meta = vec_ipv4[i];

            index = new Index();
            index->key = std::string((char*)&(meta.srcip),sizeof(meta.srcip));
            index->value = i;
            index->ts = 0;
            index->id = IndexType::SRCIP;
            index->len = sizeof(meta.srcip);
            if(!(*rings)[0]->put((void*)index)){
                break;
            }

            index = new Index();
            index->key = std::string((char*)&(meta.dstip),sizeof(meta.dstip));
            index->value = i;
            index->ts = 0;
            index->id = IndexType::DSTIP;
            index->len = sizeof(meta.dstip);
            if(!(*rings)[1]->put((void*)index)){
                break;
            }

            index = new Index();
            index->key = std::string((char*)&(meta.srcport),sizeof(meta.srcport));
            index->value = i;
            index->ts = 0;
            index->id = IndexType::SRCPORT;
            index->len = sizeof(meta.srcport);
            if(!(*rings)[2]->put((void*)index)){
                break;
            }

            index = new Index();
            index->key = std::string((char*)&(meta.dstport),sizeof(meta.dstport));
            index->value = i;
            index->ts = 0;
            index->id = IndexType::DSTPORT;
            index->len = sizeof(meta.dstport);
            if(!(*rings)[3]->put((void*)index)){
                break;
            }


            QuarTurpleIPv4 id_meta = {
                .srcip = meta.srcip,
                .dstip = meta.dstip,
                .srcport = meta.srcport,
                .dstport = meta.dstport,
            };
            index = new Index();
            index->key = std::string((char*)&(id_meta),sizeof(id_meta));
            index->value = i;
            index->ts = 0;
            index->id = IndexType::QUARTURPLEIPv4;
            index->len = sizeof(id_meta);
            if(!(*rings)[6]->put((void*)index)){
                break;
            }
        }
    }else{
        for(u_int32_t i=begin;i<begin+num;++i){
            auto meta = vec_ipv6[i];

            index = new Index();
            index->key = std::string((char*)&(meta.srcip),sizeof(meta.srcip));
            index->value = i;
            index->ts = 0;
            index->id = IndexType::SRCIPv6;
            index->len = sizeof(meta.srcip);
            if(!(*rings)[4]->put((void*)index)){
                break;
            }

            index = new Index();
            index->key = std::string((char*)&(meta.dstip),sizeof(meta.dstip));
            index->value = i;
            index->ts = 0;
            index->id = IndexType::DSTIPv6;
            index->len = sizeof(meta.dstip);
            if(!(*rings)[5]->put((void*)index)){
                break;
            }

            index = new Index();
            index->key = std::string((char*)&(meta.srcport),sizeof(meta.srcport));
            index->value = i;
            index->ts = 0;
            index->id = IndexType::SRCPORT;
            index->len = sizeof(meta.srcport);
            if(!(*rings)[2]->put((void*)index)){
                break;
            }

            index = new Index();
            index->key = std::string((char*)&(meta.dstport),sizeof(meta.dstport));
            index->value = i;
            index->ts = 0;
            index->id = IndexType::DSTPORT;
            index->len = sizeof(meta.dstport);
            if(!(*rings)[3]->put((void*)index)){
                break;
            }

            QuarTurpleIPv6 id_meta = {
                .srcip = meta.srcip,
                .dstip = meta.dstip,
                .srcport = meta.srcport,
                .dstport = meta.dstport,
            };

            index = new Index();
            index->key = std::string((char*)&(id_meta),sizeof(id_meta));
            index->value = i;
            index->ts = 0;
            index->id = IndexType::QUARTURPLEIPv6;
            index->len = sizeof(id_meta);
            if(!(*rings)[7]->put((void*)index)){
                break;
            }
        }
    }
    printf("write down.\n");
}

void thread_read(u_int32_t cpu){
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    pthread_t thread = pthread_self();

    int set_result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (set_result != 0) {
        std::cerr << "Error setting thread affinity: " << set_result << std::endl;
    }

    // 确认设置是否成功
    CPU_ZERO(&cpuset);
    pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

    if (CPU_ISSET(cpu, &cpuset)) {
        printf("thread %lu bind to cpu %d.\n",thread,cpu);
    } else {
        printf("thread %lu failed to bind to cpu %d!\n",thread,cpu);
    }

    u_int32_t id = (cpu-10)/2;

    u_int64_t duration_time = 0;

    u_int64_t count = 0;

    while (true){
        void* data = (*rings)[id]->get();
        if(data==nullptr){
            break;
        }
        Index* index = (Index*)data;

        auto start = std::chrono::high_resolution_clock::now();
    

        (*lists)[index->id]->insert(index->key,index->value,std::numeric_limits<uint64_t>::max());

        // (*trees)[index->id]->insert(index->key,index->value);
        
        auto end = std::chrono::high_resolution_clock::now();
        delete (u_int32_t*)data;
        count++;
        duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    printf("Thread %u: thread quit, during %lu us with %lu indexes.\n",id,duration_time,count);
}

void query(u_int32_t id, IndexBlockCompact& ib){
    std::string key = std::string();
    u_int64_t duration_time = 0;
    printf("ID %u.\n",id);
    for(u_int32_t i = 0;i<10000;++i){
        switch (id){
        case 0:
            key = std::string((char*)&vec_ipv4[i].srcip,sizeof(vec_ipv4[i].srcip));
            break;
        case 1:
            key = std::string((char*)&vec_ipv4[i].dstip,sizeof(vec_ipv4[i].dstip));
            break;
        case 2:
            key = std::string((char*)&vec_ipv4[i].srcport,sizeof(vec_ipv4[i].srcport));
            break;
        case 3:
            key = std::string((char*)&vec_ipv4[i].dstport,sizeof(vec_ipv4[i].dstport));
            break;
        case 4:
            key = std::string((char*)&vec_ipv6[i].srcip,sizeof(vec_ipv6[i].srcip));
            break;
        case 5:
            key = std::string((char*)&vec_ipv6[i].dstip,sizeof(vec_ipv6[i].dstip));
            break;
        case 6:
            key = std::string((char*)&vec_ipv4[i],sizeof(vec_ipv4[i]));
            break;
        case 7:
            key = std::string((char*)&vec_ipv6[i],sizeof(vec_ipv6[i]));
            break;
        default:
            break;
        }

        auto start = std::chrono::high_resolution_clock::now();
        auto ret = ib.query(key);
        auto end = std::chrono::high_resolution_clock::now();
        duration_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        printf("%lu %zu\n",duration_time,ret.size());
    }
    // printf("ID %u: time %lu us.\n",id,duration_time);
}

void query(u_int32_t id, IndexBlock& ib){
    std::string key = std::string();
    u_int64_t duration_time = 0;
    QuarTurpleIPv4 quar_v4;
    QuarTurpleIPv6 quar_v6;
    printf("ID %u.\n",id);
    for(u_int32_t i = 0;i<10000;++i){
        switch (id){
        case 0:
            key = std::string((char*)&vec_ipv4[i].srcip,sizeof(vec_ipv4[i].srcip));
            break;
        case 1:
            key = std::string((char*)&vec_ipv4[i].dstip,sizeof(vec_ipv4[i].dstip));
            break;
        case 2:
            key = std::string((char*)&vec_ipv4[i].srcport,sizeof(vec_ipv4[i].srcport));
            break;
        case 3:
            key = std::string((char*)&vec_ipv4[i].dstport,sizeof(vec_ipv4[i].dstport));
            break;
        case 4:
            key = std::string((char*)&vec_ipv6[i].srcip,sizeof(vec_ipv6[i].srcip));
            break;
        case 5:
            key = std::string((char*)&vec_ipv6[i].dstip,sizeof(vec_ipv6[i].dstip));
            break;
        case 6:
            quar_v4 = {
                .srcip = vec_ipv4[i].srcip,
                .dstip = vec_ipv4[i].dstip,
                .srcport = vec_ipv4[i].srcport,
                .dstport = vec_ipv4[i].dstport,
            };
            key = std::string((char*)&quar_v4,sizeof(quar_v4));
            break;
        case 7:
            quar_v6 = {
                .srcip = vec_ipv6[i].srcip,
                .dstip = vec_ipv6[i].dstip,
                .srcport = vec_ipv6[i].srcport,
                .dstport = vec_ipv6[i].dstport,
            };
            key = std::string((char*)&quar_v6,sizeof(quar_v6));
            break;
        default:
            break;
        }

        auto start = std::chrono::high_resolution_clock::now();
        auto ret = ib.query(key);
        auto end = std::chrono::high_resolution_clock::now();
        duration_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        printf("%lu %zu\n",duration_time,ret.size());

        // if(id >= 6 && ret.size()!=1){
        //     printf("id:%u, size:%zu, keysize:%zu\n",id,ret.size(),key.size());
        // }
    }
    
}

void queryFalse(u_int32_t id, IndexBlock& ib, IndexBlockCompact& ibt){
    std::string key = std::string();
    u_int64_t duration_time = 0;
    QuarTurpleIPv4 quar_v4;
    QuarTurpleIPv6 quar_v6;
    printf("ID %u.\n",id);

    std::random_device rd;  // 用于获得种子
    std::mt19937 gen(rd()); // 使用随机设备种子初始化Mersenne Twister生成器
    std::uniform_int_distribution<u_int64_t> distrib(0, UINT64_MAX);

    srand((unsigned int)time(NULL));

    u_int32_t count = 0;
    for(u_int32_t i = 0;i<10000000;++i){
        IPv6Address ip ={
            .high = distrib(gen),
            .low = distrib(gen),
        };
        switch (id){
        case 4:
            key = std::string((char*)&ip,sizeof(vec_ipv6[i].srcip));
            break;
        case 5:
            key = std::string((char*)&ip,sizeof(vec_ipv6[i].dstip));
            break;
        case 6:
            quar_v4 = {
                .srcip = (u_int32_t)distrib(gen),
                .dstip = (u_int32_t)distrib(gen),
                .srcport = (u_int16_t)distrib(gen),
                .dstport = (u_int16_t)distrib(gen),
            };
            key = std::string((char*)&quar_v4,sizeof(quar_v4));
            break;
        case 7:
            quar_v6 = {
                .srcip = ip,
                .dstip = ip,
                .srcport = (u_int16_t)distrib(gen),
                .dstport = (u_int16_t)distrib(gen),
            };
            key = std::string((char*)&quar_v6,sizeof(quar_v6));
            break;
        default:
            break;
        }

        auto start = std::chrono::high_resolution_clock::now();
        auto ret = ib.query(key);
        auto end = std::chrono::high_resolution_clock::now();
        duration_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        auto ret_t = ibt.query(key);

        if(ret.size()!=ret_t.size()){
            count++;
        }

        // printf("%lu %zu\n",duration_time,ret.size());

        // if(id >= 6 && ret.size()!=1){
        //     printf("id:%u, size:%zu, keysize:%zu\n",id,ret.size(),key.size());
        // }
    }
    printf("%u\n",count);
}

void queryRange(u_int32_t id, IndexBlockCompact& ib){
    std::string key_left = std::string();
    std::string key_right = std::string();
    u_int32_t ip_left = 0;
    u_int32_t ip_right = 0;
    u_int16_t port_left = 0;
    u_int16_t port_right = 0;
    u_int64_t duration_time = 0;
    printf("ID %u.\n",id);
    for(u_int32_t i = 0;i<10000;++i){
        switch (id){
        case 0:
            ip_left = vec_ipv4[i].srcip & (u_int32_t)0xffffff00;
            ip_right = vec_ipv4[i].srcip | (u_int32_t)0xff;
            // printf("left:%u,right:%u\n",ip_left,ip_right);
            key_left = std::string((char*)&ip_left,sizeof(ip_left));
            key_right = std::string((char*)&ip_right,sizeof(ip_right));
            break;
        case 1:
            ip_left = vec_ipv4[i].dstip & (u_int32_t)0xffffff00;
            ip_right = vec_ipv4[i].dstip | (u_int32_t)0xff;
            key_left = std::string((char*)&ip_left,sizeof(ip_left));
            key_right = std::string((char*)&ip_right,sizeof(ip_right));
            break;
        case 2:
            port_left = (u_int16_t)min(vec_ipv4[i*2].srcport,vec_ipv4[i*2+1].srcport);
            port_right = (u_int16_t)min(vec_ipv4[i*2].srcport,vec_ipv4[i*2+1].srcport);
            key_left = std::string((char*)&port_left,sizeof(port_left));
            key_right = std::string((char*)&port_right,sizeof(port_right));
            break;
        case 3:
            port_left = (u_int16_t)min(vec_ipv4[i*2].dstport,vec_ipv4[i*2+1].dstport);
            port_right = (u_int16_t)min(vec_ipv4[i*2].dstport,vec_ipv4[i*2+1].dstport);
            key_left = std::string((char*)&port_left,sizeof(port_left));
            key_right = std::string((char*)&port_right,sizeof(port_right));
            break;
        // case 4:
        //     key = std::string((char*)&vec_ipv6[i].srcip,sizeof(vec_ipv6[i].srcip));
        //     break;
        // case 5:
        //     key = std::string((char*)&vec_ipv6[i].dstip,sizeof(vec_ipv6[i].dstip));
        //     break;
        // case 6:
        //     key = std::string((char*)&vec_ipv4[i],sizeof(vec_ipv4[i]));
        //     break;
        // case 7:
        //     key = std::string((char*)&vec_ipv6[i],sizeof(vec_ipv6[i]));
        //     break;
        default:
            break;
        }

        auto start = std::chrono::high_resolution_clock::now();
        auto ret = ib.queryRange(key_left,key_right);
        auto end = std::chrono::high_resolution_clock::now();
        duration_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        printf("%lu %zu\n",duration_time,ret.size());
    }
    // printf("ID %u: time %lu us.\n",id,duration_time);
}

void queryRange(u_int32_t id, IndexBlock& ib){
    std::string key = std::string();
    u_int64_t duration_time = 0;
    QuarTurpleIPv4 quar_v4;
    QuarTurpleIPv6 quar_v6;
    if(id != 4 && id != 5){
        return;
    }
    printf("ID %u.\n",id);
    for(u_int32_t i = 0;i<10000;++i){
        switch (id){
        // case 0:
        //     key = std::string((char*)&vec_ipv4[i].srcip,sizeof(vec_ipv4[i].srcip));
        //     break;
        // case 1:
        //     key = std::string((char*)&vec_ipv4[i].dstip,sizeof(vec_ipv4[i].dstip));
        //     break;
        // case 2:
        //     key = std::string((char*)&vec_ipv4[i].srcport,sizeof(vec_ipv4[i].srcport));
        //     break;
        // case 3:
        //     key = std::string((char*)&vec_ipv4[i].dstport,sizeof(vec_ipv4[i].dstport));
        //     break;
        case 4:
            key = std::string(((char*)&vec_ipv6[i].srcip)+sizeof(PRE_TYPE),sizeof(vec_ipv6[i].srcip)-sizeof(PRE_TYPE));
            
            break;
        case 5:
            key = std::string(((char*)&vec_ipv6[i].dstip)+sizeof(PRE_TYPE),sizeof(vec_ipv6[i].dstip)-sizeof(PRE_TYPE));
            break;
        // case 6:
        //     quar_v4 = {
        //         .srcip = vec_ipv4[i].srcip,
        //         .dstip = vec_ipv4[i].dstip,
        //         .srcport = vec_ipv4[i].srcport,
        //         .dstport = vec_ipv4[i].dstport,
        //     };
        //     key = std::string((char*)&quar_v4,sizeof(quar_v4));
        //     break;
        // case 7:
        //     quar_v6 = {
        //         .srcip = vec_ipv6[i].srcip,
        //         .dstip = vec_ipv6[i].dstip,
        //         .srcport = vec_ipv6[i].srcport,
        //         .dstport = vec_ipv6[i].dstport,
        //     };
        //     key = std::string((char*)&quar_v6,sizeof(quar_v6));
        //     break;
        default:
            break;
        }

        auto start = std::chrono::high_resolution_clock::now();
        auto ret = ib.queryPrefix(key);
        auto end = std::chrono::high_resolution_clock::now();
        duration_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        printf("%lu %zu\n",duration_time,ret.size());

        // if(id >= 6 && ret.size()!=1){
        //     printf("id:%u, size:%zu, keysize:%zu\n",id,ret.size(),key.size());
        // }
    }
    
}

void intersect(std::list<u_int64_t>& la, std::list<u_int64_t>& lb){
    la.sort();
    lb.sort();
    auto ita = la.begin();
    auto itb = lb.begin();
    while (ita != la.end() && itb != lb.end()) {
        if (*ita < *itb) {
            ita = la.erase(ita);
        } else if (*ita > *itb) {
            ++itb;
        } else {
            ++ita;
            ++itb;
        }
    }
    la.erase(ita,la.end());
}

// void combineTest(){
//     auto srcipv4_str = (*lists)[0]->outputToCharCompact();
//     auto dstipv4_str = (*lists)[1]->outputToCharCompact();
//     auto srcport_str = (*lists)[2]->outputToCharCompact();
//     auto dstport_str = (*lists)[3]->outputToCharCompact();
//     auto srcipv6_str = (*lists)[4]->outputToCharCompressedInt();
//     auto dstipv6_str = (*lists)[5]->outputToCharCompressedInt();
//     auto quaripv4_str = (*lists)[6]->outputToCharCompressedInt();
//     auto quaripv6_str = (*lists)[7]->outputToCharCompressedInt();

//     IndexBlockCompact srcipv4_ib(4,(char*)&srcipv4_str[0],srcipv4_str.size());
//     IndexBlockCompact dstipv4_ib(4,(char*)&dstipv4_str[0],dstipv4_str.size());
//     IndexBlockCompact srcport_ib(2,(char*)&srcport_str[0],srcport_str.size());
//     IndexBlockCompact dstport_ib(2,(char*)&dstport_str[0],dstport_str.size());
//     IndexBlock srcipv6_ib(16,(char*)&srcipv6_str[0],srcipv6_str.size());
//     IndexBlock dstipv6_ib(16,(char*)&dstipv6_str[0],dstipv6_str.size());
//     IndexBlock quaripv4_ib(12,(char*)&quaripv4_str[0],quaripv4_str.size());
//     IndexBlock quaripv6_ib(36,(char*)&quaripv6_str[0],quaripv6_str.size());

//     printf("ipv4\n");
//     for(u_int32_t i = 0; i<10000;++i){
//         auto start = std::chrono::high_resolution_clock::now();

//         auto srcip_ret = srcipv4_ib.query(std::string((char*)&vec_ipv4[i].srcip,sizeof(vec_ipv4[i].srcip)));
//         auto dstip_ret = dstipv4_ib.query(std::string((char*)&vec_ipv4[i].dstip,sizeof(vec_ipv4[i].dstip)));
//         auto srcport_ret = srcport_ib.query(std::string((char*)&vec_ipv4[i].srcport,sizeof(vec_ipv4[i].srcport)));
//         auto dstport_ret = srcport_ib.query(std::string((char*)&vec_ipv4[i].dstport,sizeof(vec_ipv4[i].dstport)));
//         intersect(srcip_ret,dstip_ret);
//         intersect(srcip_ret,srcport_ret);
//         intersect(srcip_ret,dstport_ret);
        
//         auto end = std::chrono::high_resolution_clock::now();
//         // printf("ret:%u\n",srcip_ret.size());
//         auto duration_one = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
//         printf("%llu\n",duration_one);
//     }
//     printf("ipv6\n");
//     for(u_int32_t i = 0; i<10000;++i){
//         auto start = std::chrono::high_resolution_clock::now();

//         auto srcip_ret = srcipv6_ib.query(std::string((char*)&vec_ipv6[i].srcip,sizeof(vec_ipv6[i].srcip)));
//         auto dstip_ret = dstipv6_ib.query(std::string((char*)&vec_ipv6[i].dstip,sizeof(vec_ipv6[i].dstip)));
//         auto srcport_ret = srcport_ib.query(std::string((char*)&vec_ipv4[i].srcport,sizeof(vec_ipv4[i].srcport)));
//         auto dstport_ret = srcport_ib.query(std::string((char*)&vec_ipv4[i].dstport,sizeof(vec_ipv4[i].dstport)));
//         intersect(srcip_ret,dstip_ret);
//         intersect(srcip_ret,srcport_ret);
//         intersect(srcip_ret,dstport_ret);
        
//         auto end = std::chrono::high_resolution_clock::now();
//         // printf("ret:%u\n",srcip_ret.size());
//         auto duration_one = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
//         printf("%llu\n",duration_one);
//     }
// }

template <class KeyType>
std::list<u_int64_t> binarySearch(char* index, u_int32_t index_len, KeyType key){
    std::list<u_int64_t> ret = std::list<u_int64_t>();
    u_int32_t ele_len = sizeof(KeyType) + sizeof(u_int64_t);
    // printf("ele:%u\n",ele_len);
    u_int32_t left = 0;
    u_int32_t right = index_len/ele_len;

    while (left < right) {
        
        u_int32_t mid = left + (right - left) / 2;
        // printf("mid:%u\n",mid);

        KeyType key_mid = *(KeyType*)(index + mid * ele_len);

        // QuarTurpleIPv4 id = *(QuarTurpleIPv4*)(index + mid * ele_len);
        // printf("%u,%u,%u,%u\n",id.srcip,id.dstip,id.srcport,id.dstport);

        if (key_mid < key) {
            // printf("left\n");
            left = mid + 1;
        } else {
            // printf("right\n");
            right = mid;
        }
    }
    for(;left<index_len/ele_len;left++){
        KeyType key_now = *(KeyType*)(index + left * ele_len);
        if(key_now != key){
            break;
        }
        u_int64_t value = *(u_int64_t*)(index + left * ele_len + sizeof(KeyType));
        ret.push_back(value);
    }
    return ret;
}

int main(){
    // auto vec = readIndex();
    // printf("read done.\n");
    // writeVecIPv6(vec);
    vec_ipv4 = std::vector<TestIndexIPv4>();
    vec_ipv4.resize(IPV4_SIZE);
    std::ifstream infile_v4("./data/index/test_v4.vec", std::ios::binary);
    if (!infile_v4.is_open()) {
        std::cerr << "Error opening file: " << "./data/index/test_v4.vec" << std::endl;
    }
    infile_v4.read(reinterpret_cast<char*>(vec_ipv4.data()), vec_ipv4.size()*sizeof(TestIndexIPv4));
    infile_v4.close();

    vec_ipv6 = std::vector<TestIndexIPv6>();
    vec_ipv6.resize(IPV6_SIZE);
    std::ifstream infile_v6("./data/index/test_v6.vec", std::ios::binary);
    if (!infile_v6.is_open()) {
        std::cerr << "Error opening file: " << "./data/index/test_v6.vec" << std::endl;
    }
    infile_v6.read(reinterpret_cast<char*>(vec_ipv6.data()), vec_ipv6.size()*sizeof(TestIndexIPv6));
    infile_v6.close();

    rings = new std::vector<PointerRingBuffer*>();
    lists = new std::vector<SkipList*>();
    // trees = new std::vector<TrieTree*>();

    for(u_int32_t i=0;i<flowMetaEleLens.size();++i){
        PointerRingBuffer* ir =  new PointerRingBuffer(1024*1024);
        rings->push_back(ir);
    }
    

    for(u_int32_t i=0;i<flowMetaEleLens.size();++i){
        SkipList* li = new SkipList(flowMetaEleLens[i]*8,flowMetaEleLens[i],sizeof(u_int64_t));
        lists->push_back(li);

        // TrieTree* tree = new TrieTree(flowMetaEleLens[i],flowMetaEleLens[i],sizeof(u_int64_t));
        // trees->push_back(tree);
    }

    std::vector<std::thread*> read_threads = std::vector<std::thread*>();
    std::vector<std::thread*> write_threads = std::vector<std::thread*>();

    for(u_int32_t i=0;i<8;++i){
        std::thread* rt = new std::thread(&thread_read,i*2+10);
        read_threads.push_back(rt);
    }

    for(u_int32_t i=0;i<16;++i){
        std::thread* wt = new std::thread(&thread_write,true,IPV6_SIZE*i,IPV6_SIZE);
        write_threads.push_back(wt);
    }

    for(u_int32_t i=0;i<1;++i){
        std::thread* wt = new std::thread(&thread_write,false,IPV6_SIZE*i,IPV6_SIZE);
        write_threads.push_back(wt);
    }

    for(auto t:write_threads){
        t->join();
        printf("stop.\n");
    }

    for(u_int32_t i=0;i<flowMetaEleLens.size();++i){
        (*rings)[i]->asynchronousStop();
    }
    

    printf("should stop\n");

    for(auto t:read_threads){
        t->join();
    }

    

    std::string s1;
    std::string s2;
    std::string s3;
    u_int32_t id = 0;
    for(auto sk: *lists){
        s1 = sk->outputToChar();
        // printf("%u\n",sk->getNodeNum());
        if(sk->getKeyLen()<=4){
            auto start = std::chrono::high_resolution_clock::now();
        
            s2 = sk->outputToCharCompact();
            auto end = std::chrono::high_resolution_clock::now();
            auto duration_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            // printf("time:%lu\n",duration_time);

            IndexBlockCompact ib(sk->getKeyLen(),(char*)&s2[0],s2.size());
            queryRange(id,ib);
            
        }else{
            auto start = std::chrono::high_resolution_clock::now();
            // s2 = sk->outputToCharCompact();
            s2 = sk->outputToCharCompressedIntBloom();
            s3 = sk->outputToCharCompact();
            auto end = std::chrono::high_resolution_clock::now();
            auto duration_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            // printf("time:%lu\n",duration_time);

            IndexBlock ib(sk->getKeyLen(),&s2[0],s2.size());
            // IndexBlockCompact ibt(sk->getKeyLen(),(char*)&s3[0],s3.size());
            queryRange(id,ib);

            // query(id,ib);
            // queryFalse(id,ib,ibt);
            
        }
        id++;
        // printf("s1:%zu, s2:%zu\n",s1.size(),s2.size());
    }

    // combineTest();

    return 0;
}
