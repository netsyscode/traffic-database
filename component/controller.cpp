#include "controller.hpp"
#define IDSIZE 256

void MultiThreadController::makePacketBuffer(u_int32_t maxLength, u_int32_t warningLength){
    this->packetBuffer = new ShareBuffer(maxLength,warningLength);
}
void MultiThreadController::makePacketPointer(u_int32_t maxLength, u_int32_t warningLength){
    this->packetPointer = new ArrayList<uint32_t>(maxLength,warningLength);
}
void MultiThreadController::makeTraceCatcher(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename){
    this->traceCatcher = new PcapReader(pcap_header_len,eth_header_len,filename,this->packetBuffer,this->packetPointer);
    this->packetPointer->addWriteThread();
}
void MultiThreadController::pushPacketAggregatorInit(u_int32_t eth_header_len){
    PacketAggregator* aggregator = new PacketAggregator(eth_header_len, this->packetBuffer, this->packetPointer);
    ThreadReadPointer* read_pointer = new ThreadReadPointer{(u_int32_t)(this->threadReadPointers.size()), std::mutex(), std::condition_variable(), std::atomic_bool(false)};
    if(this->packetPointer->addReadThread(read_pointer)){
        return;
    }
    aggregator->setThreadID(read_pointer->id);
    this->packetAggregators.push_back(aggregator);
    this->threadReadPointers.push_back(read_pointer);
};
void MultiThreadController::allocateID(){
    u_int32_t tmp = 0;
    u_int32_t threadCount = this->packetAggregators.size();
    u_int32_t mod = IDSIZE % threadCount;
    u_int32_t id_count_list[threadCount];
    for(int i=0;i<threadCount;++i){
        id_count_list[i] = i<mod ? IDSIZE/threadCount+1 : IDSIZE/threadCount;
    }
    u_int32_t num = 0;
    for(auto agg:this->packetAggregators){
        for(int i=0;i<id_count_list[num];++i){
            agg->addID(tmp);
            tmp++;
        }
        num++;
    }
}
void MultiThreadController::threadsRun(){
    this->traceCatcherThread = new std::thread(&PcapReader::run,this->traceCatcher);
    for(auto agg:this->packetAggregators){
        std::thread* t = new std::thread(&PacketAggregator::run,agg);
        this->packetAggregatorThreads.push_back(t);
    }
}

void MultiThreadController::threadsStop(){
    if(this->traceCatcherThread!=nullptr){
        this->traceCatcher->asynchronousStop();
        this->traceCatcherThread->join();
    }
    for(int i=0;i<this->packetAggregatorThreads.size();++i){
        this->packetAggregators[i]->asynchronousStop();
        this->packetAggregatorThreads[i]->join();
    }
}

void MultiThreadController::init(InitData init_data){
    this->makePacketBuffer(init_data.buffer_len,init_data.buffer_warn);
    this->makePacketPointer(init_data.packet_num,init_data.packet_warn);
    this->makeTraceCatcher(init_data.pcap_header_len,init_data.eth_header_len,init_data.filename);
    for(int i=0;i<init_data.threadCount;++i){
        this->pushPacketAggregatorInit(init_data.eth_header_len);
    }
    this->allocateID();
    std::cout << "Controller log: init." << std::endl;
}
void MultiThreadController::run(){
    if(this->packetAggregators.size()==0){
        std::cerr << "Controller error: run without init!" << std::endl;
        return;
    }
    this->threadsRun();

    //for test
    std::this_thread::sleep_for(std::chrono::seconds(1));
    this->threadsStop();
    
    std::cout << "Controller log: run quit." << std::endl;
}
const OutputData MultiThreadController::outputForTest(){
    const OutputData data = {
        .packetBuffer = this->packetBuffer,
        .packetPointer = this->packetPointer,
    };
    return data;
}