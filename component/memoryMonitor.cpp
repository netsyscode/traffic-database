#include "memoryMonitor.hpp"
#define IDSIZE 256
#define FLOW_META_NUM 4

ShareBuffer* MemoryMonitor::makePacketBuffer(u_int32_t maxLength, u_int32_t warningLength){
    auto packetbuffer = new ShareBuffer(maxLength,warningLength);
    return packetbuffer;
}
ArrayList<u_int32_t>* MemoryMonitor::makePacketPointer(u_int32_t maxLength, u_int32_t warningLength){
    auto packetPointer = new ArrayList<uint32_t>(maxLength,warningLength);
    return packetPointer;
}
std::vector<RingBuffer*>* MemoryMonitor::makeFlowMetaIndexBuffers(u_int32_t capacity, const std::vector<u_int32_t>& ele_lens){
    auto flowMetaIndexBuffers = new std::vector<RingBuffer*>();
    for(auto len:ele_lens){
        RingBuffer* ring_buffer = new RingBuffer(capacity, len+sizeof(u_int32_t));
        flowMetaIndexBuffers->push_back(ring_buffer);
        // flowMetaIndexGenerators.push_back(std::vector<IndexGenerator*>());
        // flowMetaIndexGeneratorPointers.push_back(std::vector<ThreadPointer*>());
    }
    return flowMetaIndexBuffers;
}
std::vector<SkipList*>* MemoryMonitor::makeFlowMetaIndexCaches(const std::vector<u_int32_t>& ele_lens){
    auto flowMetaIndexCaches = new std::vector<SkipList*>();
    for(auto len:ele_lens){
        SkipList* cache = new SkipList(len*8,len,sizeof(u_int32_t));
        flowMetaIndexCaches->push_back(cache);
    }
    return flowMetaIndexCaches;
}

