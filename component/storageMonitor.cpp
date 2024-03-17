#include "storageMonitor.hpp"

bool StorageMonitor::readTruncatePipe(){
    std::string data = this->truncatePipe->get(this->threadID);
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
                // (*(tg.flowMetaIndexBuffers))[i]->ereaseReadThread(p);
                delete p;
            }
            (*(tg.flowMetaIndexGeneratorPointers))[i].clear();
        }
        tg.flowMetaIndexGeneratorPointers->clear();
        delete tg.flowMetaIndexGeneratorPointers;
        tg.flowMetaIndexBuffers = nullptr;
    }

    if(tg.flowMetaIndexGenerators != nullptr){
        for(int i=0;i<tg.flowMetaIndexGenerators->size();++i){
            for(auto p:(*(tg.flowMetaIndexGenerators))[i]){
                delete p;
            }
            tg.flowMetaIndexGenerators[i].clear();
        }
        tg.flowMetaIndexGenerators->clear();
    }

    if(tg.oldPacketBuffer!=nullptr){
        delete tg.oldPacketBuffer;
    }

    if(tg.oldPacketPointer!=nullptr){
        delete tg.oldPacketPointer;
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
            delete rb;
        }
        tg.flowMetaIndexBuffers->clear();
        delete tg.flowMetaIndexBuffers;
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
    pointerFile.write(pointer.c_str(),pointer.size());
    this->pointer_offset += pointer.size();
    pointerFile.close();
    meta.pointer_end = this->pointer_offset;

    std::ofstream dataFile(this->data_name, std::ios::app);
    if (!dataFile.is_open()) {
        std::cerr << "Storage monitor error: store to non-exist file name " << this->data_name << "!" << std::endl;
        return;
    }
    dataFile.seekp(this->data_offset,std::ios::beg);
    std::string data = (tg.oldPacketBuffer->outputToChar());
    dataFile.write(data.c_str() + this->pcap_header_len, data.size() - this->pcap_header_len);
    this->data_offset += data.size() - this->pcap_header_len;
    // std::cout << "Storage monitor log: store " << data.size() - this->pcap_header_len << " bytes." << std::endl;
    dataFile.close();
    meta.data_end = this->data_offset;

    this->storageMetas->push_back(meta);
    std::cout << "Storage monitor log: store finish." << std::endl;
}
void StorageMonitor::monitor(){
    if(this->truncatedMemory.size()==0){
        std::cerr << "Storage monitor error: monitor with empty truncatedMemory !" << std::endl;
    }
    auto tg = this->truncatedMemory[0];
    for(int i=0;i<tg.flowMetaIndexGeneratorThreads->size();++i){
        for(int j=0;j<(*(tg.flowMetaIndexGeneratorThreads))[i].size();++j){
            (*(tg.flowMetaIndexGeneratorThreads))[i][j]->join();
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
    auto tg = this->truncatedMemory[0];
    this->truncatedMemory.erase(this->truncatedMemory.begin());
    this->store(tg);
    this->clearTruncateGroup(tg);
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
    std::ofstream dataFile(this->data_name, std::ios::binary);
    if (!dataFile.is_open()) {
        std::cerr << "Storage monitor error: store to non-exist file name " << this->data_name << "!" << std::endl;
        return;
    }
    dataFile.clear();
    dataFile.seekp(this->data_offset,std::ios::beg);
    dataFile.write(this->pcap_header.c_str(), this->pcap_header_len);
    this->data_offset += this->pcap_header_len;
    dataFile.close();
    std::cout << "Storage monitor log: init." << std::endl;
}
void StorageMonitor::run(){
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
    std::cout << "Storage monitor log: quit." << std::endl;
}