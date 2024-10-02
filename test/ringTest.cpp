#include "../dpdk_lib/pointerRingBuffer.hpp"
// #include "../dpdk_lib/lockRing.hpp"
#include <thread>
#define NODE_NUM 1024*1024*16
#define THREAD_NUM 8

void writer(PointerRingBuffer* ring, u_int32_t cpu){
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


    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NODE_NUM/THREAD_NUM; ++i) {
        int* data = new int(i);
        ring->put((void*)data);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    printf("write time: %lu\n",duration_time);
}

void reader(PointerRingBuffer* ring, u_int32_t cpu){
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

    int count = 0;
    bool has_start = false;
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    while(true){
        int* data = (int*)ring->get();
        if(data == nullptr){
            break;
        }
        if(!has_start){
            has_start = true;
            start = std::chrono::high_resolution_clock::now();
        }
        count++;
        delete data;
        end = std::chrono::high_resolution_clock::now();
    }
    auto duration_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    printf("read time: %lu, with %d datas.\n",duration_time,count);
}

int main(){
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset);

    pthread_t thread = pthread_self();

    int set_result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (set_result != 0) {
        std::cerr << "Error setting thread affinity: " << set_result << std::endl;
    }

    // 确认设置是否成功
    CPU_ZERO(&cpuset);
    pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

    if (CPU_ISSET(1, &cpuset)) {
        printf("thread %lu bind to cpu %d.\n",thread,1);
    } else {
        printf("thread %lu failed to bind to cpu %d!\n",thread,1);
    }

    PointerRingBuffer* ring = new PointerRingBuffer(1024*1024);
    // CircularBuffer* ring = new CircularBuffer(1024*1024);
    std::vector<std::thread*> writers;
    std::vector<std::thread*> readers;

    for(int i=0;i<THREAD_NUM;++i){
        std::thread* t = new std::thread(reader,ring,i*2+3);
        readers.push_back(t);
    }

    for(int i=0;i<THREAD_NUM;++i){
        std::thread* t = new std::thread(writer,ring,(i+THREAD_NUM)*2+3);
        writers.push_back(t);
    }

    for(auto t: writers){
        t->join();
        delete t;
    }

    ring->asynchronousStop();

    for(auto t: readers){
        t->join();
        delete t;
    }

}