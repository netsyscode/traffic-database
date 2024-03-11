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
    if(index.key.size()!=this->keyLen){
        std::cerr << "Index generator error: putIndexToCache with error key length!" << std::endl;
        return;
    }
    this->indexCache->insert(index.key,index.value);
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