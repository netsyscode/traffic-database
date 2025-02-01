#include "controller.hpp"

void Controller::threadsRun(){
    // this->storageThread = new std::thread(&Storage::run,this->storage);

    // this->checkThread = new std::thread(&TruncateChecker::run,this->checker);
    // std::thread t = std::thread(&DPDKReader::run,readers[0]);

    // sleep(3);

    for(auto s:this->indexStorages){
        std::thread* indexStorageThread = new std::thread(&IndexStorage::run, s);
        this->indexStorageThreads.push_back(indexStorageThread);
    }

    for(auto s:this->directStorages){
        std::thread* directStorageThread = new std::thread(&DirectStorage::run, s);
        this->directStorageThreads.push_back(directStorageThread);
    }

    for(auto ig:this->indexGenerators){
        std::thread* t = new std::thread(&IndexGenerator::run,ig);
        this->indexGeneratorThreads.push_back(t);
    }

    unsigned lcore_id;
    u_int16_t queue_id = 0;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        printf("Worker core ID: %u\n", queue_id);
        int ret = rte_eal_remote_launch(&DPDKReader::launch, readers[queue_id], lcore_id);
        queue_id ++;
        if (ret < 0){
            rte_exit(EXIT_FAILURE, "Error launching lcore %u\n", lcore_id);
        }
    }
    
    // readers[0]->asynchronousStop();
    // t.join();
}

// void Controller::queryThreadRun(){
//     this->querierThread = new std::thread(&Querier::run,this->querier);
// }

void Controller::threadsStop(){
    for(auto r:this->readers){
        r->asynchronousStop();
    }
    unsigned lcore_id;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        rte_eal_wait_lcore(lcore_id);
        printf("lcore %u stop.\n",lcore_id);
    }

    for(auto ir:*this->indexRings){
        ir->asynchronousStop();
    }
    for(u_int32_t i=0;i<this->indexGenerators.size();++i){
        this->indexGenerators[i]->asynchronousStop();
        this->indexGeneratorThreads[i]->join();
    }

    for(u_int32_t i=0;i<this->directStorages.size();++i){
        this->directStorages[i]->asynchronousStop();
        this->directStorageThreads[i]->join();
    }

    for(u_int32_t i=0;i<this->indexStorages.size();++i){
        this->indexStorages[i]->asynchronousStop();
        this->indexStorageThreads[i]->join();
    }
    

    // for(int i=0;i<INDEX_NUM;++i){
    //     (*(this->indexRings))[i]->asynchronousStop();
    // }

    // for(int i=0;i<INDEX_NUM;++i){
    //     printf("indexGenerator %d should stop.\n",i);
    //     (*(this->indexRings))[i]->asynchronousStop();
    
    // }

    // this->checker->asynchronousStop();
    // this->checkThread->join();

    // this->storageRing->asynchronousStop();
    // this->storage->asynchronousStop();
    // this->storageThread->join();
}

