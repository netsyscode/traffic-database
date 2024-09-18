#include "indexStorage.hpp"

void IndexStorage::processSkipList(SkipList* list, u_int64_t ts, u_int32_t id){
    std::string data = list->outputToChar();

    u_int32_t ts_l = ts & 0xffffffff;
    u_int32_t ts_h = ts >> sizeof(ts_h)*8;

    std::string fileName = "./data/index/" + std::to_string(ts_h) + "." + std::to_string(ts_l) + "." + std::to_string(id) + "_idx";
    int fileFD = open(fileName.c_str(), O_WRONLY | O_CREAT, 0644);
    if (fileFD == -1) {
        printf("indexStorage error: failed to open file of zorder_list!\n");
    }
    ssize_t bytes_written = write(fileFD, data.c_str(), data.size());

    if (bytes_written == -1) {
        printf("indexStorage error: failed to pwrite zorder_list!\n");
    }
    close(fileFD);
    delete list;
    
    // ZOrderIPv4Meta* zorder_list = list->outputToZOrderIPv4();
    // u_int64_t num = list->getNodeNum();
    // ZOrderTreeIPv4* tree = new ZOrderTreeIPv4(zorder_list,num);
    // tree->buildTree();

    // u_int32_t ts_l = ts & 0xffffffff;
    // u_int32_t ts_h = ts >> sizeof(ts_h)*8;
    // std::string fileName = "./data/index/" + std::to_string(ts_h) + "." + std::to_string(ts_l) + "_" + std::to_string(num) + ".offset";

    // u_int64_t block_size = num * sizeof(ZOrderIPv4Meta);
    // // block_size = (block_size/PAGE_SIZE + 1) * PAGE_SIZE;

    // // int fileFD = open(fileName.c_str(), O_WRONLY | O_CREAT | O_DIRECT, 0644);
    // int fileFD = open(fileName.c_str(), O_WRONLY | O_CREAT, 0644);
    // if (fileFD == -1) {
    //     printf("indexStorage error: failed to open file of zorder_list!\n");
    // }
    // ssize_t bytes_written = write(fileFD, (char*)zorder_list , block_size);
    // // ssize_t bytes_written = pwrite(fileFD, (char*)zorder_list , block_size, 0);
    // if (bytes_written == -1) {
    //     printf("indexStorage error: failed to pwrite zorder_list!\n");
    // }
    // close(fileFD);

    // u_int64_t buffer_size = tree->getBufferSize();
    // fileName = "./data/index/" + std::to_string(ts_h) + "." + std::to_string(ts_l) + "_" + std::to_string(buffer_size) + "_" + std::to_string(tree->getMaxLevel()) + ".idx";
    // const char* data = tree->outputToChar();
    // block_size = buffer_size * sizeof(ZOrderTreeNode);
    // // block_size = (block_size/PAGE_SIZE + 1) * PAGE_SIZE;

    // // fileFD = open(fileName.c_str(), O_WRONLY | O_CREAT | O_DIRECT, 0644);
    // fileFD = open(fileName.c_str(), O_WRONLY | O_CREAT, 0644);
    // if (fileFD == -1) {
    //     printf("Index Storage error: failed to open file of rtree!\n");
    // }
    // // bytes_written = pwrite(fileFD, data, block_size, 0);
    // bytes_written = write(fileFD, data, block_size);
    // if (bytes_written == -1) {
    //     printf("IndexStorage error: failed to pwrite rtree!\n");
    // }
    // close(fileFD);

    // delete tree;
    // delete list;
}
bool IndexStorage::runUnit(){
    auto [list,ts] = this->buffer->getCache(this->checkID);
    if(list == nullptr){
        return false;
    }
    auto sk_lists = list->getCache();
    u_int32_t id = 0;
    for(auto sk:sk_lists){
        processSkipList(sk,ts,id);
        id++;
    }
    printf("Index Storage log: write id %u.\n",this->checkID);
    this->checkID ++;
    this->checkID %= this->cacheCount;
    return true;
}
void IndexStorage::run(){
    this->stop = false;
    while (true){
        bool change = this->runUnit();
        if(!change && this->stop){
            break;
        }
        if(!change){
            usleep(500000);
        }
    }
    auto [list,ts] = this->buffer->directGetCache(this->checkID);
    if(list == nullptr){
        return;
    }
    if(list->getNodeNum()==0){
        return;
    }
    printf("Index Storage log: direct write id %u.\n",this->checkID);
    auto sk_lists = list->getCache();
    u_int32_t id = 0;
    for(auto sk:sk_lists){
        processSkipList(sk,ts,id);
        id++;
    }
    printf("Index Storage log: thread quit.\n");
    return;
}
void IndexStorage::asynchronousStop(){
    this->stop = true;
}