#include "directStorage.hpp"

bool DirectStorage::runUnit(){
    bool change = false;
    for(u_int32_t i=0; i<this->buffers.size(); ++i){
        if(!this->buffers[i]->checkAndWriteFile(this->checkID[i])){
            continue;
        }
        // printf("test id: %u.\n",this->testID);
        change = true;
        this->checkID[i]++;
        this->checkID[i] %= this->buffers[i]->getNum();
    }
    return change;
}

void DirectStorage::addBuffer(MemoryBuffer* buffer){
    this->buffers.push_back(buffer);
    this->checkID.push_back(0);
}

int DirectStorage::run(){
    while (true){
        bool change = this->runUnit();
        if(!change && this->stop){
            break;
        }
        if(!change){
            usleep(50000);
        }
    }
    for(u_int32_t i=0; i<this->buffers.size(); ++i){
        this->buffers[i]->directWriteFile(this->checkID[i]);
    }
    printf("Direct Storage log: thread quit.\n");
    return 0;
}

void DirectStorage::asynchronousStop(){
    this->stop = true;
}