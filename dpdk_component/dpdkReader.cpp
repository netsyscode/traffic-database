#include "dpdkReader.hpp"
#include <regex>
#include <random>

const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0xff,0xff,0x00,0x00,0x01,0x00,0x00,0x00};

// u_int8_t hasher_32(u_int32_t value){
//     u_int8_t hashValue = 0;
//     for(int i = 0;i<sizeof(u_int32_t)/sizeof(u_int8_t);++i){
//         hashValue ^= value & 0xff;
//         value >>= sizeof(u_int8_t);
//     }
//     return hashValue;
// }

// u_int8_t hasher_16(u_int16_t value){
//     u_int8_t hashValue = 0;
//     for(int i = 0;i<sizeof(u_int16_t)/sizeof(u_int8_t);++i){
//         hashValue ^= value & 0xff;
//         value >>= sizeof(u_int8_t);
//     }
//     return hashValue;
// }

uint64_t swap_endianness(uint64_t value) {
    return ((value >> 56) & 0x00000000000000FFULL) | // byte 0
           ((value >> 40) & 0x000000000000FF00ULL) | // byte 1
           ((value >> 24) & 0x00000000FF000000ULL) | // byte 2
           ((value >> 8)  & 0x00FF000000000000ULL) | // byte 3
           ((value << 8)  & 0xFF00000000000000ULL) | // byte 4
           ((value << 24) & 0x0000FF0000000000ULL) | // byte 5
           ((value << 40) & 0x000000FF00000000ULL) | // byte 6
           ((value << 56) & 0x00000000000000FFULL);   // byte 7
}

void DPDKReader::readPacket(struct rte_mbuf *buf, u_int64_t ts, PacketMeta* meta){
    // printf("tag:%03x\n",buf->dynfield1[0]);
    meta->header->flow_next_diff = std::numeric_limits<uint32_t>::max();
    meta->header->caplen = rte_pktmbuf_data_len(buf) - this->eth_header_len;
    meta->header->ts_l = (u_int32_t)(ts & 0xFFFFFFFF);
    meta->header->ts_h = (u_int32_t)(ts >> 32);

    meta->len = meta->header->caplen;
    meta->data = rte_pktmbuf_mtod(buf, const char *);
}

u_int64_t DPDKReader::writePacketToPacketBuffer(PacketMeta& meta){
    this->packetBuffer->writePointer((const char*)meta.header,sizeof(pcap_header));
    this->packetBuffer->writePointer(meta.data + this->eth_header_len, meta.len);
    return (u_int32_t)(this->packetBuffer->getFileOffset() + this->packetBuffer->getOffset()) - meta.len - sizeof(pcap_header);
}

FlowMetadata DPDKReader::getFlowMetaData(PacketMeta& meta){
    uint8_t version = (*(u_int8_t*)(meta.data + this->eth_header_len) >> 4) & 0x0F;
    if(version == 4){
        const struct ip_header* ip_protocol = (const struct ip_header *)(meta.data + this->eth_header_len);
        const u_int16_t* sport = (const u_int16_t*)(meta.data + this->eth_header_len + ip_protocol->ip_header_length * 4);
        const u_int16_t* dport = sport + 1;
        u_int32_t srcip = htonl(ip_protocol->ip_source_address);
        u_int32_t dstip = htonl(ip_protocol->ip_destination_address);
        FlowMetadata flow_meta = {
            .sourceAddress = std::string((char*)&srcip,sizeof(srcip)),
            .destinationAddress = std::string((char*)&dstip,sizeof(dstip)),
            .sourcePort = htons(*sport),
            .destinationPort = htons(*dport),
        };
        return flow_meta;
    }else if(version == 6){
        const u_int16_t* sport = (const u_int16_t*)(meta.data + this->eth_header_len + IPV6_HEADER_LEN);
        const u_int16_t* dport = sport + 1;
        IPv6Address srcip = {
            .high = swap_endianness(*(u_int64_t*)(meta.data + this->eth_header_len + 8)),
            .low = swap_endianness(*(u_int64_t*)(meta.data + this->eth_header_len + 16)),
        };
        IPv6Address dstip = {
            .high = swap_endianness(*(u_int64_t*)(meta.data + this->eth_header_len + 24)),
            .low = swap_endianness(*(u_int64_t*)(meta.data + this->eth_header_len + 32)),
        };
        FlowMetadata flow_meta = {
            .sourceAddress = std::string((char*)&srcip,sizeof(srcip)),
            .destinationAddress = std::string((char*)&dstip,sizeof(dstip)),
            .sourcePort = htons(*sport),
            .destinationPort = htons(*dport),
        };
        return flow_meta;
    }
    FlowMetadata flow_meta = {
        .sourceAddress = std::string(),
        .destinationAddress = std::string(),
        .sourcePort = 0,
        .destinationPort = 0,
    };
    return flow_meta;
}

