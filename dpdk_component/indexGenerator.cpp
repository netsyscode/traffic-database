#include "indexGenerator.hpp"

Index* IndexGenerator::readIndexFromBuffer(){
    void* data = this->buffer->get();
    Index* index = (Index*)data;
    // if(data.size() == 0){
    //     return index;
    // }
    // if(data.size() <= sizeof(u_int32_t)){
    //     std::cerr << "Index generator error: readIndexFromBuffer with too short index!" << std::endl;
    //     return index;
    // }
    // index.key = std::string(data.c_str(),data.size()-sizeof(index.value));
    // memcpy(&index.value, &data[data.size()-sizeof(index.value)],sizeof(index.value));
    return index;
}
void IndexGenerator::putIndexToCache(const Index* index){
    if(index->key.size()!=this->keyLen){
        std::cerr << "Index generator error: putIndexToCache with error key length!" << std::endl;
        return;
    }
    this->indexCache->insert(index->key,index->value);
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
    // if(this->threadID == std::numeric_limits<uint32_t>::max()){
    //     std::cerr << "Index generator error: run without threadID!" << std::endl;
    //     return;
    // }

    // pthread_t threadId = pthread_self();

    // // 创建 CPU 集合，并将指定核心加入集合中
    // cpu_set_t cpuset;
    // CPU_ZERO(&cpuset);
    // CPU_SET(32+this->threadID, &cpuset);

    // // // 设置线程的 CPU 亲和性
    // int result = pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpuset);

    printf("Index generator log: thread run.\n");
    //std::cout << "Index generator log: thread " << this->threadID << " run." << std::endl;
    this->stop = false;
    // this->pause = false;

    // u_int32_t count = 0; // just for test
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
        if(data->key.size()==0){
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
        if(this->truncator->getPause()){
            auto tmp = this->indexCache;
            this->indexCache = this->truncator->newIndexMap;
            tmp->ereaseWriteThread();
            this->indexCache->addWriteThread();
            while (true){
                if(!this->truncator->getPause()){
                    break;
                }
            }
        }

        // printf("Index generator log: updateTS.\n");
        this->truncator->updateTS(data->ts);
        // printf("Index generator log: updateTS done.\n");

        this->putIndexToCache(data);
        
        delete data;

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