#include "../component/dpdkReader.hpp"
#include <vector>
#include <iostream>
#include <thread>

const u_int16_t nb_rx = 2;
const u_int32_t buffer_warn = 4*1024*1024;
const u_int32_t packet_warn = 1e5;
const u_int32_t pcap_header_len = 24;
const u_int32_t eth_header_len = 14;
const u_int64_t capacity = 1024*1024*1024;

int main(){
    DPDK* dpdk = new DPDK(nb_rx,1);
    std::vector<DPDKReader*> readers = std::vector<DPDKReader*>();
    // std::vector<std::thread*> threads = std::vector<std::thread*>();
    for(u_int16_t i = 0;i<nb_rx;++i){
        DPDKReader* reader = new DPDKReader(pcap_header_len,eth_header_len,dpdk,nullptr,nullptr,0,i,capacity);
        readers.push_back(reader);
    }
    // for(u_int16_t i = 0;i<nb_rx;++i){
    //     std::thread* t = new std::thread(&DPDKReader::run,readers[i]);
    //     threads.push_back(t);
    // }
    printf("Detected logical cores: %u\n", rte_lcore_count());
    unsigned lcore_id;
    u_int16_t queue_id = 0;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        // uint16_t queue_id = rte_lcore_id() - 1;  // queue_id starts from 0
        printf("Worker core ID: %u\n", queue_id);
        int ret = rte_eal_remote_launch(&DPDKReader::launch, readers[queue_id], lcore_id);
        queue_id ++;
        if (ret < 0){
            rte_exit(EXIT_FAILURE, "Error launching lcore %u\n", lcore_id);
        }
        // printf("lcore_id:%u.\n",lcore_id);
    }


    printf("wait.\n");
    std::this_thread::sleep_for(std::chrono::seconds(30));

    for(u_int16_t i = 0;i<nb_rx;++i){
        readers[i]->asynchronousStop();
        // threads[i]->join();
    }

    printf("stop.\n");

    // std::this_thread::sleep_for(std::chrono::seconds(5));
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        rte_eal_wait_lcore(lcore_id);
        printf("lcore %u stop.\n",lcore_id);
    }

    // printf("test.\n");
    for(u_int16_t i = 0;i<nb_rx;++i){
        // delete threads[i];
        delete readers[i];
        // threads.clear();
        readers.clear();
    }
    delete dpdk;
    printf("done.\n");
    return 0;
}