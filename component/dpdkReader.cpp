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

struct PacketMetaTurple{
    u_int32_t srcIP;
    u_int32_t dstIP;
    u_int16_t srcPort;
    u_int16_t dstPort;
};

bool DPDKReader::fileInit(){
    std::ofstream file(this->fileName, std::ios::app);
    if (!file.is_open()) {
        printf("DPDK reader error: fileInit failed while openning file!\n");
        return false;
    }
    file.clear();
    file.close();
    this->packetBuffer->changeFileName(this->fileName);
    this->packetBuffer->openFile();
    this->packetBuffer->writePointer((char*)pcap_head,this->pcap_header_len);
    // file.write((const char*)pcap_head,this->pcap_header_len);
    // printf("offset:%u\n",this->pcap_header_len);
    // file.close();
    // this->packetBuffer->changeFileOffset(this->pcap_header_len);
    return true;
}

PacketMeta DPDKReader::readPacket(struct rte_mbuf *buf, u_int64_t ts){
    PacketMeta meta = {
        .data = nullptr,
        .len = 0,
    };
    data_header header;
    // struct timeval tv;
    // gettimeofday(&tv, NULL);
    header.len = rte_pktmbuf_pkt_len(buf);
    header.caplen = rte_pktmbuf_data_len(buf);
    header.ts_l = (u_int32_t)(ts & 0xFFFFFFFF);
    header.ts_h = (u_int32_t)(ts >> 32);

    meta.len = header.caplen + sizeof(struct data_header);
    meta.data = new char[meta.len];
    memcpy(meta.data, (char*)&header, sizeof(struct data_header));
    memcpy(meta.data+sizeof(struct data_header), rte_pktmbuf_mtod(buf, const char *), header.caplen);

    rte_pktmbuf_free(buf);

    return meta;
}

u_int32_t DPDKReader::writePacketToPacketBuffer(PacketMeta& meta){
    if(!this->packetBuffer->writePointer(meta.data,meta.len)){
        return std::numeric_limits<uint32_t>::max();
    }
    return (u_int32_t)(this->packetBuffer->getOffset() + this->packetBuffer->getLength());
}

u_int8_t DPDKReader::calPacketID(PacketMeta& meta){
    PacketMetaTurple meta_turple;
    ip_header* ip_protocol = (ip_header*)(meta.data + sizeof(struct data_header) + this->eth_header_len);
    int ip_header_length = ip_protocol->ip_header_length;
    int ip_prot = ip_protocol->ip_protocol;
    meta_turple.srcIP = htonl(ip_protocol->ip_source_address);
    meta_turple.dstIP = htonl(ip_protocol->ip_destination_address);

    if(ip_prot == 6){
        tcp_header* tcp_protocol = (tcp_header*)(meta.data + sizeof(struct data_header) + this->eth_header_len + ip_header_length * 4);
        meta_turple.srcPort = htons(tcp_protocol->tcp_source_port);
        meta_turple.dstPort = htons(tcp_protocol->tcp_destination_port);
    }else if(ip_prot == 17){
        udp_header* udp_protocol = (udp_header*)(meta.data + sizeof(struct data_header) + this->eth_header_len + ip_header_length * 4);
        meta_turple.srcPort = htons(udp_protocol->udp_source_port);
        meta_turple.dstPort = htons(udp_protocol->udp_destination_port);
    }else{
        return 0;
    }

    u_int8_t id = hasher_32(meta_turple.srcIP)^hasher_32(meta_turple.dstIP)^
                    hasher_16(meta_turple.srcPort)^hasher_16(meta_turple.dstPort);
    return id;
}
    
u_int32_t DPDKReader::writePacketToPacketPointer(u_int32_t _offset, u_int8_t id){
    u_int64_t pos = (u_int64_t)(this->port_id);
    pos <<= sizeof(u_int16_t)*8;
    pos += (u_int64_t)(this->rx_id);
    pos <<= sizeof(u_int32_t)*8;
    pos += (u_int64_t)_offset;
    return this->packetPointer->addNodeMultiThread(pos,id);
}

void DPDKReader::truncate(){
    if(this->newpacketPointer == nullptr){
        std::cerr << "DPDK Reader error: trancate without new memory!" << std::endl;
        this->pause = false;
        this->monitor_cv->notify_all();
        return;
    }
    // this->packetBuffer = this->newPacketBuffer;
    this->packetPointer = this->newpacketPointer;
    // this->newPacketBuffer = nullptr;
    this->newpacketPointer = nullptr;
    // this->packetBuffer->writeOneThread((const char*)pcap_head,this->pcap_header_len);
    this->pause = false;
    this->monitor_cv->notify_all();
}

int DPDKReader::run(){
    if(!this->fileInit()){
        return -1;
    }
    std::cout << "DPDK reader log: thread run." << std::endl;
    this->stop = false;
    this->pause = false;
    //align
    // this->packetBuffer->writeOneThread((const char*)pcap_head,this->pcap_header_len);
    
    auto start = std::chrono::high_resolution_clock::now();
    u_int64_t truncate_time = 0;

    struct rte_mbuf *bufs[BURST_SIZE];
    int nb_rx;
    u_int64_t ts;
    u_int64_t pkt_count = 0;
    
    while(true){
        ts = rte_rdtsc();
        nb_rx = this->dpdk->getRXBurst(bufs,this->port_id,this->rx_id);
        // nb_rx = rte_eth_rx_burst(this->port_id, this->rx_id, bufs, BURST_SIZE);
        if(nb_rx == 0 && !(this->stop) && !(this->pause)){
            continue;
        }
        int err = 0;
        for(int i=0;i<nb_rx;++i){
            pkt_count ++;
            // printf("DPDK Reader log: get one.\n");
            PacketMeta meta = this->readPacket(bufs[i],ts);
            if(meta.data == nullptr){
                std::cout << "DPDK Reader log: read over." << std::endl;
                err = 1;
                break;
            }
            u_int32_t _offset = this->writePacketToPacketBuffer(meta);
            if(_offset == std::numeric_limits<uint32_t>::max()){
                std::cerr << "DPDK Reader error: packet buffer overflow!" << std::endl;
                err = 1;
                break;
            }
            u_int8_t id = this->calPacketID(meta);
            // u_int32_t nodeNum = this->writePacketToPacketPointer(_offset,id);
            // if(nodeNum == std::numeric_limits<uint32_t>::max()){
            //     std::cerr << "DPDK Reader error: packet pointer overflow!" << std::endl;
            //     err = 1;
            //     break;
            // }
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

void DPDKReader::asynchronousPause(ArrayList<u_int64_t>* newpacketPointer){
    this->newpacketPointer = newpacketPointer;
    this->pause = true;
}

bool DPDKReader::getPause() const{
    return this->pause;
}