void Controller::clear(){
    // if(this->checkThread!=nullptr){
    //     delete this->checkThread;
    //     this->checkThread = nullptr;
    // }
    // if(this->storageThread!=nullptr){
    //     delete this->storageThread;
    //     this->storageThread = nullptr;
    // }
    // if(this->checker!=nullptr){
    //     delete this->checker;
    //     this->checker = nullptr;
    // }
    // if(this->storage!=nullptr){
    //     delete this->storage;
    //     this->storage = nullptr;
    // }
    // if(this->indexGenerators.size()!=0){
    //     for(int i=0;i<INDEX_NUM;++i){
    if(this->indexGenerators.size()!=0){
        for(u_int32_t i=0;i<this->indexGenerators.size();++i){
            delete this->indexGenerators[i];
            delete this->indexGeneratorThreads[i];
        }
        this->indexGenerators.clear();
        this->indexGeneratorThreads.clear();
    }
    //         this->indexGenerators[i].clear();
    //         this->indexGeneratorThreads[i].clear();
    //     }
    //     this->indexGenerators.clear();
    //     this->indexGeneratorThreads.clear();
    // }
    for(u_int32_t i=0;i<this->directStorageThreads.size();++i){
        if(this->directStorageThreads[i]!=nullptr){
            delete this->directStorageThreads[i];
            this->directStorageThreads[i] = nullptr;
        }
    }
    this->directStorageThreads.clear();

    for(u_int32_t i=0;i<this->indexStorageThreads.size();++i){
        if(this->indexStorageThreads[i]!=nullptr){
            delete this->indexStorageThreads[i];
            this->indexStorageThreads[i] = nullptr;
        }
    }
    this->indexStorageThreads.clear();
    
    if(this->readers.size()!=0){
        for(u_int16_t i = 0;i<this->readers.size();++i){
            delete readers[i];
        }
        readers.clear();
    }

    for(u_int32_t i=0;i<this->indexStorages.size();++i){
        if(this->indexStorages[i]!=nullptr){
            delete this->indexStorages[i];
            this->indexStorages[i] = nullptr;
        }
    }
    this->indexStorages.clear();

    for(u_int32_t i=0;i<this->directStorages.size();++i){
        if(this->directStorages[i]!=nullptr){
            delete this->directStorages[i];
            this->directStorages[i] = nullptr;
        }
    }
    this->directStorages.clear();
    
    for(u_int32_t i = 0;i<this->buffers.size();++i){
        delete buffers[i];
    }
    this->buffers.clear();

    if(this->dpdk!=nullptr){
        delete this->dpdk;
        this->dpdk = nullptr;
    }

    // if(this->storageMetas!=nullptr){
    //     delete[] this->storageMetas;
    //     this->storageMetas = nullptr;
    // }
    // if(this->storageRing!=nullptr){
    //     delete this->storageRing;
    //     this->storageRing = nullptr;
    // }
    // if(this->truncators!=nullptr){
    //     for(int i=0;i<INDEX_NUM;++i){
    //         delete (*(this->truncators))[i];
    //     }
    //     this->truncators->clear();
    //     delete this->truncators;
    // }
    if(this->indexRings!=nullptr){
        for(auto ib:(*(this->indexRings))){
            if(ib!=nullptr){
                delete ib;
                ib = nullptr;
            }
        }
        delete this->indexRings;
        this->indexRings = nullptr;
    }
    if(this->indexBuffers != nullptr){
        for(auto ib:(*(this->indexBuffers))){
            if(ib!=nullptr){
                delete ib;
                ib = nullptr;
            }
        }
        delete this->indexBuffers;
        this->indexBuffers = nullptr;
    }
    // if(this->querier!=nullptr){
    //     delete this->querier;
    // }
    // if(this->querierThread!=nullptr){
    //     delete this->querierThread;
    // }
}

