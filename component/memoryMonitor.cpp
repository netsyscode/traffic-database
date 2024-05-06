#include "memoryMonitor.hpp"
#define IDSIZE 256
#define FLOW_META_NUM 4

// ShareBuffer* MemoryMonitor::makePacketBuffer(u_int32_t maxLength, u_int32_t warningLength){
//     auto packetbuffer = new ShareBuffer(maxLength,warningLength);
//     return packetbuffer;
// }
MmapBuffer* MemoryMonitor::makePacketBuffer(){
    auto packetbuffer = new MmapBuffer();
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

void MemoryMonitor::makeTraceCatcher(u_int32_t pcap_header_len, u_int32_t eth_header_len, std::string filename, u_int32_t id){
    // this->traceCatcher = new PcapReader(pcap_header_len,eth_header_len,filename,this->packetBuffer,this->memoryPool[0].packetPointer,this->trace_catcher_cv);
    PcapReader* traceCatcher = new PcapReader(pcap_header_len,eth_header_len,filename,this->packetBuffer,this->memoryPool[0].packetPointer,this->trace_catcher_cv,this->init_data.packet_warn,id);
    this->memoryPool[0].packetPointer->addWriteThread();
    this->traceCatchers.push_back(traceCatcher);
}
void MemoryMonitor::pushPacketAggregatorInit(u_int32_t eth_header_len){
    PacketAggregator* aggregator = new PacketAggregator(eth_header_len, this->packetBuffer, this->memoryPool[0].packetPointer, this->memoryPool[0].flowMetaIndexBuffers,this->packet_aggregator_cv);
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
    //     printf("Memory monitor log: skip list size of %llu .\n",ic->getNodeNum());
    //     //std::cout << "Memory monitor log: skip list size of " << ic->getNodeNum() << std::endl;
    // }
    // std::cout << "Memory monitor log: share buffer size of " << group.oldPacketBuffer->getLen() << std::endl;
    this->truncatePipe->put((void*)&group,this->threadId,sizeof(group));
}

void MemoryMonitor::makeMemoryPool(){
    int i = this->memoryPool.size();
    for(;i<this->memoryPoolSize;++i){
        MemoryGroup group = {
            // .packetBuffer = this->makePacketBuffer(this->init_data.buffer_len,this->init_data.buffer_warn),
            .packetPointer = this->makePacketPointer(this->init_data.packet_num,this->init_data.packet_warn),
            .flowMetaIndexBuffers = this->makeFlowMetaIndexBuffers(this->init_data.flow_capacity,this->flowMetaEleLens),
            .flowMetaIndexCaches = this->makeFlowMetaIndexCaches(this->flowMetaEleLens),
        };
        this->memoryPool.push_back(group);
    }
}
void MemoryMonitor::makeStaticComponent(){
    for(int i=0;i<init_data.pcapReaderThreadCount;++i){
        this->makeTraceCatcher(this->init_data.pcap_header.size(),this->init_data.eth_header_len,this->init_data.filename,i);
    }
    // this->makeTraceCatcher(this->init_data.pcap_header.size(),this->init_data.eth_header_len,this->init_data.filename);
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
    // this->traceCatcherThread = new std::thread(&PcapReader::run,this->traceCatcher);
    for(auto tc:this->traceCatchers){
        std::thread* t = new std::thread(&PcapReader::run,tc);
        this->traceCatcherThreads.push_back(t);
    }

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

    // std::unique_lock<std::mutex> lock_pa(this->mutex);
    for(auto pa:this->packetAggregators){
        while (true){
            if(!(pa->getPause())){
                break;
            }
        }
        // this->packet_aggregator_cv->wait(lock_pa, [pa]{return !(pa->getPause());});
    }
    // lock_pa.unlock();

    auto memory_group = this->memoryPool[0];
    this->memoryPool.erase(this->memoryPool.begin());

    TruncateGroup truncate_group = {
        // .oldPacketBuffer = memory_group.packetBuffer,
        .packetBuffer = this->packetBuffer,
        .oldPacketPointer = memory_group.packetPointer,
        // .newPacketBuffer = this->memoryPool[0].packetBuffer,
        .newPacketPointer = this->memoryPool[0].packetPointer,
        .flowMetaIndexBuffers = memory_group.flowMetaIndexBuffers,
        .flowMetaIndexCaches = memory_group.flowMetaIndexCaches,
        .flowMetaIndexGenerators = this->flowMetaIndexGenerators,
        .flowMetaIndexGeneratorPointers = this->flowMetaIndexGeneratorPointers,
        .flowMetaIndexGeneratorThreads = this->flowMetaIndexGeneratorThreads,
    };

    // std::cout << "Memory monitor log: traceCatcher truncate begin." << std::endl;
    for(auto tc:this->traceCatchers){
        tc->asynchronousPause(this->memoryPool[0].packetPointer);
    }
    
    // std::unique_lock<std::mutex> lock_tc(this->mutex);
    // for(auto tc:this->traceCatchers){
    //     this->trace_catcher_cv->wait(lock_tc, [tc]{return !(tc->getPause());});
    // }
    // lock_tc.unlock();

    for(auto tc:this->traceCatchers){
        while (true){
            if(!(tc->getPause())){
                break;
            }
        }
    }
    

    // std::cout << "Memory monitor log: traceCatcher truncate end." << std::endl;

    for(int i = 0;i<this->traceCatchers.size();++i){
        truncate_group.oldPacketPointer->ereaseWriteThread();
        this->memoryPool[0].packetPointer->addWriteThread();
    }

    // std::cout << "Memory monitor log: packetAggregator truncate begin." << std::endl;

    for(int i=0;i<this->packetAggregatorThreads.size();++i){
        this->packetAggregators[i]->asynchronousPause(this->memoryPool[0].packetPointer,this->memoryPool[0].flowMetaIndexBuffers,this->packetAggregatorPointers[i]);
        memory_group.packetPointer->asynchronousPause(this->packetAggregatorPointers[i]->id);
        // for(auto rb:*(truncate_group.flowMetaIndexBuffers)){
        //     rb->ereaseWriteThread(this->packetAggregatorPointers[i]);
        // }
    }

    // std::cout << "Memory monitor log: indexGenerator truncate begin." << std::endl;

    // make new dynamic components
    // this need to be optimized
    this->makeDynamicComponent();

    // std::cout << "Memory monitor log: indexGenerator run begin." << std::endl;

    this->dynamicThreadsRun();

    // if(this->stop){
    //     truncate_group.newPacketBuffer = nullptr;
    //     truncate_group.newPacketPointer = nullptr;
    //     // for(auto p:this->packetAggregatorPointers){
    //     //     truncate_group.oldPacketPointer->ereaseReadThread(p);
    //     // }
    // }

    // std::cout << "Memory monitor log: truncate wait." << std::endl;

    // std::cout << "Memory monitor log: putTruncateGroupToPipe begin." << std::endl;

    // pass truncate_group to storage_monitor
    this->putTruncateGroupToPipe(truncate_group);

    // std::cout << "Memory monitor log: makeMemoryPool begin." << std::endl;

    // make new memory
    this->makeMemoryPool();
    
    // std::cout << "Memory monitor log: truncate end." << std::endl;
}
void MemoryMonitor::monitor(){
    std::cout << "Memory monitor log: monitoring ..." << std::endl;
    while(!this->stop){
        // auto pb = this->memoryPool[0].packetBuffer;
        auto pb = this->memoryPool[0].packetPointer;
        // std::unique_lock<std::mutex> lock(this->mutex);
        // this->trace_catcher_cv->wait(lock, [pb,this]{return pb->getWarning() || this->stop;});
        // lock.unlock();
        while(true){
            if(pb->getWarning() || this->stop){
                break;
            }
        }
        this->truncate();
        if(this->stop){
            break;
        }
    }
    // std::cout << "Memory monitor log: monitor end." << std::endl;
}

void MemoryMonitor::threadsStop(){
    // std::cout << "Memory monitor log: threadsStop." <<std::endl;

    // std::unique_lock<std::mutex> lock(this->mutex);
    for(auto pa:this->packetAggregators){
        while (true){
            if(!(pa->getPause())){
                break;
            }
        }
        // this->packet_aggregator_cv->wait(lock, [pa]{return !(pa->getPause());});
    }
    // lock.unlock();

    // if(this->traceCatcherThread!=nullptr){
    //     this->traceCatcher->asynchronousStop();
    //     this->traceCatcherThread->join();
    //     delete this->traceCatcherThread;
    //     this->traceCatcherThread = nullptr;
    // }
    for(int i=0;i<this->traceCatcherThreads.size();++i){
        this->traceCatchers[i]->asynchronousStop();
        this->traceCatcherThreads[i]->join();
        delete this->traceCatcherThreads[i];
    }
    this->traceCatcherThreads.clear();

    for(int i=0;i<this->packetAggregatorThreads.size();++i){

        // wait truncate finish to avoid fault

        this->packetAggregators[i]->asynchronousStop();
        this->memoryPool[0].packetPointer->asynchronousStop(this->packetAggregatorPointers[i]->id);
        
        this->packetAggregatorThreads[i]->join();
        delete this->packetAggregatorThreads[i];
    }
    this->packetAggregatorThreads.clear();

    // printf("Memory monitor log: packetAggregatorThreads all clear.\n");
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
    // std::cout << "Memory monitor log: threadsClear." <<std::endl;
    if(this->flowMetaIndexGeneratorPointers != nullptr){
        for(int i=0;i<this->flowMetaIndexGeneratorPointers->size();++i){
            for(auto p:(*(this->flowMetaIndexGeneratorPointers))[i]){
                if(p==nullptr){
                    continue;
                }
                (*(this->memoryPool[0].flowMetaIndexBuffers))[i]->ereaseReadThread(p);
                delete p;
            }
            (*(this->flowMetaIndexGeneratorPointers))[i].clear();
        }
        this->flowMetaIndexGeneratorPointers->clear();
        delete this->flowMetaIndexGeneratorPointers;
        this->flowMetaIndexGeneratorPointers = nullptr;
    }

    if(this->flowMetaIndexGenerators != nullptr){
        for(int i=0;i<this->flowMetaIndexGenerators->size();++i){
            for(auto p:(*(this->flowMetaIndexGenerators))[i]){
                if(p==nullptr){
                    continue;
                }
                delete p;
            }
            this->flowMetaIndexGenerators[i].clear();
        }
        this->flowMetaIndexGenerators->clear();
        delete this->flowMetaIndexGenerators;
        this->flowMetaIndexGenerators = nullptr;
    }

    for(auto t:this->packetAggregatorPointers){
        if(t==nullptr){
            continue;
        }
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
    
    // if(this->traceCatcher!=nullptr){
    //     this->memoryPool[0].packetPointer->ereaseWriteThread();
    //     delete this->traceCatcher;
    //     this->traceCatcher = nullptr;
    // }
    for(auto tc:this->traceCatchers){
        this->memoryPool[0].packetPointer->ereaseWriteThread();
        delete tc;
    }
    this->traceCatchers.clear();
}
void MemoryMonitor::memoryClear(){
    for(auto m:this->memoryPool){
        // if(m.packetBuffer!=nullptr){
        //     delete m.packetBuffer;
        // }

        if(m.packetPointer!=nullptr){
            // std::cout << "Memory monitor log: test." << std::endl;
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
    if(this->packetBuffer!=nullptr){
        this->packetBuffer->closeFile();
        delete this->packetBuffer;
    }
}

void MemoryMonitor::init(InitData init_data){
    this->init_data = init_data;

    this->packetBuffer = this->makePacketBuffer();
    this->packetBuffer->changeFileName(init_data.filename);
    this->packetBuffer->openFile();

    this->makeMemoryPool();
    this->makeStaticComponent();
    this->makeDynamicComponent();
    std::cout << "Memory monitor log: init." << std::endl;
}
void MemoryMonitor::run(){

    // pthread_t threadId = pthread_self();

    // // 创建 CPU 集合，并将指定核心加入集合中
    // cpu_set_t cpuset;
    // CPU_ZERO(&cpuset);
    // CPU_SET(3, &cpuset);

    // // // 设置线程的 CPU 亲和性
    // int result = pthread_setaffinity_np(threadId, sizeof(cpu_set_t), &cpuset);

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
    this->trace_catcher_cv->notify_all();
    this->packet_aggregator_cv->notify_all();
}