#ifndef INDEX_BUFFER_HPP_
#define INDEX_BUFFER_HPP_

#include <cmath>
#include "skipList.hpp"

struct SkipListMeta{
    u_int32_t maxLvl;
    u_int32_t keyLen;
    u_int32_t valueLen;
};

class IndexCache{
    std::vector<SkipList*> skipLists;
    std::atomic_uint64_t nodeNum;
public:
    IndexCache(std::vector<SkipListMeta> metas){
        this->skipLists = std::vector<SkipList*>();
        for (auto m:metas){
            SkipList* sk = new SkipList(m.maxLvl,m.keyLen,m.valueLen);
            this->skipLists.push_back(sk);
        }
        this->nodeNum = 0;
    }
    ~IndexCache(){
        for(u_int32_t i = 0;i<this->skipLists.size();++i){
            delete skipLists[i];
        }
        this->skipLists.clear();
    }
    std::vector<SkipListMeta> getMetas(){
        std::vector<SkipListMeta> metas = std::vector<SkipListMeta>();
        for(auto sk:this->skipLists){
            SkipListMeta m = {
                .keyLen = sk->getKeyLen(),
                .valueLen = sk->getValueLen(),
                .maxLvl = sk->getMaxLevel(),
            };
            metas.push_back(m);
        }
        return metas;
    }
    bool insert(std::vector<std::string> keys, u_int64_t value, u_int64_t maxNode, u_int32_t start){
        u_int64_t now_node_num = this->nodeNum++;
        if(now_node_num > maxNode){
            this->nodeNum--;
            return false;
        }
        for (u_int32_t i = start; i< start + this->skipLists.size(); ++i){
            u_int32_t tmp = i % this->skipLists.size();
            this->skipLists[tmp]->insert(keys[tmp],value);
        }
        return true;
    }
    std::vector<SkipList*> getCache(){
        return this->skipLists;
    }
    u_int64_t getNodeNum()const{
        return this->nodeNum;
    }
};

class IndexBuffer{
    const u_int32_t cacheCount;
    // const u_int32_t maxLvl;
    // const u_int32_t keyLen;
    // const u_int32_t valueLen;
    const u_int64_t maxNode;

    // SkipList** caches;
    IndexCache** caches;
    bool* cacheFlag; // true for read
    u_int64_t* start_times;

public:
    IndexBuffer(u_int32_t _cacheCount, std::vector<SkipListMeta> metas, u_int32_t _maxNode):
    cacheCount(_cacheCount),maxNode(_maxNode){
        this->caches = new IndexCache*[_cacheCount];
        for(u_int32_t i=0; i<_cacheCount; ++i){
            this->caches[i] = new IndexCache(metas);
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
    bool insert(std::vector<std::string> keys, u_int64_t value, u_int32_t id, u_int64_t ts, u_int32_t start){
        if(this->cacheFlag[id]){
            return false;
        }
        if(this->caches[id]->insert(keys,value,this->maxNode, start)){
            this->start_times[id] = std::min(this->start_times[id],ts);
            return true;
        }
        printf("Index Buffer log: %u is full.\n",id);
        this->cacheFlag[id] = true;
        return false;
    }
    std::pair<IndexCache*,u_int64_t> getCache(u_int32_t id){
        if(!this->cacheFlag[id]){
            return {nullptr,0};
        }
        IndexCache* ret = this->caches[id];
        u_int64_t ts = this->start_times[id];
        this->caches[id] = new IndexCache(ret->getMetas());
        this->cacheFlag[id] = false;
        return {ret,ts};
    }
    std::pair<IndexCache*,u_int64_t> directGetCache(u_int32_t id){
        IndexCache* ret = this->caches[id];
        u_int64_t ts = this->start_times[id];
        this->caches[id] = new IndexCache(ret->getMetas());
        this->cacheFlag[id] = false;
        return {ret,ts};
    }
    u_int32_t getCacheCount()const{
        return this->cacheCount;
    }
};

#endif