void MemoryMonitor::makeTraceCatcher(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename){
    this->traceCatcher = new PcapReader(pcap_header_len,eth_header_len,filename,this->memoryPool[0].packetBuffer,this->memoryPool[0].packetPointer,this->cv);
    this->memoryPool[0].packetPointer->addWriteThread();
}
void MemoryMonitor::pushPacketAggregatorInit(u_int32_t eth_header_len){
    PacketAggregator* aggregator = new PacketAggregator(eth_header_len, this->memoryPool[0].packetBuffer, this->memoryPool[0].packetPointer, this->memoryPool[0].flowMetaIndexBuffers);
    ThreadPointer* read_pointer = new ThreadPointer{(u_int32_t)(this->packetAggregatorPointers.size()), std::mutex(), std::condition_variable(), std::atomic_bool(false),std::atomic_bool(false)};
    if(!this->memoryPool[0].packetPointer->addReadThread(read_pointer)){
        return;
    }
    for(auto rb:*(this->memoryPool[0].flowMetaIndexBuffers)){
        if(!rb->addWriteThread(read_pointer)){
            return;
        }
    }
    aggregator->setThreadID(read_pointer->id);
    this->packetAggregators.push_back(aggregator);
    this->packetAggregatorPointers.push_back(read_pointer);
}
void MemoryMonitor::allocateID(){
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

void MemoryMonitor::pushFlowMetaIndexGeneratorInit(const std::vector<u_int32_t>& ele_lens){
    for(auto len:ele_lens){
        this->flowMetaIndexGenerators->push_back(std::vector<IndexGenerator*>());
        this->flowMetaIndexGeneratorPointers->push_back(std::vector<ThreadPointer*>());
    }
    for(int i=0;i<this->memoryPool[0].flowMetaIndexBuffers->size();++i){
        IndexGenerator* generator = new IndexGenerator((*(this->memoryPool[0].flowMetaIndexBuffers))[i],(*(this->memoryPool[0].flowMetaIndexCaches))[i],this->flowMetaEleLens[i]);
        ThreadPointer* write_pointer = new ThreadPointer{
            (u_int32_t)((*(this->flowMetaIndexGeneratorPointers))[i].size()*this->memoryPool[0].flowMetaIndexBuffers->size() + i), 
            std::mutex(), std::condition_variable(), std::atomic_bool(false), std::atomic_bool(false)};
        if(!(*(this->memoryPool[0].flowMetaIndexBuffers))[i]->addReadThread(write_pointer)){
            return;
        }
        generator->setThreadID(write_pointer->id);
        (*(this->flowMetaIndexGenerators))[i].push_back(generator);
        (*(this->flowMetaIndexGeneratorPointers))[i].push_back(write_pointer);
    }
}

void MemoryMonitor::putTruncateGroupToPipe(TruncateGroup group){
    // for test
    // for(auto ic:*(group.flowMetaIndexCaches)){
    //     std::cout << "Memory monitor log: skip list size of " << ic->getNodeNum() << std::endl;
    // }
    std::cout << "Memory monitor log: skip list size of " << group.oldPacketBuffer->getLen() << std::endl;
    this->truncatePipe->put((void*)&group,this->threadId,sizeof(group));
}

void MemoryMonitor::makeMemoryPool(){
    int i = this->memoryPool.size();
    for(;i<this->memoryPoolSize;++i){
        MemoryGroup group = {
            .packetBuffer = this->makePacketBuffer(this->init_data.buffer_len,this->init_data.buffer_warn),
            .packetPointer = this->makePacketPointer(this->init_data.packet_num,this->init_data.packet_warn),
            .flowMetaIndexBuffers = this->makeFlowMetaIndexBuffers(this->init_data.flow_capacity,this->flowMetaEleLens),
            .flowMetaIndexCaches = this->makeFlowMetaIndexCaches(this->flowMetaEleLens),
        };
        this->memoryPool.push_back(group);
    }
}
void MemoryMonitor::makeStaticComponent(){
    this->makeTraceCatcher(this->init_data.pcap_header.size(),this->init_data.eth_header_len,this->init_data.filename);
    for(int i=0;i<init_data.packetAggregatorThreadCount;++i){
        this->pushPacketAggregatorInit(this->init_data.eth_header_len);
    }
    this->allocateID();
}
void MemoryMonitor::makeDynamicComponent(){
    this->flowMetaIndexGenerators = new std::vector<std::vector<IndexGenerator*>>();
    this->flowMetaIndexGeneratorPointers = new std::vector<std::vector<ThreadPointer*>>();
    for(int i=0;i<this->init_data.flowMetaIndexGeneratorThreadCountEach;++i){
        this->pushFlowMetaIndexGeneratorInit(this->flowMetaEleLens);
    }
}

void MemoryMonitor::staticThreadsRun(){
    this->traceCatcherThread = new std::thread(&PcapReader::run,this->traceCatcher);

    for(auto agg:this->packetAggregators){
        std::thread* t = new std::thread(&PacketAggregator::run,agg);
        this->packetAggregatorThreads.push_back(t);
    }
}
void MemoryMonitor::dynamicThreadsRun(){
    this->flowMetaIndexGeneratorThreads = new std::vector<std::vector<std::thread*>>();
    for(int i=0; i<this->flowMetaIndexGenerators->size();++i){
        this->flowMetaIndexGeneratorThreads->push_back(std::vector<std::thread*>());
        for(auto ger:(*(this->flowMetaIndexGenerators))[i]){
            std::thread* t = new std::thread(&IndexGenerator::run,ger);
            (*(this->flowMetaIndexGeneratorThreads))[i].push_back(t);
        }
    }
}

void MemoryMonitor::threadsRun(){
    this->staticThreadsRun();
    this->dynamicThreadsRun();    
}
void MemoryMonitor::truncate(){
    if(this->memoryPool.size()<=1){
        std::cerr << "Memory monitor error: truncate with too less memory in pool!" << std::endl;
    }
    std::cout << "Memory monitor log: truncate." << std::endl;

    // this needs to be optimized
    // for(int i=0;i<this->packetAggregatorThreads.size();++i){
    //     while (this->packetAggregators[i]->getPause());
    // }

    auto memory_group = this->memoryPool[0];
    this->memoryPool.erase(this->memoryPool.begin());

    TruncateGroup truncate_group = {
        .oldPacketBuffer = memory_group.packetBuffer,
        .oldPacketPointer = memory_group.packetPointer,
        .newPacketBuffer = this->memoryPool[0].packetBuffer,
        .newPacketPointer = this->memoryPool[0].packetPointer,
        .flowMetaIndexBuffers = memory_group.flowMetaIndexBuffers,
        .flowMetaIndexCaches = memory_group.flowMetaIndexCaches,
        .flowMetaIndexGenerators = this->flowMetaIndexGenerators,
        .flowMetaIndexGeneratorPointers = this->flowMetaIndexGeneratorPointers,
        .flowMetaIndexGeneratorThreads = this->flowMetaIndexGeneratorThreads,
    };

    this->traceCatcher->asynchronousPause(this->memoryPool[0].packetBuffer,this->memoryPool[0].packetPointer);
    auto tc = this->traceCatcher;
    
    std::unique_lock<std::mutex> lock(this->mutex);
    this->cv->wait(lock, [tc]{return !(tc->getPause());});
    lock.unlock();

    truncate_group.oldPacketPointer->ereaseWriteThread();
    this->memoryPool[0].packetPointer->addWriteThread();

    for(int i=0;i<this->packetAggregatorThreads.size();++i){
        this->packetAggregators[i]->asynchronousPause(this->memoryPool[0].packetBuffer,this->memoryPool[0].packetPointer,this->memoryPool[0].flowMetaIndexBuffers,this->packetAggregatorPointers[i]);
        memory_group.packetPointer->asynchronousPause(this->packetAggregatorPointers[i]->id);
        // for(auto rb:*(truncate_group.flowMetaIndexBuffers)){
        //     rb->ereaseWriteThread(this->packetAggregatorPointers[i]);
        // }
    }

    // make new dynamic components
    // this need to be optimized
    this->makeDynamicComponent();
    this->dynamicThreadsRun();

    // pass truncate_group to storage_monitor
    this->putTruncateGroupToPipe(truncate_group);

    // make new memory
    this->makeMemoryPool();
    
    std::cout << "Memory monitor log: truncate end." << std::endl;
}
void MemoryMonitor::monitor(){
    std::cout << "Memory monitor log: monitoring ..." << std::endl;
    while(!this->stop){
        auto pb = this->memoryPool[0].packetBuffer;
        std::unique_lock<std::mutex> lock(this->mutex);
        this->cv->wait(lock, [pb,this]{return pb->getWarning() || this->stop;});
        lock.unlock();
        this->truncate();
        if(this->stop){
            break;
        }
    }
}

void MemoryMonitor::threadsStop(){
    std::cout << "Memory monitor log: threadsStop." <<std::endl;

    if(this->traceCatcherThread!=nullptr){
        this->traceCatcher->asynchronousStop();
        this->traceCatcherThread->join();
        delete this->traceCatcherThread;
        this->traceCatcherThread = nullptr;
    }

    for(int i=0;i<this->packetAggregatorThreads.size();++i){

        // wait truncate finish to avoid fault
        while (this->packetAggregators[i]->getPause());

        this->packetAggregators[i]->asynchronousStop();
        this->memoryPool[0].packetPointer->asynchronousStop(this->packetAggregatorPointers[i]->id);
        
        this->packetAggregatorThreads[i]->join();
        delete this->packetAggregatorThreads[i];
    }
    this->packetAggregatorThreads.clear();

    if(this->flowMetaIndexGeneratorThreads != nullptr){    
        for(int i=0;i<this->flowMetaIndexGeneratorThreads->size();++i){
            for(int j=0;j<(*(this->flowMetaIndexGeneratorThreads))[i].size();++j){
                // std::cout << "Controller log: read thread " << this->flowMetaIndexGeneratorPointers[i][j]->id << " should stop." <<std::endl;
                if((*(this->flowMetaIndexGeneratorThreads))[i][j]!=nullptr){
                    (*(this->flowMetaIndexGenerators))[i][j]->asynchronousStop();
                    (*(this->memoryPool[0].flowMetaIndexBuffers))[i]->asynchronousStop((*(this->flowMetaIndexGeneratorPointers))[i][j]->id);
                    (*(this->flowMetaIndexGeneratorThreads))[i][j]->join();
                    delete (*(this->flowMetaIndexGeneratorThreads))[i][j];
                    (*(this->flowMetaIndexGeneratorThreads))[i][j] = nullptr;
                }
            }
            (*(this->flowMetaIndexGeneratorThreads))[i].clear();
        }
        this->flowMetaIndexGeneratorThreads->clear();
        delete this->flowMetaIndexGeneratorThreads;
        this->flowMetaIndexGeneratorThreads = nullptr;
    }
}
void MemoryMonitor::threadsClear(){
    if(this->flowMetaIndexGeneratorPointers != nullptr){
        for(int i=0;i<this->flowMetaIndexGeneratorPointers->size();++i){
            for(auto p:(*(this->flowMetaIndexGeneratorPointers))[i]){
                (*(this->memoryPool[0].flowMetaIndexBuffers))[i]->ereaseReadThread(p);
                delete p;
            }
            (*(this->flowMetaIndexGeneratorPointers))[i].clear();
        }
        this->flowMetaIndexGeneratorPointers->clear();
    }

    if(this->flowMetaIndexGenerators != nullptr){
        for(int i=0;i<this->flowMetaIndexGenerators->size();++i){
            for(auto p:(*(this->flowMetaIndexGenerators))[i]){
                delete p;
            }
            this->flowMetaIndexGenerators[i].clear();
        }
        this->flowMetaIndexGenerators->clear();
    }

    for(auto t:this->packetAggregatorPointers){
        this->memoryPool[0].packetPointer->ereaseReadThread(t);
        for(auto rb:*(this->memoryPool[0].flowMetaIndexBuffers)){
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
        this->memoryPool[0].packetPointer->ereaseWriteThread();
        delete this->traceCatcher;
        this->traceCatcher = nullptr;
    }
}
void MemoryMonitor::memoryClear(){
    for(auto m:this->memoryPool){
        if(m.packetBuffer!=nullptr){
            delete m.packetBuffer;
        }

        if(m.packetPointer!=nullptr){
            std::cout << "Memory monitor log: test." << std::endl;
            delete m.packetPointer;
        }

        if(m.flowMetaIndexCaches!=nullptr){
            for(auto ic:*(m.flowMetaIndexCaches)){
                if(ic==nullptr){
                    continue;
                }
                delete ic;
            }
            m.flowMetaIndexCaches->clear();
            delete m.flowMetaIndexCaches;
        }

        if(m.flowMetaIndexBuffers!=nullptr){
            for(auto rb:*(m.flowMetaIndexBuffers)){
                delete rb;
            }
            m.flowMetaIndexBuffers->clear();
            delete m.flowMetaIndexBuffers;
        }
    }
}

void MemoryMonitor::init(InitData init_data){
    this->init_data = init_data;
    this->makeMemoryPool();
    this->makeStaticComponent();
    this->makeDynamicComponent();
    std::cout << "Memory monitor log: init." << std::endl;
}
void MemoryMonitor::run(){
    if(this->flowMetaIndexGeneratorPointers == nullptr){
        std::cerr << "Memory monitor error: run without init!" << std::endl;
        return;
    }
    this->stop = false;
    std::cout << "Memory monitor log: run." << std::endl;
    this->threadsRun();
    this->monitor();
    
    this->threadsStop();
    this->threadsClear();

    std::cout << "Memory monitor log: run quit." << std::endl;
}
void MemoryMonitor::asynchronousStop(){
    this->stop = true;
    this->cv->notify_one();
}