void Controller::init(InitData init_data){
    if(init_data.bind_core){
        this->bindCore(init_data.controller_core_id);
    }
    // this->bindCore(24);

    // for(u_int32_t i=0;i<flowMetaEleLens.size();++i){
    //     PointerRingBuffer* ir =  new PointerRingBuffer(init_data.index_ring_capacity);
    //     this->indexRings->push_back(ir);
    // }

    PointerRingBuffer* ir =  new PointerRingBuffer(init_data.index_ring_capacity);
    this->indexRings->push_back(ir);


    this->dpdk = new DPDK(init_data.nb_rx,1,init_data.bind_core,init_data.dpdk_core_id_list);
    
    for(u_int32_t i=0;i<init_data.direct_storage_thread_num;++i){
        DirectStorage* directStorage = new DirectStorage(i);
        this->directStorages.push_back(directStorage);
    }

    for(u_int32_t i=0;i<init_data.index_storage_thread_num;++i){
        // IndexStorage* indexStorage = new IndexStorage((*(this->indexBuffers))[i%flowMetaEleLens.size()],i%flowMetaEleLens.size());
        IndexStorage* indexStorage = new IndexStorage();
        this->indexStorages.push_back(indexStorage);
    }

    std::vector<SkipListMeta> metas = std::vector<SkipListMeta>();
    for(auto len:flowMetaEleLens){
        SkipListMeta meta = {
            .keyLen = len,
            .valueLen = sizeof(u_int64_t),
            .maxLvl = len*8,
        };
        metas.push_back(meta);
    }

    SkipListMeta tagMeta = {
        .keyLen = sizeof(u_int64_t),
        .valueLen = sizeof(u_int64_t),
        .maxLvl = sizeof(u_int64_t)*8,
    };

    for(u_int32_t i=0;i<flowMetaEleLens.size();++i){
        IndexBuffer* ib = new IndexBuffer(5,metas[i],init_data.max_node);
        this->indexStorages[i%init_data.index_storage_thread_num]->addBuffer(ib,i);
        (*(this->indexBuffers)).push_back(ib);
    }

    for(u_int32_t i=0;i<MAX_TAG_TYPE;++i){
        IndexBuffer* ib = new IndexBuffer(5,tagMeta,init_data.max_node);
        this->indexStorages[(i + flowMetaEleLens.size()) %init_data.index_storage_thread_num]->addBuffer(ib,i + flowMetaEleLens.size());
        (*(this->indexBuffers)).push_back(ib);
    }

    for(u_int32_t i=0;i<init_data.index_thread_num;++i){
        // IndexGenerator* ig = new IndexGenerator((*(this->indexRings))[0],this->indexBuffers,(*(this->indexBuffers))[0]->getCacheCount(),i*2+42);
        IndexGenerator* ig;
        if (init_data.bind_core){
            ig = new IndexGenerator((*(this->indexRings))[0],this->indexBuffers,(*(this->indexBuffers))[0]->getCacheCount(),i,init_data.bind_core,init_data.indexing_core_id_list[i]);
        }else{
            ig = new IndexGenerator((*(this->indexRings))[0],this->indexBuffers,(*(this->indexBuffers))[0]->getCacheCount(),i);
        }
        this->indexGenerators.push_back(ig);
    }

    for(u_int16_t i = 0;i<init_data.nb_rx;++i){
        std::string file_name = "./data/input/" + std::to_string(0) + "-" + std::to_string(i) + ".pcap";
        MemoryBuffer* buffer = new MemoryBuffer(0,init_data.file_capacity,5,file_name);
        this->buffers.push_back(buffer);
        this->directStorages[i%init_data.direct_storage_thread_num]->addBuffer(buffer);
        DPDKReader* reader;
        if (init_data.bind_core){
            reader = new DPDKReader(init_data.pcap_header_len,init_data.eth_header_len,dpdk,this->indexRings,0,i,init_data.file_capacity,buffer, init_data.bind_core, init_data.packet_core_id_list[i]);
        }else{
            reader = new DPDKReader(init_data.pcap_header_len,init_data.eth_header_len,dpdk,this->indexRings,0,i,init_data.file_capacity,buffer, init_data.bind_core, 0);
        }
        this->readers.push_back(reader);
    }

    printf("Detected logical cores: %u\n", rte_lcore_count());

    this->bpf_prog_name = init_data.bpf_prog_name;

    // this->storage = new Storage(this->storageRing,this->storageMetas);

    // this->checker = new TruncateChecker(this->truncators);

    // this->pcapHeader = init_data.pcap_header;

    // this->querier = new Querier(this->storageMetas,init_data.pcap_header);
}

void Controller::bindCore(u_int32_t cpu){
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    pthread_t thread = pthread_self();

    int set_result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (set_result != 0) {
        std::cerr << "Error setting thread affinity: " << set_result << std::endl;
    }

    // 确认设置是否成功
    CPU_ZERO(&cpuset);
    pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

    if (CPU_ISSET(cpu, &cpuset)) {
        printf("Controller log: %lu bind to cpu %d.\n",thread,cpu);
    } else {
        printf("Controller warning: %lu failed to bind to cpu %d!\n",thread,cpu);
    }
}

void Controller::run(){

    std::cout << "Controller log: run." << std::endl;
    this->threadsRun();

    // this->queryThreadRun();

    printf("wait.\n");
    
    for(u_int16_t i=0; i<this->readers.size(); ++i){
        if(this->dpdk->loadBPF(0, i, this->bpf_prog_name)){
            printf("Controller error: load bpf fail at %u\n",i);
        }
    }
    // std::this_thread::sleep_for(std::chrono::seconds(4));
    // for(u_int16_t i=0; i<this->readers.size(); ++i){
    //     this->dpdk->unloadBPF(0, i);
    // }
    // std::this_thread::sleep_for(std::chrono::seconds(1));

    char stop;

    std::cin>>stop;
    
    // this->querierThread->join();
    this->threadsStop();
}