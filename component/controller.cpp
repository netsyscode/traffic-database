#include "controller.hpp"
#define IDSIZE 256
#define FLOW_META_NUM 4
#define MEMORY_MONITOR_ID 100
#define STORAGE_MONITOR_ID 100

void MultiThreadController::makeMemoryStoragePipe(){
    this->memoryStoragePipe = new RingBuffer(this->capacity,sizeof(TruncateGroup));
}
void MultiThreadController::makeStorageMetas(){
    this->storageMetas = new std::vector<StorageMeta>();
}
void MultiThreadController::makeMemoryMonitor(){
    this->memoryMonitor = new MemoryMonitor(this->memoryStoragePipe, MEMORY_MONITOR_ID);
    this->memoryMonitorPointer = new ThreadPointer{
        MEMORY_MONITOR_ID, std::mutex(), std::condition_variable(), std::atomic_bool(false), std::atomic_bool(false),
    };
    this->memoryStoragePipe->addWriteThread(this->memoryMonitorPointer);
}
void MultiThreadController::makeStorageMonitor(){
    this->storageMonitor = new StorageMonitor(this->memoryStoragePipe,this->storageMetas,STORAGE_MONITOR_ID);
    this->storageMonitorPointer = new ThreadPointer{
        STORAGE_MONITOR_ID, std::mutex(), std::condition_variable(), std::atomic_bool(false), std::atomic_bool(false),
    };
    this->memoryStoragePipe->addReadThread(this->storageMonitorPointer);
}
void MultiThreadController::makeQuerier(){
    this->querier = new Querier(this->storageMetas,this->pcapHeader);
}

void MultiThreadController::memoryMonitorThreadRun(){
    this->memoryMonitorThread = new std::thread(&MemoryMonitor::run,this->memoryMonitor);
}
void MultiThreadController::storageMonitorThreadRun(){
    this->storageMonitorThread = new std::thread(&StorageMonitor::run,this->storageMonitor);
}
void MultiThreadController::queryThreadRun(){
    this->makeQuerier();
    this->queryThread = new std::thread(&Querier::run,this->querier);
}
void MultiThreadController::threadsRun(){
    this->memoryMonitorThreadRun();
    this->storageMonitorThreadRun();
}
void MultiThreadController::threadsStop(){
    std::cout << "Controller log: threads stop." << std::endl;
    if(this->memoryMonitorThread != nullptr){
        this->memoryMonitor->asynchronousStop();
        this->memoryMonitorThread->join();
        std::cout << "Controller log: memoryMonitorThread stop." << std::endl;
        this->memoryStoragePipe->ereaseWriteThread(this->memoryMonitorPointer);
        delete this->memoryMonitorThread;
        this->memoryMonitorThread = nullptr;
    }
    if(this->storageMonitorThread != nullptr){
        std::cout << "Controller log: storageMonitorThread wait." << std::endl;
        this->storageMonitorThread->join();
        delete this->storageMonitorThread;
        this->storageMonitorThread = nullptr;
    }
    if(this->queryThread != nullptr){
        delete this->queryThread;
    }
}
void MultiThreadController::threadsClear(){
    if(this->memoryMonitorPointer != nullptr){
        delete this->memoryMonitorPointer;
        this->memoryMonitorPointer = nullptr;
    }

    if(this->memoryMonitor != nullptr){
        delete this->memoryMonitor;
        this->memoryMonitor = nullptr;
    }

    if(this->storageMonitorPointer != nullptr){
        this->memoryStoragePipe->ereaseReadThread(this->storageMonitorPointer);
        delete this->storageMonitorPointer;
        this->storageMonitorPointer = nullptr;
    }

    if(this->storageMonitor != nullptr){
        delete this->storageMonitor;
        this->storageMonitor = nullptr;
    }

    if(this->querier != nullptr){
        delete this->querier;
    }
}

void MultiThreadController::init(InitData init_data){
    this->makeMemoryStoragePipe();
    this->makeStorageMetas();
    this->makeMemoryMonitor();
    this->makeStorageMonitor();
    
    this->memoryMonitor->init(init_data);
    this->storageMonitor->init(init_data);
    this->pcapHeader = init_data.pcap_header;
    std::cout << "Controller log: init." << std::endl;
}
void MultiThreadController::run(){
    if(this->memoryMonitorPointer == nullptr){
        std::cerr << "Controller error: run without init." << std::endl;
        return;
    }
    
    std::cout << "Controller log: run." << std::endl;
    this->threadsRun();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    this->threadsStop();

    //for test
    this->queryThreadRun();
    this->queryThread->join();

    this->threadsClear();
    std::cout << "Controller log: run quit." << std::endl;
}