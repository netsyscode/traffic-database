#include "controller.hpp"
#define IDSIZE 256
#define FLOW_META_NUM 4

void MultiThreadController::makePacketBuffer(u_int32_t maxLength, u_int32_t warningLength){
    this->packetBuffer = new ShareBuffer(maxLength,warningLength);
}
void MultiThreadController::makePacketPointer(u_int32_t maxLength, u_int32_t warningLength){
    this->packetPointer = new ArrayList<uint32_t>(maxLength,warningLength);
}
void MultiThreadController::makeFlowMetaIndexBuffers(u_int32_t capacity, const std::vector<u_int32_t>& ele_lens){
    this->flowMetaIndexBuffers = new std::vector<RingBuffer*>();
    for(auto len:ele_lens){
        RingBuffer* ring_buffer = new RingBuffer(capacity, len+sizeof(u_int32_t));
        this->flowMetaIndexBuffers->push_back(ring_buffer);
        this->flowMetaIndexGenerators.push_back(std::vector<IndexGenerator*>());
        this->flowMetaIndexGeneratorPointers.push_back(std::vector<ThreadPointer*>());
    }
}
bool MultiThreadController::makeFlowMetaIndexCaches(const std::vector<u_int32_t>& ele_lens){
    bool ret = true;
    this->flowMetaIndexCaches = new std::vector<SkipList*>();
    for(auto len:ele_lens){
        SkipList* cache = new SkipList(len*8,len,sizeof(u_int32_t));
        this->flowMetaIndexCaches->push_back(cache);
    }
    return ret;
}
void MultiThreadController::makeTraceCatcher(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename){
    this->traceCatcher = new PcapReader(pcap_header_len,eth_header_len,filename,this->packetBuffer,this->packetPointer);
    this->packetPointer->addWriteThread();
}
void MultiThreadController::pushPacketAggregatorInit(u_int32_t eth_header_len){
    PacketAggregator* aggregator = new PacketAggregator(eth_header_len, this->packetBuffer, this->packetPointer, this->flowMetaIndexBuffers);
    ThreadPointer* read_pointer = new ThreadPointer{(u_int32_t)(this->packetAggregatorPointers.size()), std::mutex(), std::condition_variable(), std::atomic_bool(false)};
    if(!this->packetPointer->addReadThread(read_pointer)){
        return;
    }
    for(auto rb:*(this->flowMetaIndexBuffers)){
        if(!rb->addWriteThread(read_pointer)){
            return;
        }
    }
    aggregator->setThreadID(read_pointer->id);
    this->packetAggregators.push_back(aggregator);
    this->packetAggregatorPointers.push_back(read_pointer);
};
void MultiThreadController::pushFlowMetaIndexGeneratorInit(){
    for(int i=0;i<this->flowMetaIndexBuffers->size();++i){
        IndexGenerator* generator = new IndexGenerator((*(this->flowMetaIndexBuffers))[i],(*(this->flowMetaIndexCaches))[i],this->flowMetaEleLens[i]);
        ThreadPointer* write_pointer = new ThreadPointer{
            (u_int32_t)(this->flowMetaIndexGeneratorPointers[i].size()*this->flowMetaIndexBuffers->size() + i), 
            std::mutex(), std::condition_variable(), std::atomic_bool(false)};
        if(!(*(this->flowMetaIndexBuffers))[i]->addReadThread(write_pointer)){
            return;
        }
        generator->setThreadID(write_pointer->id);
        this->flowMetaIndexGenerators[i].push_back(generator);
        this->flowMetaIndexGeneratorPointers[i].push_back(write_pointer);
    }
}
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

    for(int i=0; i<this->flowMetaIndexGenerators.size();++i){
        this->flowMetaIndexGeneratorThreads.push_back(std::vector<std::thread*>());
        for(auto ger:this->flowMetaIndexGenerators[i]){
            std::thread* t = new std::thread(&IndexGenerator::run,ger);
            this->flowMetaIndexGeneratorThreads[i].push_back(t);
        }
    }
}

void MultiThreadController::queryThreadRun(){
    this->querier = new Querier(this->packetBuffer,this->packetPointer,this->flowMetaIndexCaches,this->pcapHeader);
    this->queryThread = new std::thread(&Querier::run,this->querier);
}

