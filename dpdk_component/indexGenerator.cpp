#include "indexGenerator.hpp"

Index* IndexGenerator::readIndexFromBuffer(){
    void* data = this->buffer->get();
    Index* index = (Index*)data;
    return index;
}
void IndexGenerator::putIndexToCache(Index* index){
    ZOrderIPv4 zorder = ZOrderIPv4(index);
    while(true){
        if(this->indexBuffer->insert(std::string((char*)&zorder,sizeof(zorder)),index->value,this->cacheID,index->ts)){
            break;
        }
        this->cacheID++;
        this->cacheID %= this->indexCacheCount;
    } 
}

void IndexGenerator::setThreadID(u_int32_t threadID){
    this->threadID = threadID;
}
void IndexGenerator::run(){
    printf("Index generator log: thread run.\n");

    this->stop = false;

    u_int64_t duration_time = 0;

    while (true){
        if(this->stop){
            break;
        }
        Index* data = this->readIndexFromBuffer();
        // printf("Index generator log: get one.\n");
        if(data == nullptr){
            break;
        }
        auto start = std::chrono::high_resolution_clock::now();

        this->putIndexToCache(data);
        
        delete data;

        // count++;// just for test
        auto end = std::chrono::high_resolution_clock::now();
        duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    // std::cout << "Index generator log: thread " << this->threadID << " count " << count << std::endl; //just for test
    printf("Index generator log: thread %u quit, during %llu us.\n",this->threadID,duration_time);
}
void IndexGenerator::asynchronousStop(){
    this->stop = true;
}