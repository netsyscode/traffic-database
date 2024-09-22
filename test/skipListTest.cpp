#include "../dpdk_lib/skipList.hpp"
#include "../dpdk_lib/indexBuffer.hpp"
#include "../dpdk_lib/pointerRingBuffer.hpp"
#include <iostream>
#include <random>

#include <iostream>
#include <random>
#include <thread>

#define TEST_INDEX_NUM 1024*1024*4
#define THREAD_NUM 8
#define CPU_BEGIN 4

// SkipList* list;
IndexBuffer* buffer;
IndexBuffer* buffer2;
PointerRingBuffer* ring;
PointerRingBuffer* ring2;

 
void thread_run(u_int32_t cpu){
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
        printf("thread %lu bind to cpu %d.\n",thread,cpu);
    } else {
        printf("thread %lu failed to bind to cpu %d!\n",thread,cpu);
    }

    u_int64_t duration_time = 0;

    while (true){
        void* data = ring->get();
        if(data==nullptr){
            break;
        }
        auto start = std::chrono::high_resolution_clock::now();
        // list->insert(std::string((char*)(data),sizeof(u_int32_t)),0,std::numeric_limits<uint64_t>::max());
        auto key = std::string((char*)(data),sizeof(u_int32_t));
        buffer->insert(key,0,0,0,0);
        auto end = std::chrono::high_resolution_clock::now();
        delete (u_int32_t*)data;
        duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    printf("Thread %lu: thread quit, during %lu us.\n",thread,duration_time);
}

void thread_run2(){

    u_int64_t duration_time = 0;

    while (true){
        void* data = ring2->get();
        if(data==nullptr){
            break;
        }
        auto start = std::chrono::high_resolution_clock::now();
        // list->insert(std::string((char*)(data),sizeof(u_int32_t)),0,std::numeric_limits<uint64_t>::max());
        auto key = std::string((char*)(data),sizeof(u_int32_t));
        buffer2->insert(key,0,0,0,0);
        auto end = std::chrono::high_resolution_clock::now();
        delete (u_int32_t*)data;
        duration_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    printf("DPDK Reader log: thread quit, during %lu us.\n",duration_time);
}

int main(){

    SkipListMeta meta = {
        .keyLen = sizeof(u_int32_t),
        .valueLen = sizeof(u_int64_t),
        .maxLvl = sizeof(u_int32_t)*8,
    };

    buffer = new IndexBuffer(5,meta,std::numeric_limits<uint64_t>::max());
    // buffer2 = new IndexBuffer(5,meta,std::numeric_limits<uint64_t>::max());
    // list = new SkipList(sizeof(u_int32_t)*8,sizeof(u_int32_t),sizeof(u_int64_t));
    ring = new PointerRingBuffer(1024*1024*1024);
    // ring2 = new PointerRingBuffer(1024*1024*1024);

    std::vector<std::thread*> threads = std::vector<std::thread*>();
   
    std::random_device rd;  // 用于获得种子
    std::mt19937 gen(rd()); // 使用随机设备种子初始化Mersenne Twister生成器
    std::uniform_int_distribution<> distrib(0, 1000000); // 定义分布范围[10, 100]
 
    // 生成随机数
    for(u_int64_t i=0;i<TEST_INDEX_NUM;++i){
        u_int32_t* random_integer = new u_int32_t(distrib(gen));
        ring->put(random_integer);
    }
    // for(u_int64_t i=0;i<TEST_INDEX_NUM;++i){
    //     u_int32_t* random_integer = new u_int32_t(distrib(gen));
    //     ring2->put(random_integer);
    // }

    printf("done.\n");

    for(u_int32_t i=0;i<THREAD_NUM;++i){
        std::thread* t = new std::thread(thread_run,i*2 + CPU_BEGIN - 1);
        threads.push_back(t);
    }
    // for(u_int32_t i=0;i<THREAD_NUM;++i){
    //     std::thread* t = new std::thread(thread_run2);
    //     threads.push_back(t);
    // }


    ring->asynchronousStop();
    // ring2->asynchronousStop();

    for(auto t:threads){
        t->join();
    }
    
    return 0;
}