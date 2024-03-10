#include "controller.hpp"
#define IDSIZE 256
#define FLOW_META_NUM 4

void MultiThreadController::makePacketBuffer(u_int32_t maxLength, u_int32_t warningLength){
    this->packetBuffer = new ShareBuffer(maxLength,warningLength);
}
void MultiThreadController::makePacketPointer(u_int32_t maxLength, u_int32_t warningLength){
    this->packetPointer = new ArrayList<uint32_t>(maxLength,warningLength);
}
void MultiThreadController::makeFlowMetaIndexBuffers(u_int32_t capacity, std::vector<u_int32_t> ele_lens){
    this->flowMetaIndexBuffers = new std::vector<RingBuffer*>();
    for(auto len:ele_lens){
        RingBuffer* ring_buffer = new RingBuffer(capacity, len);
        this->flowMetaIndexBuffers->push_back(ring_buffer);
    }
}
void MultiThreadController::makeTraceCatcher(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename){
    this->traceCatcher = new PcapReader(pcap_header_len,eth_header_len,filename,this->packetBuffer,this->packetPointer);
    this->packetPointer->addWriteThread();
}
void MultiThreadController::pushPacketAggregatorInit(u_int32_t eth_header_len){
    PacketAggregator* aggregator = new PacketAggregator(eth_header_len, this->packetBuffer, this->packetPointer, this->flowMetaIndexBuffers);
    ThreadReadPointer* read_pointer = new ThreadReadPointer{(u_int32_t)(this->threadReadPointers.size()), std::mutex(), std::condition_variable(), std::atomic_bool(false)};
    if(!this->packetPointer->addReadThread(read_pointer)){
        return;
    }
    aggregator->setThreadID(read_pointer->id);
    this->packetAggregators.push_back(aggregator);
    this->threadReadPointers.push_back(read_pointer);
    for(auto rb:*(this->flowMetaIndexBuffers)){
        if(!rb->addWriteThread(read_pointer)){
            return;
        }
    }
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
        delete this->traceCatcherThread;
        this->traceCatcherThread = nullptr;
    }
    for(int i=0;i<this->packetAggregatorThreads.size();++i){
        this->packetAggregators[i]->asynchronousStop();
        this->packetPointer->asynchronousStop(this->threadReadPointers[i]->id);
        for(auto rb:*(this->flowMetaIndexBuffers)){
            rb->asynchronousStop(this->threadReadPointers[i]->id);
        }
        this->packetAggregatorThreads[i]->join();
        delete this->packetAggregatorThreads[i];
    }
    this->packetAggregatorThreads.clear();
}
void MultiThreadController::threadsClear(){
    for(auto t:this->threadReadPointers){
        this->packetPointer->ereaseReadThread(t);
        for(auto rb:*(this->flowMetaIndexBuffers)){
            rb->ereaseWriteThread(t);
        }
        delete t;
    }
    this->threadReadPointers.clear();
    for(auto p:this->packetAggregators){
        delete p;
    }
    this->packetAggregators.clear();
    // this->packetAggregators.clear();
    if(this->traceCatcher!=nullptr){
        this->packetPointer->ereaseWriteThread();
        delete this->traceCatcher;
        this->traceCatcher = nullptr;
    }
}

void MultiThreadController::init(InitData init_data){
    this->makePacketBuffer(init_data.buffer_len,init_data.buffer_warn);
    this->makePacketPointer(init_data.packet_num,init_data.packet_warn);

    std::vector<u_int32_t> ele_lens = {4 + sizeof(u_int32_t), 4 + sizeof(u_int32_t), 2 + sizeof(u_int32_t), 2 + sizeof(u_int32_t)};
    this->makeFlowMetaIndexBuffers(init_data.flow_capacity,ele_lens);

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
    std::cout << "Controller log: run." << std::endl;
    this->threadsRun();

    //for test
    std::this_thread::sleep_for(std::chrono::seconds(1));
    this->threadsStop();
    this->threadsClear();

    std::cout << "Controller log: run quit." << std::endl;
}
const OutputData MultiThreadController::outputForTest(){
    const OutputData data = {
        .packetBuffer = this->packetBuffer,
        .packetPointer = this->packetPointer,
    };
    return data;
}