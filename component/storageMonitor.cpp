#include "storageMonitor.hpp"

bool StorageMonitor::readTruncatePipe(){
    std::string data = this->truncatePipe->get(this->threadID);
    // std::cout << "Storage log: readTruncatePipe get one." << std::endl;
    TruncateGroup group;
    if(data.size() == 0){
        return false;
    }
    if(data.size() != sizeof(TruncateGroup)){
        std::cerr << "Storage error: readTruncatePipe with wrong TruncateGroup size!" << std::endl;
        return false;
    }
    memcpy(&group, &(data[0]),data.size());
    this->truncatedMemory.push_back(group);
    return true;
}
void StorageMonitor::clearTruncateGroup(TruncateGroup& tg){
    if(tg.flowMetaIndexGeneratorThreads != nullptr){
        for(int i=0;i<tg.flowMetaIndexGeneratorThreads->size();++i){
            for(int j=0;j<(*(tg.flowMetaIndexGeneratorThreads))[i].size();++j){
                // assert thread stop
                // (*(tg.flowMetaIndexGenerators))[i][j]->asynchronousStop();
                // (*(tg.flowMetaIndexBuffers))[i]->asynchronousStop((*(tg.flowMetaIndexGeneratorPointers))[i][j]->id);
                // (*(tg.flowMetaIndexGeneratorThreads))[i][j]->join();
                if((*(tg.flowMetaIndexGeneratorThreads))[i][j]==nullptr){
                    continue;
                }
                delete (*(tg.flowMetaIndexGeneratorThreads))[i][j];
                (*(tg.flowMetaIndexGeneratorThreads))[i][j] = nullptr;
            }
            (*(tg.flowMetaIndexGeneratorThreads))[i].clear();
        }
        tg.flowMetaIndexGeneratorThreads->clear();
        delete tg.flowMetaIndexGeneratorThreads;
        tg.flowMetaIndexGeneratorThreads = nullptr;
    }

    if(tg.flowMetaIndexGeneratorPointers != nullptr){
        for(int i=0;i<tg.flowMetaIndexGeneratorPointers->size();++i){
            for(auto p:(*(tg.flowMetaIndexGeneratorPointers))[i]){
                if(p==nullptr){
                    continue;
                }
                // (*(tg.flowMetaIndexBuffers))[i]->ereaseReadThread(p);
                delete p;
            }
            (*(tg.flowMetaIndexGeneratorPointers))[i].clear();
        }
        tg.flowMetaIndexGeneratorPointers->clear();
        delete tg.flowMetaIndexGeneratorPointers;
        tg.flowMetaIndexGeneratorPointers = nullptr;
    }

    if(tg.flowMetaIndexGenerators != nullptr){
        for(int i=0;i<tg.flowMetaIndexGenerators->size();++i){
            for(auto p:(*(tg.flowMetaIndexGenerators))[i]){
                if(p==nullptr){
                    continue;
                }
                delete p;
            }
            tg.flowMetaIndexGenerators[i].clear();
        }
        tg.flowMetaIndexGenerators->clear();
        delete tg.flowMetaIndexGenerators;
        tg.flowMetaIndexGenerators = nullptr;
    }

    // if(tg.oldPacketBuffer!=nullptr){
    //     delete tg.oldPacketBuffer;
    //     tg.oldPacketBuffer = nullptr;
    // }

    if(tg.oldPacketPointer!=nullptr){
        delete tg.oldPacketPointer;
        tg.oldPacketPointer = nullptr;
    }

    if(tg.flowMetaIndexCaches!=nullptr){
        for(auto ic:*(tg.flowMetaIndexCaches)){
            if(ic==nullptr){
                continue;
            }
            delete ic;
        }
        tg.flowMetaIndexCaches->clear();
        delete tg.flowMetaIndexCaches;
        tg.flowMetaIndexCaches = nullptr;
    }

    if(tg.flowMetaIndexBuffers!=nullptr){
        for(auto rb:*(tg.flowMetaIndexBuffers)){
            if(rb==nullptr){
                continue;
            }
            delete rb;
        }
        tg.flowMetaIndexBuffers->clear();
        delete tg.flowMetaIndexBuffers;
        tg.flowMetaIndexBuffers = nullptr;
    }

}
void StorageMonitor::store(TruncateGroup& tg){
    StorageMeta meta = {
        .pointer_offset = this->pointer_offset,
        .data_offset = this->data_offset,
    };
    for(int i=0;i<FLOW_META_INDEX_NUM;++i){
        meta.index_offset[i] = this->index_offset[i];
        std::ofstream indexFile(this->index_name[i], std::ios::app);
        if (!indexFile.is_open()) {
            std::cerr << "Storage monitor error: store to non-exist file name " << this->index_name[i] << "!" << std::endl;
            return;
        }
        indexFile.seekp(this->index_offset[i],std::ios::beg);
        std::string index = (*(tg.flowMetaIndexCaches))[i]->outputToChar();
        indexFile.write(index.c_str(),index.size());
        // printf("Storage monitor log: skip list size of %u\n",index.size());
        // std::cout << "index len of " << i << " : " << index.size() << std::endl;
        this->index_offset[i] += index.size();
        indexFile.close();
        meta.index_end[i] = this->index_offset[i];
    }

    std::ofstream pointerFile(this->pointer_name, std::ios::app);
    if (!pointerFile.is_open()) {
        std::cerr << "Storage monitor error: store to non-exist file name " << this->pointer_name << "!" << std::endl;
        return;
    }
    pointerFile.seekp(this->pointer_offset,std::ios::beg);
    std::string pointer = (tg.oldPacketPointer->outputToChar());
    // printf("Storage monitor log: pointer size of %u\n",pointer.size());
    pointerFile.write(pointer.c_str(),pointer.size());
    this->pointer_offset += pointer.size();
    pointerFile.close();
    meta.pointer_end = this->pointer_offset;

    // std::ofstream dataFile(this->data_name, std::ios::app);
    // if (!dataFile.is_open()) {
    //     std::cerr << "Storage monitor error: store to non-exist file name " << this->data_name << "!" << std::endl;
    //     return;
    // }
    // dataFile.seekp(this->data_offset,std::ios::beg);
    // std::string data = (tg.oldPacketBuffer->outputToChar());
    // dataFile.write(data.c_str() + this->pcap_header_len, data.size() - this->pcap_header_len);
    // this->data_offset += data.size() - this->pcap_header_len;
    // std::cout << "Storage monitor log: store " << data.size() - this->pcap_header_len << " bytes." << std::endl;
    // dataFile.close();

    // get time, assume data and pointers are time ordered
    ArrayListNode<u_int32_t>* pointer_array = (ArrayListNode<u_int32_t>*)(&pointer[0]);
    u_int32_t first_packet_offset = pointer_array[0].value;
    u_int32_t last_packet_offset = pointer_array[pointer.size()/sizeof(ArrayListNode<u_int32_t>) - 1].value;

    // char* data = tg.packetBuffer->getPointer(first_packet_offset);
    // if(data == nullptr){
    //     printf("Storage monitor error: store with null data.\n");
    // }
    data_header* first_packet_header = (data_header*)(tg.packetBuffer->getPointer(first_packet_offset));
    data_header* last_packet_header = (data_header*)(tg.packetBuffer->getPointer(last_packet_offset));

    meta.data_end = last_packet_offset + last_packet_header->caplen + sizeof(data_header);

    // printf("Storage monitor log: start time:%u.%u, end time:%u.%u.\n");
    meta.time_start = ((u_int64_t)(first_packet_header->ts_h) << sizeof(u_int32_t)*8) + (u_int64_t)(first_packet_header->ts_l);
    meta.time_end = ((u_int64_t)(last_packet_header->ts_h) << sizeof(u_int32_t)*8) + (u_int64_t)(last_packet_header->ts_l);

    // printf("Storage monitor log: start time:%llu, end time:%llu.\n",meta.time_start,meta.time_end);
    this->storageMetas->push_back(meta);
    // std::cout << "Storage monitor log: store finish." << std::endl;
}
void StorageMonitor::monitor(){
    if(this->truncatedMemory.size()==0){
        std::cerr << "Storage monitor error: monitor with empty truncatedMemory !" << std::endl;
    }
    auto tg = this->truncatedMemory[0];
    for(int i=0;i<tg.flowMetaIndexGeneratorThreads->size();++i){
        for(int j=0;j<(*(tg.flowMetaIndexGeneratorThreads))[i].size();++j){
            // std::cout << "Storage monitor log: index generator thread " << (*(tg.flowMetaIndexGeneratorPointers))[i][j]->id << " wait." << std::endl;
            (*(tg.flowMetaIndexGeneratorThreads))[i][j]->join();
            // std::cout << "Storage monitor log: index generator thread " << (*(tg.flowMetaIndexGeneratorPointers))[i][j]->id << " stop." << std::endl;
            (*(tg.flowMetaIndexBuffers))[i]->ereaseReadThread((*(tg.flowMetaIndexGeneratorPointers))[i][j]);
        }
    }
}

