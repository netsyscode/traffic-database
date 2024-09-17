#ifndef INDEX_BUFFER_HPP_
#define INDEX_BUFFER_HPP_

#include <cmath>
#include "skipList.hpp"

class IndexBuffer{
    const u_int32_t cacheCount;
    const u_int32_t maxLvl;
    const u_int32_t keyLen;
    const u_int32_t valueLen;
    const u_int64_t maxNode;

    SkipList** caches;
    bool* cacheFlag; // true for read
    u_int64_t* start_times;

public:
    IndexBuffer(u_int32_t _cacheCount, u_int32_t _maxLvl, u_int32_t _keyLen, u_int32_t _valueLen, u_int32_t _maxNode):
    cacheCount(_cacheCount),maxLvl(_maxLvl),keyLen(_keyLen),valueLen(_valueLen),maxNode(_maxNode){
        this->caches = new SkipList*[_cacheCount];
        for(u_int32_t i=0; i<_cacheCount; ++i){
            this->caches[i] = new SkipList(_maxLvl,_keyLen,_valueLen);
        }
        this->cacheFlag = new bool[_cacheCount]();
        this->start_times = new u_int64_t[_cacheCount];
        for(u_int32_t i=0; i<_cacheCount; ++i){
            this->start_times[i] = std::numeric_limits<uint64_t>::max();
        }
    }
    ~IndexBuffer(){
        for(u_int32_t i=0; i<this->cacheCount; ++i){
            delete this->caches[i];
        }
        delete[] this->start_times;
        delete[] this->cacheFlag;
        delete[] this->caches;
    }
    bool insert(std::string key, u_int64_t value, u_int32_t id, u_int64_t ts){
        if(this->cacheFlag[id]){
            return false;
        }
        if(this->caches[id]->insert(key,value,this->maxNode)){
            this->start_times[id] = std::min(this->start_times[id],ts);
            return true;
        }
        this->cacheFlag[id] = true;
        return false;
    }
    std::pair<SkipList*,u_int64_t> getCache(u_int32_t id){
        if(!this->cacheFlag[id]){
            return {nullptr,0};
        }
        SkipList* ret = this->caches[id];
        u_int64_t ts = this->start_times[id];
        this->caches[id] = new SkipList(this->maxLvl,this->keyLen,this->valueLen);
        this->cacheFlag[id] = false;
        return {ret,ts};
    }
    std::pair<SkipList*,u_int64_t> directGetCache(u_int32_t id){
        SkipList* ret = this->caches[id];
        u_int64_t ts = this->start_times[id];
        this->caches[id] = new SkipList(this->maxLvl,this->keyLen,this->valueLen);
        this->cacheFlag[id] = false;
        return {ret,ts};
    }
    u_int32_t getCacheCount()const{
        return this->cacheCount;
    }
};

#endif