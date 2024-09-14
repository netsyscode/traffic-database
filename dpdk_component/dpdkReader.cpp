#include "dpdkReader.hpp"

const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0xff,0xff,0x00,0x00,0x01,0x00,0x00,0x00};

u_int8_t hasher_32(u_int32_t value){
    u_int8_t hashValue = 0;
    for(int i = 0;i<sizeof(u_int32_t)/sizeof(u_int8_t);++i){
        hashValue ^= value & 0xff;
        value >>= sizeof(u_int8_t);
    }
    return hashValue;
}

u_int8_t hasher_16(u_int16_t value){
    u_int8_t hashValue = 0;
    for(int i = 0;i<sizeof(u_int16_t)/sizeof(u_int8_t);++i){
        hashValue ^= value & 0xff;
        value >>= sizeof(u_int8_t);
    }
    return hashValue;
}

void DPDKReader::readPacket(struct rte_mbuf *buf, u_int64_t ts, PacketMeta* meta){
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
    const struct ip_header* ip_protocol = (const struct ip_header *)(meta.data + this->eth_header_len);
    const u_int16_t* sport = (const u_int16_t*)(meta.data + this->eth_header_len + ip_protocol->ip_header_length * 4);
    const u_int16_t* dport = sport + 1;
    FlowMetadata flow_meta = {
        .sourceAddress = htonl(ip_protocol->ip_source_address),
        .destinationAddress = htonl(ip_protocol->ip_destination_address),
        .sourcePort = htons(*sport),
        .destinationPort = htons(*dport),
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

bool DPDKReader::writeIndexToRing(u_int64_t value, PacketMeta meta){
    // auto index_vec = get_index(meta,this->eth_header_len,value);
    // int i=0;
    // for(auto x:index_vec){
    //     if(!(*(this->indexRings))[i]->put((void*)x)){
    //         return false;
    //     }
    //     i++;
    // }
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
            u_int64_t last = this->packetAggregator->addPacket(flow_meta,_offset,ts);
            if(last != std::numeric_limits<uint64_t>::max()){
                // printf("%llu\n",last);
                u_int32_t diff = (u_int32_t)(_offset - last);
                this->packetBuffer->writeBefore((const char*)(&diff),sizeof(diff),last);
            }

            // const HeaderInfo* info = (const HeaderInfo*)meta.data;

            // printf("packet offset: %llu, l3 offset: %u, l4 offset: %u.\n",_offset,info->l3_offset,info->l4_offset);
            
            /* with index */
            // u_int64_t value = this->calValue(_offset);
            // if(!this->writeIndexToRing(value,meta)){
            //     printf("DPDK Reader error: write index to ring failed!\n");
            // }
            // delete meta.header;
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
        // if(this->pause){
        //     auto start_truncate = std::chrono::high_resolution_clock::now();
        //     this->truncate();
        //     auto end_truncate = std::chrono::high_resolution_clock::now();
        //     truncate_time += std::chrono::duration_cast<std::chrono::microseconds>(end_truncate - start_truncate).count();
        // }
        
        // if(this->packetBuffer->getWarning()){
        //     this->monitor_cv->notify_all();
        // }
    }
    auto end = std::chrono::high_resolution_clock::now();

    this->duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // while(true){// wait
    //     if(this->stop){
    //         std::cout << "DPDK Reader log: asynchronous stop." << std::endl;
    //         break;
    //     }
    //     if(this->pause){
    //         this->truncate();
    //     }
    // }
    // rte_eal_cleanup();
    printf("DPDK Reader log: thread quit, during %llu us with %llu packets.\n",this->duration_time,pkt_count);
    return 0;
}

void DPDKReader::asynchronousStop(){
    this->stop = true;
}

// void DPDKReader::asynchronousPause(ArrayList<u_int64_t>* newpacketPointer){
//     this->newpacketPointer = newpacketPointer;
//     this->pause = true;
// }

// bool DPDKReader::getPause() const{
//     return this->pause;
// }