u_int64_t DPDKReader::calValue(u_int64_t _offset){
    u_int64_t value = 0;
    value |= this->port_id & 0xff;
    value <<= 8;
    value |= this->rx_id & 0xff;
    value <<= 48;
    value |= _offset & 0xffffffffffff;
    // printf("offset:%llu.\n",_offset);
    return value;
}

bool DPDKReader::writeIndexToRing(u_int64_t value, FlowMetadata meta, u_int64_t ts){
    Index* index = new Index();
    // index->key = *(u_int32_t*)(meta.sourceAddress.c_str());
    index->key = meta.sourceAddress;
    index->value = value;
    index->ts = ts;
    index->id = meta.sourceAddress.size() == 4? IndexType::SRCIP:IndexType::SRCIPv6;
    index->len = meta.sourceAddress.size();
    if(!(*(this->indexRings))[0]->put((void*)index)){
        return false;
    }

    index = new Index();
    // index->key =  *(u_int32_t*)(meta.destinationAddress.c_str());
    index->key = meta.destinationAddress;
    index->value = value;
    index->ts = ts;
    index->id = meta.sourceAddress.size() == 4? IndexType::DSTIP:IndexType::DSTIPv6;
    index->len = meta.sourceAddress.size();
    if(!(*(this->indexRings))[0]->put((void*)index)){
        return false;
    }

    index = new Index();
    // index->key = meta.sourcePort;
    index->key = std::string((char*)&(meta.sourcePort),sizeof(meta.sourcePort));
    index->value = value;
    index->ts = ts;
    index->id = IndexType::SRCPORT;
    index->len = sizeof(meta.sourcePort);
    if(!(*(this->indexRings))[0]->put((void*)index)){
        return false;
    }

    index = new Index();
    // index->key = meta.destinationPort;
    index->key = std::string((char*)&(meta.destinationPort),sizeof(meta.destinationPort));
    index->value = value;
    index->ts = ts;
    index->id = IndexType::DSTPORT;
    index->len = sizeof(meta.sourcePort);
    if(!(*(this->indexRings))[0]->put((void*)index)){
        return false;
    }

    if (meta.sourceAddress.size() == 4){
        index = new Index();
        // index->key = meta.destinationPort;
        QuarTurpleIPv4 ipv4Turple = {
            .srcip = *(u_int32_t*)(meta.sourceAddress.c_str()),
            .dstip = *(u_int32_t*)(meta.destinationAddress.c_str()),
            .srcport = meta.sourcePort,
            .dstport = meta.destinationPort,
        };
        index->key = std::string((char*)&(ipv4Turple),sizeof(ipv4Turple));
        index->value = value;
        index->ts = ts;
        index->id = IndexType::QUARTURPLEIPv4;
        index->len = sizeof(ipv4Turple);
        if(!(*(this->indexRings))[0]->put((void*)index)){
            return false;
        }
    }else{
        index = new Index();
        // index->key = meta.destinationPort;
        QuarTurpleIPv6 ipv6Turple = {
            .srcip = *(IPv6Address*)(meta.sourceAddress.c_str()),
            .dstip = *(IPv6Address*)(meta.destinationAddress.c_str()),
            .srcport = meta.sourcePort,
            .dstport = meta.destinationPort,
        };
        index->key = std::string((char*)&(ipv6Turple),sizeof(ipv6Turple));
        index->value = value;
        index->ts = ts;
        index->id = IndexType::QUARTURPLEIPv6;
        index->len = sizeof(ipv6Turple);
        if(!(*(this->indexRings))[0]->put((void*)index)){
            return false;
        }
    }
    return true;
}