void StorageMonitor::memoryClear(){
    for(auto tg:this->truncatedMemory){
        this->clearTruncateGroup(tg);
    }
    this->truncatedMemory.clear();
}
void StorageMonitor::runUnit(){
    this->monitor();
    
    auto start = std::chrono::high_resolution_clock::now();

    auto tg = this->truncatedMemory[0];
    this->truncatedMemory.erase(this->truncatedMemory.begin());
    
    this->store(tg);
    this->clearTruncateGroup(tg);

    auto end = std::chrono::high_resolution_clock::now();
    this->duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

void StorageMonitor::init(InitData init_data){
    for(int i=0;i<FLOW_META_INDEX_NUM;++i){
        this->index_offset[i] = 0;
    }
    this->pointer_offset = 0;
    this->data_offset = 0;

    this->pcap_header = init_data.pcap_header;
    this->pcap_header_len = this->pcap_header.size();

    for(int i=0;i<FLOW_META_INDEX_NUM;++i){//clear files
        std::ofstream indexFile(this->index_name[i], std::ios::binary);
        if (!indexFile.is_open()) {
            std::cerr << "Storage monitor error: store to non-exist file name " << this->index_name[i] << "!" << std::endl;
            return;
        }
        indexFile.clear();
        indexFile.close();
    }
    std::ofstream pointerFile(this->pointer_name, std::ios::binary);
    if (!pointerFile.is_open()) {//clear files
        std::cerr << "Storage monitor error: store to non-exist file name " << this->pointer_name << "!" << std::endl;
        return;
    }
    pointerFile.clear();
    pointerFile.close();
    unlink(this->data_name.c_str());
    if (link(init_data.filename.c_str(), this->data_name.c_str()) == -1) {
        printf("Storage monitor error: symlink error!\n");
        return;
    }
    // std::ofstream dataFile(this->data_name, std::ios::binary);
    // if (!dataFile.is_open()) {
    //     std::cerr << "Storage monitor error: store to non-exist file name " << this->data_name << "!" << std::endl;
    //     return;
    // }
    // dataFile.clear();
    // dataFile.seekp(this->data_offset,std::ios::beg);
    // dataFile.write(this->pcap_header.c_str(), this->pcap_header_len);
    this->data_offset += this->pcap_header_len;
    // dataFile.close();
    std::cout << "Storage monitor log: init." << std::endl;
}
void StorageMonitor::run(){

    // pthread_t threadId = pthread_self();

    // // 创建 CPU 集合，并将指定核心加入集合中
    // cpu_set_t cpuset;
    // CPU_ZERO(&cpuset);
    // CPU_SET(4, &cpuset);

    // // // 设置线程的 CPU 亲和性
    // int result = pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpuset);

    if(this->pcap_header_len == 0){
        std::cerr << "Storage monitor error: run without init!" << std::endl;
        return;
    }
    std::cout << "Storage monitor log: run." << std::endl;
    while(true){
        if(!this->readTruncatePipe()){
            break;
        }
        this->runUnit();
    }
    while(this->truncatedMemory.size()){
        this->runUnit();
    }
    printf("Storage monitor log: quit, during %lld store time.\n",this->duration_time);
}