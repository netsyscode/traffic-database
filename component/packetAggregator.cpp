#include "packetAggregator.hpp"

u_int32_t PacketAggregator::readFromPacketPointer(){
    if(this->threadID == std::numeric_limits<uint32_t>::max()){
        std::cerr << "Packet aggregator error: readFromPacketPointer without thread id!" << std::endl;
        return std::numeric_limits<uint32_t>::max();
    }
    IDData id_data;
    while(true){
        id_data = this->packetPointer->getID(this->readPos,this->threadID);
        if(this->stop){
            std::cout << "Packet aggregator log: asynchronous stop." << std::endl;
            return std::numeric_limits<uint32_t>::max();
        }
        if(id_data.err == 4 && this->pause){ // pause
            std::cout << "Packet aggregator log: asynchronous pause." << std::endl;
            return std::numeric_limits<uint32_t>::max();
        }
        if(id_data.err){
            return std::numeric_limits<uint32_t>::max();
        }
        this->readPos++;
        if(this->IDSet.find(id_data.data) != this->IDSet.end()){
            break;
        }
        // if(id_data.data == 0){
        //     std::cout << this->threadID << std::endl;
        // }
    }
    return this->packetPointer->getValue(this->readPos - 1,id_data.data);
}

std::string PacketAggregator::readFromPacketBuffer(u_int32_t offset){
    return this->packetBuffer->readPcap(offset);
}