void MultiThreadController::threadsStop(){
    std::cout << "Controller log: threadsStop." <<std::endl;
    if(this->queryThread!=nullptr){// queryThread must stop when threadsStop
        delete this->queryThread;
        this->queryThread = nullptr;
    }

    if(this->traceCatcherThread!=nullptr){
        this->traceCatcher->asynchronousStop();
        this->traceCatcherThread->join();
        delete this->traceCatcherThread;
        this->traceCatcherThread = nullptr;
    }

    for(int i=0;i<this->packetAggregatorThreads.size();++i){
        // std::cout << "Controller log: thread " << this->packetAggregatorPointers[i]->id << " should stop." <<std::endl;
        this->packetAggregators[i]->asynchronousStop();
        this->packetPointer->asynchronousStop(this->packetAggregatorPointers[i]->id);
        
        this->packetAggregatorThreads[i]->join();
        delete this->packetAggregatorThreads[i];
    }
    this->packetAggregatorThreads.clear();

    for(int i=0;i<this->flowMetaIndexGeneratorThreads.size();++i){
        for(int j=0;j<this->flowMetaIndexGeneratorThreads[i].size();++j){
            // std::cout << "Controller log: read thread " << this->flowMetaIndexGeneratorPointers[i][j]->id << " should stop." <<std::endl;
            this->flowMetaIndexGenerators[i][j]->asynchronousStop();
            (*(this->flowMetaIndexBuffers))[i]->asynchronousStop(this->flowMetaIndexGeneratorPointers[i][j]->id);
            this->flowMetaIndexGeneratorThreads[i][j]->join();
            delete this->flowMetaIndexGeneratorThreads[i][j];
        }
        this->flowMetaIndexGeneratorThreads[i].clear();
    }
    this->flowMetaIndexGeneratorThreads.clear();
}
void MultiThreadController::threadsClear(){
    for(int i=0;i<this->flowMetaIndexGeneratorPointers.size();++i){
        for(auto p:this->flowMetaIndexGeneratorPointers[i]){
            (*(this->flowMetaIndexBuffers))[i]->ereaseReadThread(p);
            delete p;
        }
        this->flowMetaIndexGeneratorPointers[i].clear();
    }
    this->flowMetaIndexGeneratorPointers.clear();
    for(int i=0;i<this->flowMetaIndexGenerators.size();++i){
        for(auto p:this->flowMetaIndexGenerators[i]){
            delete p;
        }
        this->flowMetaIndexGenerators[i].clear();
    }
    this->flowMetaIndexGenerators.clear();

    for(auto t:this->packetAggregatorPointers){
        this->packetPointer->ereaseReadThread(t);
        for(auto rb:*(this->flowMetaIndexBuffers)){
            rb->ereaseWriteThread(t);
        }
        delete t;
    }
    this->packetAggregatorPointers.clear();
    for(auto p:this->packetAggregators){
        delete p;
    }
    this->packetAggregators.clear();
    
    if(this->traceCatcher!=nullptr){
        this->packetPointer->ereaseWriteThread();
        delete this->traceCatcher;
        this->traceCatcher = nullptr;
    }
}

void MultiThreadController::init(InitData init_data){
    // if(this->packetAggregatorsMaxCount > init_data.packetAggregatorThreadCount){
    //     std::cerr << "Controller error: init with too many packetAggregators!" << std::endl;
    // }
    this->pcapHeader = init_data.pcap_header;
    this->makePacketBuffer(init_data.buffer_len,init_data.buffer_warn);
    this->makePacketPointer(init_data.packet_num,init_data.packet_warn);

    this->makeFlowMetaIndexBuffers(init_data.flow_capacity,this->flowMetaEleLens);
    if(!this->makeFlowMetaIndexCaches(this->flowMetaEleLens)){
        return;
    }

    this->makeTraceCatcher(init_data.pcap_header.size(),init_data.eth_header_len,init_data.filename);
    for(int i=0;i<init_data.packetAggregatorThreadCount;++i){
        this->pushPacketAggregatorInit(init_data.eth_header_len);
    }
    this->allocateID();
    for(int i=0;i<init_data.flowMetaIndexGeneratorThreadCountEach;++i){
        this->pushFlowMetaIndexGeneratorInit();
    }
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
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    this->queryThreadRun();
    this->queryThread->join();
    
    this->threadsStop();
    this->threadsClear();

    std::cout << "Controller log: run quit." << std::endl;
}
const OutputData MultiThreadController::outputForTest(){
    const OutputData data = {
        .packetBuffer = this->packetBuffer,
        .packetPointer = this->packetPointer,
        .flowMetaIndexBuffers = this->flowMetaIndexBuffers,
        .flowMetaIndexCaches = this->flowMetaIndexCaches,
        .flowMetaEleLens = this->flowMetaEleLens,
    };
    return data;
}