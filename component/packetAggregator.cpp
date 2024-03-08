#include "packetAggregator.hpp"

u_int32_t PacketAggregator::readFromPacketPointer(){
    if(this->threadID == std::numeric_limits<uint32_t>::max()){
        std::cerr << "Packet aggregator error: readFromPacketPointer without thread id!" << std::endl;
        return std::numeric_limits<uint32_t>::max();
    }
    IDData id_data;
    while(true){
        id_data = this->packetPointer->getID(this->readPos,this->threadID);
        if(id_data.err){
            return std::numeric_limits<uint32_t>::max();
        }
        this->readPos++;
        if(this->IDSet.find(id_data.data) != this->IDSet.end()){
            break;
        }
        if(this->stop){
            std::cout << "Packet aggregator log: asynchronous stop." << std::endl;
            return std::numeric_limits<uint32_t>::max();
        }
    }
    return this->packetPointer->getValue(readPos - 1,id_data.data);
}

char* PacketAggregator::readFromPacketBuffer(u_int32_t offset){
    return this->packetBuffer->readPcap(offset);
}

FlowMetadata PacketAggregator::parsePacket(char* packet){
    FlowMetadata meta;
    ip_header* ip_protocol = (ip_header*)(packet + sizeof(struct data_header) + this->eth_header_len);
    int ip_header_length = ip_protocol->ip_header_length;
    int ip_prot = ip_protocol->ip_protocol;
    meta.sourceAddress = ip_protocol->ip_source_address;
    meta.destinationAddress = ip_protocol->ip_destination_address;

    if(ip_prot == 6){
        tcp_header* tcp_protocol = (tcp_header*)(packet + sizeof(struct data_header) + this->eth_header_len + ip_header_length * 4);
        meta.sourcePort = htons(tcp_protocol->tcp_source_port);
        meta.destinationPort = htons(tcp_protocol->tcp_destination_port);
    }else if(ip_prot == 17){
        udp_header* udp_protocol = (udp_header*)(packet + sizeof(struct data_header) + this->eth_header_len + ip_header_length * 4);
        meta.sourcePort = htons(udp_protocol->udp_source_port);
        meta.destinationPort = htons(udp_protocol->udp_destination_port);
    }else{
        std::cout << "Packet aggregator warning: parsePacket with unknown protocol!" << std::endl;
        meta.sourcePort = 0;
        meta.destinationPort = 0;
    }
    return meta;
}

u_int32_t PacketAggregator::addPacketToMap(FlowMetadata meta, u_int32_t pos){
    u_int32_t last = std::numeric_limits<uint32_t>::max();
    auto it = this->aggMap.find(meta);
    if(it == this->aggMap.end()){
        Flow flow;
        flow.head = pos;
        flow.tail = pos;
        this->aggMap.insert(std::make_pair(meta,flow));
    }else{
        last = it->second.tail;
        it->second.tail = pos;
    }
    return last;
}

bool PacketAggregator::writeNextToPacketPointer(u_int32_t last, u_int32_t now){
    if(last == std::numeric_limits<uint32_t>::max()){
        return true;
    }
    return this->packetPointer->changeNextMultiThread(last,now);
}

void PacketAggregator::setThreadID(u_int32_t threadID){
    this->threadID = threadID;
}

void PacketAggregator::addID(u_int8_t id){
    this->IDSet.insert(id);
}

void PacketAggregator::ereaseID(u_int8_t id){
    this->IDSet.erase(id);
}

void PacketAggregator::run(){
    if(this->threadID == std::numeric_limits<uint32_t>::max()){
        std::cerr << "Packet aggregator error: run without threadID!" << std::endl;
        return;
    }
    std::cout << "Packet aggregator log: thread " << this->threadID << " run." << std::endl;
    this->stop = false;
    while (true){
        if(this->stop){
            break;
        }
        u_int32_t offset = this->readFromPacketPointer();
        // std::cout << "offset:" << offset << std::endl;
        if(offset == std::numeric_limits<uint32_t>::max()){
            break;
        }

        char* packet = this->readFromPacketBuffer(offset);
        if(packet == nullptr){
            break;
        }
        // std::cout << "get packet."<< std::endl;
        FlowMetadata meta = this->parsePacket(packet);
        
        u_int32_t last = this->addPacketToMap(meta,this->readPos - 1);
        // std::cout << this->readPos - 1 << " " << last << std::endl;
        if(!this->writeNextToPacketPointer(last,this->readPos - 1)){
            break;
        }
    }
    std::cout << "Packet aggregator log: thread " << this->threadID << " quit." << std::endl;
}

void PacketAggregator::asynchronousStop(){
    this->stop = true;
}