#include "storage.hpp"

void* Storage::readRing(){  
    return this->ring->get();
}

void Storage::store(TruncateUnit* unit){
    int index_type = (int)(unit->index_type);
    std::string data = unit->index_map->outputToChar();
    if(data.size()==0){
        return;
    }
    StorageMeta meta = {
        .index_offset = this->index_offset[index_type],
        .index_end = this->index_offset[index_type] + data.size(),
        .time_start = unit->ts_first,
        .time_end = unit->ts_last,
    };
    std::ofstream indexFile(index_name[index_type], std::ios::app);
    if (!indexFile.is_open()) {
        std::cerr << "Storage monitor error: store to non-exist file name " << index_name[index_type] << "!" << std::endl;
        return;
    }
    indexFile.seekp(this->index_offset[index_type],std::ios::beg);
    indexFile.write(data.c_str(),data.size());
    this->index_offset[index_type] += data.size();
    indexFile.close();
    this->storageMetas[index_type].push_back(meta);
}

void Storage::run(){
    std::cout << "Storage monitor log: run." << std::endl;
    this->stop = false;
    while(!this->stop){
        TruncateUnit* unit = (TruncateUnit*)(this->readRing());
        if(unit == nullptr){
            break;
        }
        this->store(unit);
        delete unit->index_map;
        delete unit;
    }
    printf("Storage monitor log: quit.\n");
}

void Storage::asynchronousStop(){
    this->stop = true;
}