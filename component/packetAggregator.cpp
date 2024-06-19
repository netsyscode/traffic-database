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
            printf("Packet aggregator log: %u asynchronous stop.\n",this->threadID);
            //std::cout << "Packet aggregator log: " << this->threadID << " asynchronous stop." << std::endl;
            return std::numeric_limits<uint32_t>::max();
        }
        if(id_data.err == 4 && this->pause){ // pause
            printf("Packet aggregator log: %u asynchronous pause.\n",this->threadID);
            //std::cout << "Packet aggregator log: " << this->threadID << " asynchronous pause." << std::endl;
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

char* PacketAggregator::readFromPacketBuffer(u_int64_t offset){
    u_int32_t _offset = (u_int32_t)(offset & 0xffffffff);
    offset >>= sizeof(u_int32_t)*8;
    u_int32_t file_id = (u_int32_t)(offset & 0xffffffff);
    for(auto f:*(this->packetBuffers)){
        if(f->getFileID()==file_id){
            return f->getPointer(_offset);
        }
    }
    return nullptr;
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
        std::cout << "Packet aggregator warning: parsePacket with unknown protocol:" << ip_prot << "!" << std::endl;
        meta.sourcePort = 0;
        meta.destinationPort = 0;
    }
    return meta;
}

std::pair<u_int32_t,bool> PacketAggregator::addPacketToMap(FlowMetadata meta, u_int32_t pos){
    u_int32_t last = std::numeric_limits<uint32_t>::max();
    // todo: delay truncate
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
        std::cerr << "Packet aggregator error: writeFlowMetaIndexToIndexBuffer when flow meta number not equal." <<std::endl;
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
    if(this->newPacketPointer == nullptr || this->flowMetaIndexBuffers == nullptr || this->selfPointer == nullptr){
        std::cerr <<"Packet aggregator error: trancate without new memory!" << std::endl;
        this->pause = false;
        return;
    }

    // printf("Packet aggregator log: thread %u begin truncate.\n",this->threadID);
    // std::cout << "Packet aggregator log: thread " << this->threadID << " begin truncate." << std::endl;

    // todo: delay truncate
    // if(this->oldPacketPointer != nullptr){
    //     this->oldPacketPointer->ereaseReadThread(this->selfPointer);
    // }
    auto oldpp = this->packetPointer;
    auto oldib = this->flowMetaIndexBuffers;

    this->oldPacketPointer = this->packetPointer;
    this->oldAggMap = this->aggMap;

    // this->packetBuffer = this->newPacketBuffer;
    this->packetPointer = this->newPacketPointer;
    this->flowMetaIndexBuffers = this->newFlowMetaIndexBuffers;
    // this->aggMap = std::unordered_map<FlowMetadata, Flow, FlowMetadata::hash>();
    this->aggMap.clear();

    this->packetPointer->addReadThread(this->selfPointer);
    for(auto ib:(*(this->flowMetaIndexBuffers))){
        ib->addWriteThread(this->selfPointer);
    }

    // this->newPacketBuffer = nullptr;
    this->newPacketPointer = nullptr;
    this->newFlowMetaIndexBuffers = nullptr;
    
    this->readPos = 0;

    if(oldpp != nullptr){
        oldpp->ereaseReadThread(this->selfPointer);
    }
    if(oldib != nullptr){
        for(auto ib:(*(oldib))){
            if(ib == nullptr){
                continue;
            }
            ib->ereaseWriteThread(this->selfPointer);
        }
    }
    this->pause = false;
    // this->monitor_cv->notify_all();
    // printf("Packet aggregator log: thread %u end truncate.\n",this->threadID);
    // std::cout << "Packet aggregator log: thread " << this->threadID << " end truncate." << std::endl;
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

    // pthread_t threadId = pthread_self();

    // 创建 CPU 集合，并将指定核心加入集合中
    // cpu_set_t cpuset;
    // CPU_ZERO(&cpuset);
    // CPU_SET(48+this->threadID, &cpuset);

    // // // 设置线程的 CPU 亲和性
    // int result = pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpuset);

    printf("Packet aggregator log: thread %u run.\n",this->threadID);
    //std::cout << "Packet aggregator log: thread " << this->threadID << " run." << std::endl;
    this->stop = false;
    this->pause = false;
    
    u_int64_t truncate_time = 0;
    // u_int32_t index_count = 0;
    while (true){
        if(this->stop){
            break;
        }
        u_int32_t offset = this->readFromPacketPointer();

        auto start = std::chrono::high_resolution_clock::now();
        // std::cout << "offset:" << offset << std::endl;
        if(offset == std::numeric_limits<uint32_t>::max()){
            if(this->stop){
                printf("Packet aggregator log: thread %u should stop.\n",this->threadID);
                //std::cout << "Packet aggregator log: thread " << this->threadID << " should stop." << std::endl;
                break;
            }
            if(this->pause){
                auto start_truncate = std::chrono::high_resolution_clock::now();
                this->truncate();
                // printf("Packet aggregator log: truncate with index count %u.\n",index_count);
                // index_count = 0;
                auto end_truncate = std::chrono::high_resolution_clock::now();
                truncate_time += std::chrono::duration_cast<std::chrono::microseconds>(end_truncate - start_truncate).count();
                continue;
            }
            std::cerr << "Packet aggregator log: thread " << this->threadID << " get wrong pos!" << std::endl;
            break;
        }

        char* packet = this->readFromPacketBuffer(offset);
        if(packet == nullptr){
            break;
        }
        u_int32_t len = ((data_header*)(packet))->caplen + sizeof(data_header);
        
        // std::cout << "get packet."<< std::endl;
        FlowMetadata meta = this->parsePacket(packet);
        if(meta.sourcePort == 0){
            printf("offset:%u,len:%u,node num:%u\n",offset,len,this->packetPointer->getNodeNum());
        }

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
            // index_count++;
        }

        auto end = std::chrono::high_resolution_clock::now();
        this->duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    printf("Packet aggregator log: thread %u quit, during %llu us, with truncate time %llu us\n.",this->threadID, this->duration_time, truncate_time);
    //std::cout << "Packet aggregator log: thread " << this->threadID << " quit." << std::endl;
}

void PacketAggregator::asynchronousStop(){
    this->stop = true;
}
void PacketAggregator::asynchronousPause(ArrayList<u_int32_t>* newPacketPointer, std::vector<RingBuffer*>* newFlowMetaIndexBuffers, ThreadPointer* pointer){
    if(this->threadID != pointer->id){
        std::cerr << "Packet aggregator error: asynchronousPause with wrong thread pointer!" << std::endl;
    }
    this->selfPointer = pointer;
    this->newPacketPointer = newPacketPointer;
    this->newFlowMetaIndexBuffers = newFlowMetaIndexBuffers;
    this->pause = true;
}
bool PacketAggregator::getPause()const{
    return this->pause;
}