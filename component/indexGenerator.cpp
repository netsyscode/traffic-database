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

// void IndexGenerator::truncate(){
//     if(this->newBuffer == nullptr || this->newIndexCache == nullptr){
//         std::cerr <<"Indext generator error: trancate without new memory!" << std::endl;
//         this->pause = false;
//         return;
//     }

//     this->buffer = this->newBuffer;
//     this->indexCache = this->newIndexCache;
//     this->newBuffer = nullptr;
//     this->newIndexCache = nullptr;
//     this->pause = false;
// }

void IndexGenerator::setThreadID(u_int32_t threadID){
    this->threadID = threadID;
}
void IndexGenerator::run(){
    if(this->threadID == std::numeric_limits<uint32_t>::max()){
        std::cerr << "Index generator error: run without threadID!" << std::endl;
        return;
    }
    printf("Index generator log: thread %u  run.\n",this->threadID);
    //std::cout << "Index generator log: thread " << this->threadID << " run." << std::endl;
    this->stop = false;
    // this->pause = false;

    // u_int32_t count = 0; // just for test
    while (true){
        if(this->stop){
            break;
        }
        Index data = this->readIndexFromBuffer();
        auto start = std::chrono::high_resolution_clock::now();
        if(data.key.size()==0){
            this->stop = true;
            break;
        }
        // if(data.key.size()==0){
        //     if(this->stop){
        //         break;
        //     }
        //     if(this->pause){
        //         this->truncate();
        //     }
        // }
        this->putIndexToCache(data);

        // count++;// just for test
        auto end = std::chrono::high_resolution_clock::now();
        this->duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    // std::cout << "Index generator log: thread " << this->threadID << " count " << count << std::endl; //just for test
    printf("Index generator log: thread %u quit, during %llu us.\n",this->threadID,this->duration_time);
    //std::cout << "Index generator log: thread " << this->threadID << " quit." << std::endl;
}
void IndexGenerator::asynchronousStop(){
    this->stop = true;
}
// void IndexGenerator::asynchronousPause(RingBuffer* newBuffer, SkipList* newIndexCache){
//     this->newBuffer = newBuffer;
//     this->newIndexCache = newIndexCache;
//     this->pause = true;
// }