FlowMetadata PacketAggregator::parsePacket(const char* packet){
    FlowMetadata meta;
    ip_header* ip_protocol = (ip_header*)(packet + sizeof(struct data_header) + this->eth_header_len);
    int ip_header_length = ip_protocol->ip_header_length;
    int ip_prot = ip_protocol->ip_protocol;
    u_int32_t srcip = htonl(ip_protocol->ip_source_address);
    u_int32_t dstip = htonl(ip_protocol->ip_destination_address);
    meta.sourceAddress= std::string((char*)&srcip,4);
    meta.destinationAddress= std::string((char*)&dstip,4);

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

std::pair<u_int32_t,bool> PacketAggregator::addPacketToMap(FlowMetadata meta, u_int32_t pos){
    u_int32_t last = std::numeric_limits<uint32_t>::max();
    // auto it = this->oldAggMap.find(meta);
    // if(it != this->oldAggMap.end()){
    //     last = it->second.tail;
    //     it->second.tail = pos;
    //     // Flow flow;
    //     // flow.head = pos;
    //     // flow.tail = pos;
    //     // this->aggMap.insert(std::make_pair(meta,flow));
    //     // this->oldAggMap.erase(it);
    //     return std::make_pair(last,true);
    // }

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
    return std::make_pair(last,false);
}
bool PacketAggregator::writeNextToOldPacketPointer(u_int32_t last, u_int32_t now){
    if(this->oldPacketPointer == nullptr){
        std::cerr << "Packet aggregator error: writeNextToOldPacketPointer in nullptr!" << std::endl;
    }
    if(last == std::numeric_limits<u_int32_t>::max()){
        return true;
    }
    return this->oldPacketPointer->changeNextMultiThread(last, now + this->oldPacketPointer->getNodeNum());
}
bool PacketAggregator::writeNextToPacketPointer(u_int32_t last, u_int32_t now){
    if(last == std::numeric_limits<uint32_t>::max()){
        return true;
    }
    return this->packetPointer->changeNextMultiThread(last,now);
}

bool PacketAggregator::writeFlowMetaIndexToIndexBuffer(FlowMetadata meta, u_int32_t pos){
    if(this->flowMetaIndexBuffers->size()!=FLOW_META_NUM){
        std::cout << "Packet aggregator error: writeFlowMetaIndexToIndexBuffer when flow meta number not equal." <<std::endl;
        return false;
    }
    for(int i = 0; i<FLOW_META_NUM; ++i){
        char* input;
        switch (i){
        case 0:
            input = new char[meta.sourceAddress.size()+sizeof(pos)];
            memcpy(input,meta.sourceAddress.c_str(),meta.sourceAddress.size());
            memcpy(input+meta.sourceAddress.size(),&pos,sizeof(pos));
            (*this->flowMetaIndexBuffers)[i]->put(input,this->threadID,meta.sourceAddress.size()+sizeof(pos));
            delete[] input;
            break;
        case 1:
            input = new char[meta.destinationAddress.size()+sizeof(pos)];
            memcpy(input,meta.destinationAddress.c_str(),meta.destinationAddress.size());
            memcpy(input+meta.destinationAddress.size(),&pos,sizeof(pos));
            (*this->flowMetaIndexBuffers)[i]->put(input,this->threadID,meta.destinationAddress.size()+sizeof(pos));
            delete[] input;
            break;
        case 2:
            input = new char[sizeof(meta.sourcePort)+sizeof(pos)];
            memcpy(input,&meta.sourcePort,sizeof(meta.sourcePort));
            memcpy(input+sizeof(meta.sourcePort),&pos,sizeof(pos));
            (*this->flowMetaIndexBuffers)[i]->put(input,this->threadID,sizeof(meta.sourcePort)+sizeof(pos));
            delete[] input;
            break;
        case 3:
            input = new char[sizeof(meta.destinationPort)+sizeof(pos)];
            memcpy(input,&meta.destinationPort,sizeof(meta.destinationPort));
            memcpy(input+sizeof(meta.destinationPort),&pos,sizeof(pos));
            (*this->flowMetaIndexBuffers)[i]->put(input,this->threadID,sizeof(meta.destinationPort)+sizeof(pos));
            delete[] input;
            break;
        default:
            break;
        }
    }
    return true;
}

void PacketAggregator::truncate(){
    if(this->newPacketBuffer == nullptr || this->newPacketPointer == nullptr || this->flowMetaIndexBuffers == nullptr){
        std::cerr <<"Packet aggregator error: trancate without new memory!" << std::endl;
        this->pause = false;
        return;
    }

    if(this->oldPacketPointer != nullptr){
        this->oldPacketPointer->ereaseReadThread(this->selfPointer);
    }

    this->oldPacketPointer = this->packetPointer;
    this->oldAggMap = this->aggMap;

    for(auto ib:(*(this->flowMetaIndexBuffers))){
        ib->ereaseWriteThread(this->selfPointer);
    }

    this->packetBuffer = this->newPacketBuffer;
    this->packetPointer = this->newPacketPointer;
    this->flowMetaIndexBuffers = this->newFlowMetaIndexBuffers;
    this->aggMap = std::unordered_map<FlowMetadata, Flow, FlowMetadata::hash>();

    this->packetPointer->addReadThread(this->selfPointer);
    for(auto ib:(*(this->flowMetaIndexBuffers))){
        ib->addWriteThread(this->selfPointer);
    }

    this->newPacketBuffer = nullptr;
    this->newPacketPointer = nullptr;
    this->newFlowMetaIndexBuffers = nullptr;
    
    this->readPos = 0;

    this->pause = false;
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
    this->pause = false;
    
    while (true){
        if(this->stop){
            break;
        }
        u_int32_t offset = this->readFromPacketPointer();
        // std::cout << "offset:" << offset << std::endl;
        if(offset == std::numeric_limits<uint32_t>::max()){
            if(this->stop){
                break;
            }
            if(this->pause){
                this->truncate();
                continue;
            }
        }

        std::string packet = this->readFromPacketBuffer(offset);
        if(packet.size() == 0){
            break;
        }
        // std::cout << "get packet."<< std::endl;
        FlowMetadata meta = this->parsePacket(packet.c_str());

        auto last_ret = this->addPacketToMap(meta,this->readPos - 1);
        if(last_ret.second){
            if(!this->writeNextToOldPacketPointer(last_ret.first,this->readPos - 1)){
                break;
            }
        }else{
            if(!this->writeNextToPacketPointer(last_ret.first,this->readPos - 1)){
                break;
            }
        }

        if(last_ret.first == std::numeric_limits<uint32_t>::max()){ //new flow
            if(!this->writeFlowMetaIndexToIndexBuffer(meta,this->readPos - 1)){
                break;
            }
        }
    }
    std::cout << "Packet aggregator log: thread " << this->threadID << " quit." << std::endl;
}

void PacketAggregator::asynchronousStop(){
    this->stop = true;
}
void PacketAggregator::asynchronousPause(ShareBuffer* newPacketBuffer, ArrayList<u_int32_t>* newPacketPointer, std::vector<RingBuffer*>* newFlowMetaIndexBuffers, ThreadPointer* pointer){
    if(this->threadID != pointer->id){
        std::cerr << "Packet aggregator error: asynchronousPause with wrong thread pointer!" << std::endl;
    }
    this->selfPointer = pointer;
    this->newPacketBuffer = newPacketBuffer;
    this->newPacketPointer = newPacketPointer;
    this->newFlowMetaIndexBuffers = newFlowMetaIndexBuffers;
    this->pause = true;
}
bool PacketAggregator::getPause()const{
    return this->pause;
}