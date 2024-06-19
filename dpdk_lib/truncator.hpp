#ifndef TRUNCATOR_HPP_
#define TRUNCATOR_HPP_
#include <iostream>
#include <atomic>
#include "../dpdk_lib/skipList.hpp"
#include "../dpdk_lib/pointerRingBuffer.hpp"

struct TruncateUnit{
    SkipList* index_map;
    u_int64_t ts_first;
    u_int64_t ts_last;
    enum IndexType index_type;
};

class Truncator{
private:
    const u_int64_t truncate_interval;
    const u_int32_t max_lvl;
    const u_int32_t key_len;
    const u_int32_t value_len;
    const IndexType index_type;
    std::atomic_uint64_t ts_first;
    std::atomic_uint64_t ts_last;

    PointerRingBuffer* ring;

    std::atomic_bool pause;

public:
    SkipList* indexMap;
    SkipList* newIndexMap;
    Truncator(const u_int64_t truncate_interval, u_int32_t max_lvl, u_int32_t key_len, u_int32_t value_len, PointerRingBuffer* ring,IndexType index_type):
        truncate_interval(truncate_interval),max_lvl(max_lvl),key_len(key_len),value_len(value_len),index_type(index_type){
        this->indexMap = new SkipList(this->max_lvl,this->key_len,this->value_len);
        printf("interval:%llu.\n",this->truncate_interval);
        printf("keylen:%u.\n",this->key_len);
        this->newIndexMap = nullptr;
        this->ts_first = 0;
        this->ts_last = 0;
        this->pause = false;
        this->ring = ring;
    }
    ~Truncator(){
        if(this->indexMap!=nullptr){
            delete this->indexMap;
        }
        if(this->newIndexMap!=nullptr){
            delete this->newIndexMap;
        }
    }
    bool getPause()const{
        return this->pause;
    }
    void updateTS(u_int64_t ts){
        while (true) {
            u_int64_t current_first = this->ts_first.load();
            if (current_first != 0 && ts >= current_first){
                break;
            }
            if (this->ts_first.compare_exchange_strong(current_first, ts)) {
                break;
            }
        }
        while (true) {
            u_int64_t current_last = this->ts_last.load();
            if (ts <= current_last){
                break;
            }
            if (this->ts_last.compare_exchange_strong(current_last, ts)) {
                break;
            }
        }
    }
    void check(){
        if(this->pause){
            return;
        }
        u_int64_t current_last = this->ts_last.load();
        u_int64_t current_first = this->ts_first.load();
        if(current_last-current_first > this->truncate_interval && current_first!=0 && current_last !=0){
            this->newIndexMap = new SkipList(this->max_lvl,this->key_len,this->value_len);
            this->pause = true;
            printf("Truncator log: pause.\n");
        }
    }
    void update(){
        if(!this->pause){
            return;
        }
        if(this->indexMap->getWriteThreadCount()>0){
            return;
        }
        TruncateUnit* unit = new TruncateUnit;
        unit->index_map = this->indexMap;
        unit->ts_first = ts_first.load();
        unit->ts_last = ts_last.load();
        unit->index_type = this->index_type;
        this->ring->put((void*)unit);
        
        this->indexMap = this->newIndexMap;
        this->newIndexMap = nullptr;
        this->ts_first = 0;
        this->ts_last = 0;
        this->pause = false;
    }
};


#endif