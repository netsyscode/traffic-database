#include "controller.hpp"

void Controller::threadsRun(){
    // this->storageThread = new std::thread(&Storage::run,this->storage);

    // this->checkThread = new std::thread(&TruncateChecker::run,this->checker);

    // for(int i=0;i<INDEX_NUM;++i){
    //     for(auto ig:indexGenerators[i]){
    //         std::thread* t = new std::thread(&IndexGenerator::run,ig);
    //         this->indexGeneratorThreads[i].push_back(t);
    //     }
    // }

    for(auto s:this->directStorages){
        std::thread* directStorageThread = new std::thread(&DirectStorage::run, s);
        this->directStorageThreads.push_back(directStorageThread);
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

    for(u_int32_t i=0;i<this->directStorages.size();++i){
        this->directStorages[i]->asynchronousStop();
        this->directStorageThreads[i]->join();
    }
    

    // for(int i=0;i<INDEX_NUM;++i){
    //     (*(this->indexRings))[i]->asynchronousStop();
    // }

    // for(int i=0;i<INDEX_NUM;++i){
    //     printf("indexGenerator %d should stop.\n",i);
    //     (*(this->indexRings))[i]->asynchronousStop();
    //     for(u_int32_t j=0;j<this->indexGenerators[i].size();++j){
    //         this->indexGenerators[i][j]->asynchronousStop();
    //         this->indexGeneratorThreads[i][j]->join();
    //     }
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
    //         if(this->indexGenerators[i].size()!=0){
    //             for(u_int32_t j=0;j<this->indexGenerators[i].size();++j){
    //                 delete this->indexGenerators[i][j];
    //                 delete this->indexGeneratorThreads[i][j];
    //             }
    //             this->indexGenerators[i].clear();
    //             this->indexGeneratorThreads[i].clear();
    //         }
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
    
    if(this->readers.size()!=0){
        for(u_int16_t i = 0;i<this->readers.size();++i){
            delete readers[i];
        }
        readers.clear();
    }
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
    // if(this->indexRings!=nullptr){
    //     for(int i=0;i<INDEX_NUM;++i){
    //         delete (*(this->indexRings))[i];
    //     }
    //     this->indexRings->clear();
    //     delete this->indexRings;
    // }
    // if(this->querier!=nullptr){
    //     delete this->querier;
    // }
    // if(this->querierThread!=nullptr){
    //     delete this->querierThread;
    // }
}

void Controller::init(InitData init_data){
    this->indexRing =  new PointerRingBuffer(init_data.index_ring_capacity);
    // for(int i=0;i<INDEX_NUM;++i){
    //     (*(this->indexRings))[i] = new PointerRingBuffer(init_data.index_ring_capacity);
    // }
    // this->storageRing = new PointerRingBuffer(init_data.storage_ring_capacity);
    // for(int i=0;i<INDEX_NUM;++i){
    //     (*(this->truncators))[i] = new Truncator(init_data.truncate_interval,flowMetaEleLens[i]*8,flowMetaEleLens[i],sizeof(u_int64_t),this->storageRing,(IndexType)i);
    // }
    // this->storageMetas = new std::vector<StorageMeta>[INDEX_NUM]();
    this->dpdk = new DPDK(init_data.nb_rx,1);
    
    for(u_int32_t i=0;i<init_data.direct_storage_thread_num;++i){
        DirectStorage* directStorage = new DirectStorage(i);
        this->directStorages.push_back(directStorage);
    }

    for(u_int16_t i = 0;i<init_data.nb_rx;++i){
        std::string file_name = "./data/input/" + std::to_string(0) + "-" + std::to_string(i) + ".pcap";
        MemoryBuffer* buffer = new MemoryBuffer(0,init_data.file_capacity,5,file_name);
        this->buffers.push_back(buffer);
        this->directStorages[i%init_data.direct_storage_thread_num]->addBuffer(buffer);
        DPDKReader* reader = new DPDKReader(init_data.pcap_header_len,init_data.eth_header_len,dpdk,this->indexRing,0,i,init_data.file_capacity,buffer);
        this->readers.push_back(reader);
    }

    printf("Detected logical cores: %u\n", rte_lcore_count());

    // for(int i=0;i<INDEX_NUM;++i){
    //     for(u_int32_t j=0;j<init_data.index_thread_num;++j){
    //         IndexGenerator* ig = new IndexGenerator((*(this->indexRings))[i],(*(this->truncators))[i],flowMetaEleLens[i]);
    //         ig->setThreadID(i*init_data.index_thread_num+j);
    //         this->indexGenerators[i].push_back(ig);
    //     }
    // }
    this->bpf_prog_name = init_data.bpf_prog_name;

    // this->storage = new Storage(this->storageRing,this->storageMetas);

    // this->checker = new TruncateChecker(this->truncators);

    // this->pcapHeader = init_data.pcap_header;

    // this->querier = new Querier(this->storageMetas,init_data.pcap_header);
}

void Controller::run(){
    std::cout << "Controller log: run." << std::endl;
    this->threadsRun();

    // this->queryThreadRun();

    printf("wait.\n");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    for(u_int16_t i=0; i<this->readers.size(); ++i){
        if(this->dpdk->loadBPF(0, i, this->bpf_prog_name)){
            printf("Controller error: load bpf fail at %u\n",i);
        }
    }
    std::this_thread::sleep_for(std::chrono::seconds(4));
    // for(u_int16_t i=0; i<this->readers.size(); ++i){
    //     this->dpdk->unloadBPF(0, i);
    // }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // this->querierThread->join();
    this->threadsStop();
}