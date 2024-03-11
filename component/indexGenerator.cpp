#include "indexGenerator.hpp"

Index IndexGenerator::readIndexFromBuffer(){
    std::string data = this->buffer->get(this->threadID);
    Index index = {
        .key = std::string(),
        .value = 0,
    };
    if(data.size() == 0){
        return index;
    }
    if(data.size() <= sizeof(u_int32_t)){
        std::cerr << "Index generator error: readIndexFromBuffer with too short index!" << std::endl;
        return index;
    }
    index.key = std::string(data.c_str(),data.size()-sizeof(index.value));
    memcpy(&index.value, &data[data.size()-sizeof(index.value)],sizeof(index.value));
    return index;
}
void IndexGenerator::putIndexToCache(const Index& index){
    // if(this->threadID){
    //     return;
    // }
    if(index.key.size()!=this->keyLen){
        std::cerr << "Index generator error: putIndexToCache with error key length!" << std::endl;
        return;
    }
    if(index.key.size()==1){
        SkipList<u_int8_t,u_int32_t>* cache = (SkipList<u_int8_t,u_int32_t>*)(this->indexCache);
        u_int8_t key;
        memcpy(&key,&index.key[0],index.key.size());
        cache->insert(key,index.value);
    }else if(index.key.size()==2){
        SkipList<u_int16_t,u_int32_t>* cache = (SkipList<u_int16_t,u_int32_t>*)(this->indexCache);
        u_int16_t key;
        memcpy(&key,&index.key[0],index.key.size());
        cache->insert(key,index.value);
    }else if(index.key.size()==4){
        SkipList<u_int32_t,u_int32_t>* cache = (SkipList<u_int32_t,u_int32_t>*)(this->indexCache);
        u_int32_t key;
        memcpy(&key,&index.key[0],index.key.size());
        cache->insert(key,index.value);
    }else if(index.key.size()==8){
        SkipList<u_int64_t,u_int32_t>* cache = (SkipList<u_int64_t,u_int32_t>*)(this->indexCache);
        u_int64_t key;
        memcpy(&key,&index.key[0],index.key.size());
        cache->insert(key,index.value);
    }else{
        std::cerr << "Index generator error: putIndexToCache with undifined key length" << index.key.size() << "!" << std::endl;
    }
}

void IndexGenerator::setThreadID(u_int32_t threadID){
    this->threadID = threadID;
}
void IndexGenerator::run(){
    if(this->threadID == std::numeric_limits<uint32_t>::max()){
        std::cerr << "Index generator error: run without threadID!" << std::endl;
        return;
    }
    std::cout << "Index generator log: thread " << this->threadID << " run." << std::endl;
    this->stop = false;

    // u_int32_t count = 0; // just for test
    while (true){
        if(this->stop){
            break;
        }
        Index data = this->readIndexFromBuffer();
        if(data.key.size()==0){
            break;
        }
        this->putIndexToCache(data);

        // count++;// just for test
    }
    // std::cout << "Index generator log: thread " << this->threadID << " count " << count << std::endl; //just for test
    std::cout << "Index generator log: thread " << this->threadID << " quit." << std::endl;
}
void IndexGenerator::asynchronousStop(){
    this->stop = true;
}