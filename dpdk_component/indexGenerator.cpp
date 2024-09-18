#include "indexGenerator.hpp"

Index* IndexGenerator::readIndexFromBuffer(){
    void* data = this->buffer->get();
    Index* index = (Index*)data;
    return index;
}
void IndexGenerator::putIndexToCache(Index* index){
    // std::vector<std::string> keys = std::vector<std::string>();

    // auto start = std::chrono::high_resolution_clock::now();
    // keys.push_back(index->meta.sourceAddress);
    // keys.push_back(index->meta.destinationAddress);
    // keys.push_back(std::string((char*)&index->meta.sourcePort,sizeof(index->meta.sourcePort)));
    // keys.push_back(std::string((char*)&index->meta.destinationPort,sizeof(index->meta.destinationPort)));

    // auto end = std::chrono::high_resolution_clock::now();
    

    while(true){
        if(index->id != 0){
            break;
        }
        std::string key;
        if(index->len == 2){
            u_int32_t keyInt = (u_int16_t)(index->key);
            key = std::string((char*)&(keyInt),index->len);
        }else if(index->len == 4){
            u_int32_t keyInt = (u_int32_t)(index->key);
            key = std::string((char*)&(keyInt),index->len);
        }else {
            key = std::string();
        }
        auto start = std::chrono::high_resolution_clock::now();

        if((*(this->indexBuffers))[index->id]->insert(key,index->value,this->cacheID,index->ts,this->threadID)){
            auto end = std::chrono::high_resolution_clock::now();
            this->duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            break;
        }

        this->cacheID++;
        this->cacheID %= this->indexCacheCount;
    } 

    // this->duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // ZOrderIPv4 zorder = ZOrderIPv4(index);
    // while(true){
    //     if(this->indexBuffer->insert(std::string((char*)&zorder,sizeof(zorder)),index->value,this->cacheID,index->ts)){
    //         break;
    //     }
    //     this->cacheID++;
    //     this->cacheID %= this->indexCacheCount;
    // } 
}

void IndexGenerator::setThreadID(u_int32_t threadID){
    this->threadID = threadID;
}
void IndexGenerator::run(){
    printf("Index generator log: thread run.\n");

    this->stop = false;

    // u_int64_t duration_time = 0;

    u_int64_t count = 0;

    bool has_start = false;
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();

    while (true){
        // if(this->stop){
        //     break;
        // }
        Index* data = this->readIndexFromBuffer();
        // printf("Index generator log: get one.\n");
        if(data == nullptr){
            break;
        }
        if(!has_start){
            has_start = true;
            start = std::chrono::high_resolution_clock::now();
        }

        this->putIndexToCache(data);
        
        delete data;

        count++;
        end = std::chrono::high_resolution_clock::now();
        // count++;// just for test
    }
    
    // duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    // std::cout << "Index generator log: thread " << this->threadID << " count " << count << std::endl; //just for test
    printf("Index generator log: thread quit, process %llu indexes, during %llu us.\n",count,this->duration_time);
}
void IndexGenerator::asynchronousStop(){
    this->stop = true;
}