int DPDKReader::run(){
    // pcap file header
    this->packetBuffer->writePointer((char*)pcap_head,this->pcap_header_len);

    std::cout << "DPDK reader log: thread run." << std::endl;
    this->stop = false;
    
    u_int64_t truncate_time = 0;

    struct rte_mbuf *bufs[BURST_SIZE];
    int nb_rx;
    u_int64_t ts;
    u_int64_t pkt_count = 0;
    u_int64_t index_count = 0;
    auto start = std::chrono::high_resolution_clock::now();
    bool has_start = false;
    PacketMeta meta = {
        .header = new array_list_header,
        .data = nullptr,
        .len = 0,
    };
    
    while(true){
        ts = rte_rdtsc();
        nb_rx = this->dpdk->getRXBurst(bufs,this->port_id,this->rx_id);
        
        if(nb_rx == 0 && !(this->stop)){
            continue;
        }
        if(!has_start){
            start = std::chrono::high_resolution_clock::now();
            has_start=true;
        }
        int err = 0;
        for(int i=0;i<nb_rx;++i){
            pkt_count ++;
            
            this->readPacket(bufs[i],ts,&meta);
            
            if(meta.data == nullptr){
                std::cout << "DPDK Reader log: read over." << std::endl;
                err = 1;
                break;
            }

            u_int64_t _offset = this->writePacketToPacketBuffer(meta);
            if(_offset == std::numeric_limits<uint64_t>::max()){
                std::cerr << "DPDK Reader error: packet buffer overflow!" << std::endl;
                err = 1;
                break;
            }

            FlowMetadata flow_meta = this->getFlowMetaData(meta);
            if(flow_meta.sourceAddress.size() == 0){
                printf("DPDK Reader error: Non-IP L3 protocol!\n");
                meta.data = nullptr;
                rte_pktmbuf_free(bufs[i]);
                continue;
            }
            u_int64_t last = this->packetAggregator->addPacket(flow_meta,_offset,ts);
            if(last != std::numeric_limits<uint64_t>::max()){
                // printf("%llu\n",last);
                u_int32_t diff = (u_int32_t)(_offset - last);
                this->packetBuffer->writeBefore((const char*)(&diff),sizeof(diff),last);
            }else{
                /* with index */
                u_int64_t value = this->calValue(_offset);
                if(!this->writeIndexToRing(value,flow_meta,ts)){
                    printf("DPDK Reader error: write index to ring failed!\n");
                }
                index_count++;
            }

            // printf("packet offset: %llu, l3 offset: %u, l4 offset: %u.\n",_offset,info->l3_offset,info->l4_offset);
            
            meta.data = nullptr;
            rte_pktmbuf_free(bufs[i]);
        }
        nb_rx = 0;
        if(err){
            break;
        }
        if(this->stop){
            std::cout << "DPDK Reader log: asynchronous stop." << std::endl;
            break;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    this->duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    printf("DPDK Reader log: thread quit, during %llu us with %llu packets and %llu indexes.\n",this->duration_time,pkt_count,index_count);
    return 0;
}

// struct TestIndex{
//     u_int32_t srcip;
//     u_int32_t dstip;
//     u_int16_t srcport;
//     u_int16_t dstport;
// };

// uint32_t ipToUint32(const std::string& ip) {
//     uint32_t result = 0;
//     inet_pton(AF_INET, ip.c_str(), &result); // 将IP转换为无符号整型
//     return ntohl(result); // 将网络字节序转换为主机字节序
// }

// int DPDKReader::run(){
//     std::string filename = "./data/source/flow.txt";
//     std::ifstream infile(filename);
//     std::string line;

//     std::vector<TestIndex> vec = std::vector<TestIndex>();

//     if (!infile.is_open()) {
//         std::cerr << "Error opening file: " << filename << std::endl;
//         return  -1;
//     }

//     std::regex flowRegex(R"(\('([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)',\s*(\d+),\s*'([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)',\s*(\d+)\):\s*(\d+))");
//     std::smatch match;

//     u_int32_t count = 0;

//     while (std::getline(infile, line)) {
//         if (std::regex_search(line, match, flowRegex) && match.size() == 6) {
//             std::string srcIp = match[1];
//             uint16_t srcPort = static_cast<uint16_t>(std::stoi(match[2]));
//             std::string dstIp = match[3];
//             uint16_t dstPort = static_cast<uint16_t>(std::stoi(match[4]));
//             size_t len = static_cast<size_t>(std::stoull(match[5]));

//             uint32_t srcIpInt = ipToUint32(srcIp);
//             uint32_t dstIpInt = ipToUint32(dstIp);

//             TestIndex id = {
//                 .srcip = srcIpInt,
//                 .dstip = dstIpInt,
//                 .srcport = srcPort,
//                 .dstport = dstPort,
//             };
//             vec.push_back(id);

//         }else {
//             std::cerr << "Line format error: " << line << std::endl;
//         }
//     }
//     infile.close();
//     auto start = std::chrono::high_resolution_clock::now();
//     u_int64_t id_count = 0;
//     for(u_int32_t i = 0;i<2;++i){
//         for(auto id:vec){
            
//             FlowMetadata flow_meta = {
//                 .sourceAddress = std::string((char*)&id.srcip,sizeof(u_int32_t)),
//                 .destinationAddress = std::string((char*)&id.dstip,sizeof(u_int32_t)),
//                 .sourcePort = id.srcport,
//                 .destinationPort = id.dstport,
//             };
//             id.srcip++;
//             id.dstip++;
//             id.srcport++;
//             id.dstport++;
//             if(!this->writeIndexToRing(id_count,flow_meta,0)){
//                 printf("DPDK Reader error: write index to ring failed!\n");
//             }
//             id_count ++;
//             // if(id_count >= 800000){
//             //     break;
//             // }
//         }
//     }
//     auto end = std::chrono::high_resolution_clock::now();
//     this->duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
//     printf("DPDK Reader log: thread quit, during %llu us with %llu indexes.\n",this->duration_time,id_count);
//     return 0;
// }

void DPDKReader::asynchronousStop(){
    this->stop = true;
}
