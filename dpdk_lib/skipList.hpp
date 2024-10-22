#ifndef SKIPLIST_HPP_
#define SKIPLIST_HPP_
#include <vector>
#include <list>
#include <iostream>
#include <random>
#include <ctime>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <cstring>
#include "zOrderTree.hpp"
#include "bloomFilter.hpp"

#define PRE_TYPE u_int32_t
#define FILTER_K_LEN 3
#define OFFSET_BIT 0x80000000
#define OFFSET_MASK 0x7fffffff

#pragma pack(1)
struct PreIndex{
    u_int8_t pre;
    u_int32_t offset;
};
#pragma pack()

#pragma pack(1)
struct PreIndexInt{
    PRE_TYPE pre;
    u_int32_t offset;
};
#pragma pack()

template <class KeyType, class ValueType>
class SkipListNode{
public:
    KeyType key;
    ValueType value;
    std::vector<SkipListNode*> next;
    std::mutex mutex;

    SkipListNode(KeyType key, ValueType val, int level) : next(level, nullptr), mutex(std::mutex()) {
        this->key = key;
        this->value = val;
    }
    ~SkipListNode(){
        this->next.clear();
    }
};

// template <class KeyType, class ValueType>
class SkipList{
    const u_int32_t keyLen; // length of key
    const u_int32_t valueLen; // length of value
    const u_int32_t maxLevel; // 跳表的最大层数

    std::atomic_uint32_t level; // 当前跳表的层数
    void* head;
    // SkipListNode<KeyType,ValueType>* head; // 头节点
    std::atomic_uint64_t nodeNum;

    std::atomic_uint32_t writeThreadCount;
    std::atomic_uint32_t readThreadCount;

    u_int32_t randomLevel(){
        u_int32_t lvl = 1;
        while (rand() % 2 == 0 && lvl < maxLevel){
            lvl++;
        }
        return lvl;
    }
    void* newNode(std::string key, u_int64_t value, u_int32_t level){
        void* pointer = nullptr;
        if(this->keyLen == 1){
            u_int8_t* real_key= (u_int8_t*)&(key[0]);
            SkipListNode<u_int8_t,u_int64_t>* p = new SkipListNode<u_int8_t,u_int64_t>(*real_key,value,level);
            pointer = (void*)p;
        }else if(this->keyLen == 2){
            u_int16_t* real_key= (u_int16_t*)&(key[0]);
            SkipListNode<u_int16_t,u_int64_t>* p = new SkipListNode<u_int16_t,u_int64_t>(*real_key,value,level);
            pointer = (void*)p;
        }else if(this->keyLen == 4){
            u_int32_t* real_key= (u_int32_t*)&(key[0]);
            SkipListNode<u_int32_t,u_int64_t>* p = new SkipListNode<u_int32_t,u_int64_t>(*real_key,value,level);
            pointer = (void*)p;
        }else if(this->keyLen == 8){
            u_int64_t* real_key= (u_int64_t*)&(key[0]);
            SkipListNode<u_int64_t,u_int64_t>* p = new SkipListNode<u_int64_t,u_int64_t>(*real_key,value,level);
            pointer = (void*)p;
        }else if(this->keyLen == 12){
            ZOrderIPv4* real_key= (ZOrderIPv4*)&(key[0]);
            SkipListNode<ZOrderIPv4,u_int64_t>* p = new SkipListNode<ZOrderIPv4,u_int64_t>(*real_key,value,level);
            pointer = (void*)p;
        }else if(this->keyLen == 16){
            IPv6Address* real_key= (IPv6Address*)&(key[0]);
            SkipListNode<IPv6Address,u_int64_t>* p = new SkipListNode<IPv6Address,u_int64_t>(*real_key,value,level);
            pointer = (void*)p;
        }else if(this->keyLen == 40){
            QuarTurpleIPv6* real_key= (QuarTurpleIPv6*)&(key[0]);
            SkipListNode<QuarTurpleIPv6,u_int64_t>* p = new SkipListNode<QuarTurpleIPv6,u_int64_t>(*real_key,value,level);
            pointer = (void*)p;
        }else{
            std::cerr << "Skip list error: newNode with undifined ele_len!" << std::endl;
        }
        return pointer;
    }
    void deleteNode(void* node){
        if(this->keyLen == 1){
            SkipListNode<u_int8_t,u_int64_t>* p = (SkipListNode<u_int8_t,u_int64_t>*)node;
            delete p;
        }else if(this->keyLen == 2){
            SkipListNode<u_int16_t,u_int64_t>* p = (SkipListNode<u_int16_t,u_int64_t>*)node;
            delete p;
        }else if(this->keyLen == 4){
            SkipListNode<u_int32_t,u_int64_t>* p = (SkipListNode<u_int32_t,u_int64_t>*)node;
            delete p;
        }else if(this->keyLen == 8){
            SkipListNode<u_int64_t,u_int64_t>* p = (SkipListNode<u_int64_t,u_int64_t>*)node;
            delete p;
        }else if(this->keyLen == 12){
            SkipListNode<ZOrderIPv4,u_int64_t>* p = (SkipListNode<ZOrderIPv4,u_int64_t>*)node;
            delete p;
        }else if(this->keyLen == 16){
            SkipListNode<IPv6Address,u_int64_t>* p = (SkipListNode<IPv6Address,u_int64_t>*)node;
            delete p;
        }else if(this->keyLen == 40){
            SkipListNode<QuarTurpleIPv6,u_int64_t>* p = (SkipListNode<QuarTurpleIPv6,u_int64_t>*)node;
            delete p;
        }else{
            std::cerr << "Skip list error: deleteNode with undifined ele_len!" << std::endl;
        }
    }
    void* getNext(void* node, u_int32_t level){
        void* pointer = nullptr;
        if(this->keyLen == 1){
            SkipListNode<u_int8_t,u_int64_t>* p = (SkipListNode<u_int8_t,u_int64_t>*)node;
            pointer = (void*)(p->next[level]);
        }else if(this->keyLen == 2){
            SkipListNode<u_int16_t,u_int64_t>* p = (SkipListNode<u_int16_t,u_int64_t>*)node;
            pointer = (void*)(p->next[level]);
        }else if(this->keyLen == 4){
            SkipListNode<u_int32_t,u_int64_t>* p = (SkipListNode<u_int32_t,u_int64_t>*)node;
            pointer = (void*)(p->next[level]);
        }else if(this->keyLen == 8){
            SkipListNode<u_int64_t,u_int64_t>* p = (SkipListNode<u_int64_t,u_int64_t>*)node;
            pointer = (void*)(p->next[level]);
        }else if(this->keyLen == 12){
            SkipListNode<ZOrderIPv4,u_int64_t>* p = (SkipListNode<ZOrderIPv4,u_int64_t>*)node;
            pointer = (void*)(p->next[level]);
        }else if(this->keyLen == 16){
            SkipListNode<IPv6Address,u_int64_t>* p = (SkipListNode<IPv6Address,u_int64_t>*)node;
            pointer = (void*)(p->next[level]);
        }else if(this->keyLen == 40){
            SkipListNode<QuarTurpleIPv6,u_int64_t>* p = (SkipListNode<QuarTurpleIPv6,u_int64_t>*)node;
            pointer = (void*)(p->next[level]);
        }else{
            std::cerr << "Skip list error: getNext with undifined ele_len!" << std::endl;
        }
        return pointer;
    }
    void putNext(void* node, u_int32_t level, void* next){
        if(this->keyLen == 1){
            SkipListNode<u_int8_t,u_int64_t>* p = (SkipListNode<u_int8_t,u_int64_t>*)node;
            p->next[level] = (SkipListNode<u_int8_t,u_int64_t>*)next;
        }else if(this->keyLen == 2){
            SkipListNode<u_int16_t,u_int64_t>* p = (SkipListNode<u_int16_t,u_int64_t>*)node;
            p->next[level] = (SkipListNode<u_int16_t,u_int64_t>*)next;
        }else if(this->keyLen == 4){
            SkipListNode<u_int32_t,u_int64_t>* p = (SkipListNode<u_int32_t,u_int64_t>*)node;
            p->next[level] = (SkipListNode<u_int32_t,u_int64_t>*)next;
        }else if(this->keyLen == 8){
            SkipListNode<u_int64_t,u_int64_t>* p = (SkipListNode<u_int64_t,u_int64_t>*)node;
            p->next[level] = (SkipListNode<u_int64_t,u_int64_t>*)next;
        }else if(this->keyLen == 12){
            SkipListNode<ZOrderIPv4,u_int64_t>* p = (SkipListNode<ZOrderIPv4,u_int64_t>*)node;
            p->next[level] = (SkipListNode<ZOrderIPv4,u_int64_t>*)next;
        }else if(this->keyLen == 16){
            SkipListNode<IPv6Address,u_int64_t>* p = (SkipListNode<IPv6Address,u_int64_t>*)node;
            p->next[level] = (SkipListNode<IPv6Address,u_int64_t>*)next;
        }else if(this->keyLen == 40){
            SkipListNode<QuarTurpleIPv6,u_int64_t>* p = (SkipListNode<QuarTurpleIPv6,u_int64_t>*)node;
            p->next[level] = (SkipListNode<QuarTurpleIPv6,u_int64_t>*)next;
        }else{
            std::cerr << "Skip list error: putNext with undifined ele_len!" << std::endl;
        }
    }
    u_int64_t getValue(void* node){
        u_int64_t value;
        if(this->keyLen == 1){
            SkipListNode<u_int8_t,u_int64_t>* p = (SkipListNode<u_int8_t,u_int64_t>*)node;
            value = p->value;
        }else if(this->keyLen == 2){
            SkipListNode<u_int16_t,u_int64_t>* p = (SkipListNode<u_int16_t,u_int64_t>*)node;
            value = p->value;
        }else if(this->keyLen == 4){
            SkipListNode<u_int32_t,u_int64_t>* p = (SkipListNode<u_int32_t,u_int64_t>*)node;
            value = p->value;
        }else if(this->keyLen == 8){
            SkipListNode<u_int64_t,u_int64_t>* p = (SkipListNode<u_int64_t,u_int64_t>*)node;
            value = p->value;
        }else if(this->keyLen == 12){
            SkipListNode<ZOrderIPv4,u_int64_t>* p = (SkipListNode<ZOrderIPv4,u_int64_t>*)node;
            value = p->value;
        }else if(this->keyLen == 16){
            SkipListNode<IPv6Address,u_int64_t>* p = (SkipListNode<IPv6Address,u_int64_t>*)node;
            value = p->value;
        }else if(this->keyLen == 40){
            SkipListNode<QuarTurpleIPv6,u_int64_t>* p = (SkipListNode<QuarTurpleIPv6,u_int64_t>*)node;
            value = p->value;
        }else{
            std::cerr << "Skip list error: getNext with undifined ele_len!" << std::endl;
        }
        return value;
    }
    int compareNodeKey(void* node, std::string key){
        if(this->keyLen == 1){
            u_int8_t* real_key= (u_int8_t*)&(key[0]);
            SkipListNode<u_int8_t,u_int64_t>* p = (SkipListNode<u_int8_t,u_int64_t>*)node;
            if(p->key < *real_key){
                return -1;
            }
            if(p->key > *real_key){
                return 1;
            }
            return 0;
        }else if(this->keyLen == 2){
            u_int16_t* real_key= (u_int16_t*)&(key[0]);
            SkipListNode<u_int16_t,u_int64_t>* p = (SkipListNode<u_int16_t,u_int64_t>*)node;
            if(p->key < *real_key){
                return -1;
            }
            if(p->key > *real_key){
                return 1;
            }
            return 0;
        }else if(this->keyLen == 4){
            u_int32_t* real_key= (u_int32_t*)&(key[0]);
            SkipListNode<u_int32_t,u_int64_t>* p = (SkipListNode<u_int32_t,u_int64_t>*)node;
            if(p->key < *real_key){
                return -1;
            }
            if(p->key > *real_key){
                return 1;
            }
            return 0;
        }else if(this->keyLen == 8){
            u_int64_t* real_key= (u_int64_t*)&(key[0]);
            SkipListNode<u_int64_t,u_int64_t>* p = (SkipListNode<u_int64_t,u_int64_t>*)node;
            if(p->key < *real_key){
                return -1;
            }
            if(p->key > *real_key){
                return 1;
            }
            return 0;
        }else if(this->keyLen == 12){
            ZOrderIPv4* real_key= (ZOrderIPv4*)&(key[0]);
            SkipListNode<ZOrderIPv4,u_int64_t>* p = (SkipListNode<ZOrderIPv4,u_int64_t>*)node;
            if(p->key < *real_key){
                return -1;
            }
            if(p->key > *real_key){
                return 1;
            }
            return 0;
        }else if(this->keyLen == 16){
            IPv6Address* real_key= (IPv6Address*)&(key[0]);
            SkipListNode<IPv6Address,u_int64_t>* p = (SkipListNode<IPv6Address,u_int64_t>*)node;
            if(p->key < *real_key){
                return -1;
            }
            if(p->key > *real_key){
                return 1;
            }
            return 0;
        }else if(this->keyLen == 40){
            QuarTurpleIPv6* real_key= (QuarTurpleIPv6*)&(key[0]);
            SkipListNode<QuarTurpleIPv6,u_int64_t>* p = (SkipListNode<QuarTurpleIPv6,u_int64_t>*)node;
            if(p->key < *real_key){
                return -1;
            }
            if(p->key > *real_key){
                return 1;
            }
            return 0;
        }
        else{
            std::cerr << "Skip list error: getNext with undifined ele_len!" << std::endl;
        }
        return 0;
    }
    void lockNode(void* node){
        if(this->keyLen == 1){
            SkipListNode<u_int8_t,u_int64_t>* p = (SkipListNode<u_int8_t,u_int64_t>*)node;
            p->mutex.lock();
        }else if(this->keyLen == 2){
            SkipListNode<u_int16_t,u_int64_t>* p = (SkipListNode<u_int16_t,u_int64_t>*)node;
            p->mutex.lock();
        }else if(this->keyLen == 4){
            SkipListNode<u_int32_t,u_int64_t>* p = (SkipListNode<u_int32_t,u_int64_t>*)node;
            p->mutex.lock();
        }else if(this->keyLen == 8){
            SkipListNode<u_int64_t,u_int64_t>* p = (SkipListNode<u_int64_t,u_int64_t>*)node;
            p->mutex.lock();
        }else if(this->keyLen == 12){
            SkipListNode<ZOrderIPv4,u_int64_t>* p = (SkipListNode<ZOrderIPv4,u_int64_t>*)node;
            p->mutex.lock();
        }else if(this->keyLen == 16){
            SkipListNode<IPv6Address,u_int64_t>* p = (SkipListNode<IPv6Address,u_int64_t>*)node;
            p->mutex.lock();
        }else if(this->keyLen == 40){
            SkipListNode<QuarTurpleIPv6,u_int64_t>* p = (SkipListNode<QuarTurpleIPv6,u_int64_t>*)node;
            p->mutex.lock();
        }else{
            std::cerr << "Skip list error: getNext with undifined ele_len!" << std::endl;
        }
    }
    void unlockNode(void* node){
        if(this->keyLen == 1){
            SkipListNode<u_int8_t,u_int64_t>* p = (SkipListNode<u_int8_t,u_int64_t>*)node;
            p->mutex.unlock();
        }else if(this->keyLen == 2){
            SkipListNode<u_int16_t,u_int64_t>* p = (SkipListNode<u_int16_t,u_int64_t>*)node;
            p->mutex.unlock();
        }else if(this->keyLen == 4){
            SkipListNode<u_int32_t,u_int64_t>* p = (SkipListNode<u_int32_t,u_int64_t>*)node;
            p->mutex.unlock();
        }else if(this->keyLen == 8){
            SkipListNode<u_int64_t,u_int64_t>* p = (SkipListNode<u_int64_t,u_int64_t>*)node;
            p->mutex.unlock();
        }else if(this->keyLen == 12){
            SkipListNode<ZOrderIPv4,u_int64_t>* p = (SkipListNode<ZOrderIPv4,u_int64_t>*)node;
            p->mutex.unlock();
        }else if(this->keyLen == 16){
            SkipListNode<IPv6Address,u_int64_t>* p = (SkipListNode<IPv6Address,u_int64_t>*)node;
            p->mutex.unlock();
        }else if(this->keyLen == 40){
            SkipListNode<QuarTurpleIPv6,u_int64_t>* p = (SkipListNode<QuarTurpleIPv6,u_int64_t>*)node;
            p->mutex.unlock();
        }else{
            std::cerr << "Skip list error: getNext with undifined ele_len!" << std::endl;
        }
    }
    std::string getKey(void* node){
        std::string key;
        if(this->keyLen == 1){
            SkipListNode<u_int8_t,u_int64_t>* p = (SkipListNode<u_int8_t,u_int64_t>*)node;
            key = std::string((char*)&(p->key),this->keyLen);
        }else if(this->keyLen == 2){
            SkipListNode<u_int16_t,u_int64_t>* p = (SkipListNode<u_int16_t,u_int64_t>*)node;
            key = std::string((char*)&(p->key),this->keyLen);
        }else if(this->keyLen == 4){
            SkipListNode<u_int32_t,u_int64_t>* p = (SkipListNode<u_int32_t,u_int64_t>*)node;
            key = std::string((char*)&(p->key),this->keyLen);
        }else if(this->keyLen == 8){
            SkipListNode<u_int64_t,u_int64_t>* p = (SkipListNode<u_int64_t,u_int64_t>*)node;
            key = std::string((char*)&(p->key),this->keyLen);
        }else if(this->keyLen == 12){
            SkipListNode<ZOrderIPv4,u_int64_t>* p = (SkipListNode<ZOrderIPv4,u_int64_t>*)node;
            key = std::string((char*)&(p->key),this->keyLen);
        }else if(this->keyLen == 16){
            SkipListNode<IPv6Address,u_int64_t>* p = (SkipListNode<IPv6Address,u_int64_t>*)node;
            key = std::string((char*)&(p->key),this->keyLen);
        }else if(this->keyLen == 40){
            SkipListNode<QuarTurpleIPv6,u_int64_t>* p = (SkipListNode<QuarTurpleIPv6,u_int64_t>*)node;
            key = std::string((char*)&(p->key),this->keyLen);
        }else{
            std::cerr << "Skip list error: getNext with undifined ele_len!" << std::endl;
        }
        return key;
    }
public:
    SkipList(u_int32_t maxLvl, u_int32_t keyLen, u_int32_t valueLen) : maxLevel(maxLvl), level(0), nodeNum(0), keyLen(keyLen), valueLen(valueLen) {
        this->head = newNode(std::string(this->keyLen,0),0,this->maxLevel);
        this->writeThreadCount = 0;
        this->readThreadCount = 0;
        //srand(static_cast<int>(time(nullptr)));
        // printf("key_len:%u.\n",this->keyLen);
    }
    ~SkipList(){
        if(this->readThreadCount || this->writeThreadCount){
            std::cout << "Skip list warning: it is used by certain thread." << std::endl;
        }
        void* node = this->head;
        while(node != nullptr){
            void* node_tmp = this->getNext(node,0);
            this->deleteNode(node);
            node = node_tmp;
        }
    }
    void addWriteThread(){
        this->writeThreadCount++;
    }
    void ereaseWriteThread(){
        this->writeThreadCount--;
    }
    u_int32_t getWriteThreadCount()const{
        return this->writeThreadCount.load();
    }
    bool insert(std::string key, u_int64_t value, u_int64_t maxNode){
        // construct new node
        if(key.size()!=this->keyLen){
            printf("Skip list error: insert with wrong key length %lu-%u!\n",key.size(),this->keyLen);
            // std::cerr << "Skip list error: insert with wrong key length!" <<std::endl;
            return true;
        }

        // printf("put one.\n");
        
        u_int64_t now_node_num = this->nodeNum++;
        if(now_node_num > maxNode){
            this->nodeNum--;
            return false;
        }

        int newLevel = randomLevel();
        void* newNode = this->newNode(key,value,newLevel);

        // get last nodes
        void* curr = this->head;
        std::vector<void*> update(maxLevel, nullptr);
        u_int32_t nowLevel = this->level;
        for (int i = nowLevel > newLevel ? nowLevel - 1: newLevel -1; i >= 0; i--) {
            while (this->getNext(curr,i) != nullptr && this->compareNodeKey(this->getNext(curr,i),key)<0){
                curr = this->getNext(curr,i);
            }
            update[i] = curr;
        }

        // insert
        for(int i=0; i<newLevel; ++i){
            while(true){
                // std::cout << "Skip list log: level " << i << std::endl;
                this->lockNode(update[i]);
                if(this->getNext(update[i],i) != nullptr && this->compareNodeKey(this->getNext(update[i],i),key)<0){// there may be new node inserted.
                    this->unlockNode(update[i]);
                    update[i] = this->getNext(update[i],i);
                    continue;
                }else{
                    this->putNext(newNode,i,this->getNext(update[i],i));
                    this->putNext(update[i],i,newNode);
                    this->unlockNode(update[i]);
                    break;
                }
            }
        }

        // this->nodeNum++;

        u_int32_t curLevel = this->level.load();
        while (curLevel < newLevel) {// CAS update level
            if (this->level.compare_exchange_strong(curLevel, newLevel)) {
                break;
            }else{
                curLevel = this->level.load();
            }
        }
        return true;
    }
    std::list<u_int32_t> findByKey(std::string key){
        if(key.size()!=this->keyLen){
            std::cerr << "Skip list error: findByKey with wrong key length!" <<std::endl;
            return std::list<u_int32_t>();
        }
        void* curr = this->head;
        std::list<u_int32_t> res = std::list<u_int32_t>();
        void* beginNode = nullptr;
        for (int i = this->level - 1; i >= 0; i--) {
            while (this->getNext(curr,i) != nullptr && this->compareNodeKey(this->getNext(curr,i),key)<0)
                curr = this->getNext(curr,i);
        }

        beginNode = this->getNext(curr,0);
        for (auto node = beginNode; node!=nullptr; node = this->getNext(node,0)){
            if(this->compareNodeKey(node, key) < 0){ // there may be new node inserted.
                continue;
            }
            if(this->compareNodeKey(node, key) > 0){
                break;
            }
            res.push_back(this->getValue(node));
        }
        return res;
    }
    std::list<u_int32_t> findByRange(std::string begin, std::string end){
        if(begin.size()!=this->keyLen || end.size()!=this->keyLen){
            std::cerr << "Skip list error: findByRange with wrong key length!" <<std::endl;
            return std::list<u_int32_t>();
        }
        void* curr = this->head;
        std::list<u_int32_t> res = std::list<u_int32_t>();
        void* beginNode = nullptr;
        for (int i = this->level - 1; i >= 0; i--) {
            while (this->getNext(curr,i) != nullptr && this->compareNodeKey(this->getNext(curr,i),begin)<0)
                curr = this->getNext(curr,i);
        }
        
        beginNode = this->getNext(curr,0);
        for (auto node = beginNode; node!=nullptr; node = this->getNext(node,0)){
            if(this->compareNodeKey(node, begin) < 0){ // there may be new node inserted.
                continue;
            }
            if(this->compareNodeKey(node, end) >= 0){
                break;
            }
            res.push_back(this->getValue(node));
        }
        return res;
    }
    u_int64_t getNodeNum()const{
        return this->nodeNum;
    }
    u_int32_t getValueLen()const{
        return this->valueLen;
    }
    u_int32_t getKeyLen()const{
        return this->keyLen;
    }
    u_int32_t getMaxLevel()const{
        return this->maxLevel;
    }
    std::string outputToChar(){
        // if(this->writeThreadCount){
        //     std::cerr << "Skip list error: it is used by certain w-thread." << std::endl;
        //     return std::string();
        // }
        // std::cout << "Skip list log: outputToChar." <<std::endl;
        
        u_int64_t buffer_len = this->nodeNum * (this->keyLen + this->valueLen);
        std::string data = std::string(buffer_len,0);
        u_int64_t offset = 0;
        for(auto node = this->getNext(head,0); node!=nullptr; node = this->getNext(node,0)){
            std::string key = this->getKey(node);
            memcpy(&(data[offset]),&(key[0]),this->keyLen);
            offset += this->keyLen;
            u_int64_t value = this->getValue(node);
            memcpy(&(data[offset]),&value,this->valueLen);
            offset += this->valueLen;
        }
        return data;
    }
    std::string outputToCharCompact(){
        std::string data = std::string();
        std::string values = std::string(this->nodeNum * this->valueLen,0);
        u_int32_t offset = 0;
        std::string last_key = std::string();
        for(auto node = this->getNext(head,0); node!=nullptr; node = this->getNext(node,0)){
            auto key = this->getKey(node);
            if(last_key != key){
                data += key;
                data += std::string((char*)&offset,sizeof(offset));
                last_key = key;
            }
            u_int64_t value = this->getValue(node);
            memcpy(&(values[offset*this->valueLen]),&value,this->valueLen);
            offset ++;
        }
        data += values;
        return data;
    }
    std::string outputToCharCompressedInt(){
        const u_int32_t slice_len = sizeof(PRE_TYPE);
        const u_int32_t key_l = this->keyLen/slice_len;
        std::string data = std::string();
        BloomFilter filter = BloomFilter(this->nodeNum, FILTER_K_LEN);

        std::vector<std::string> layers = std::vector<std::string>(key_l,std::string());
        std::vector<u_int32_t> new_node_count = std::vector<u_int32_t>(key_l,0);

        std::string values = std::string(this->nodeNum * this->valueLen,0);
        u_int32_t value_offset = 0;

        std::string last_key = std::string();

        bool new_pre = true;

        for(auto node = this->getNext(head,0); node!=nullptr; node = this->getNext(node,0)){
            std::string key = this->getKey(node);
            if(key == last_key){
                u_int64_t value = this->getValue(node);
                memcpy(&(values[value_offset]),&value,this->valueLen);
                value_offset += this->valueLen;
                new_pre = false;
                continue;
            }
            last_key = key;
            filter.insert(key);

            // for(auto c:key){
            //     printf("%02x",(u_int8_t)c);
            // }
            // printf("\n");

            PRE_TYPE* key_int = (PRE_TYPE*)(&key[0]);
            

            for(u_int8_t i = 0; i< key_l; ++i){
                
                // u_int8_t pre = key[key.size() - i - 1];
                PRE_TYPE pre = key_int[key_l - i - 1];

                // printf("%02x\n",(u_int8_t)pre);

                if(new_pre){
                    // only one node
                    if (new_node_count[i]==1 && i != key_l-1){
                        u_int32_t off = OFFSET_BIT;
                        off += *(u_int32_t*)(&(layers[key_l-1][layers[key_l-1].size()-sizeof(off)]));
                        memcpy(&layers[i][layers[i].size()-sizeof(off)],&off,sizeof(off));
                        for(u_int8_t j=i+1;j<key_l; ++j){
                            layers[j].resize(layers[j].length()-sizeof(PreIndexInt));
                            new_node_count[j] = 0;
                        }
                    }

                    // layers[i].push_back(pre);
                    layers[i] += std::string((char*)&pre,sizeof(pre));
                    u_int32_t offset = (i == key_l - 1)? value_offset/this->valueLen : layers[i+1].size()/sizeof(PreIndexInt);
                    layers[i] += std::string((char*)&offset,sizeof(u_int32_t));
                    // printf("layer: %u, offset: %u\n",i,offset);
                    new_node_count[i] = 1;
                    new_pre = true;
                }else{
                    // u_int8_t last_pre = layers[i][layers[i].size()-sizeof(PreIndexInt)];
                    PRE_TYPE last_pre = *(PRE_TYPE*)&(layers[i][layers[i].size()-sizeof(PreIndexInt)]);
                    if (last_pre == pre){
                        new_node_count[i]++;
                        continue;
                    }

                    // only one node
                    if (new_node_count[i]==1 && i != key_l-1){
                        u_int32_t off = OFFSET_BIT;
                        off += *(u_int32_t*)(&(layers[key_l-1][layers[key_l-1].size()-sizeof(off)]));
                        memcpy(&layers[i][layers[i].size()-sizeof(off)],&off,sizeof(off));
                        for(u_int8_t j=i+1;j<key_l; ++j){
                            layers[j].resize(layers[j].length()-sizeof(PreIndexInt));
                            new_node_count[j] = 0;
                        }
                    }

                    // layers[i].push_back(pre);
                    layers[i] += std::string((char*)&pre,sizeof(pre));
                    u_int32_t offset = (i == key_l - 1)? value_offset/this->valueLen : layers[i+1].size()/sizeof(PreIndexInt);
                    layers[i] += std::string((char*)&offset,sizeof(u_int32_t));
                    // printf("layer: %u, offset: %u\n",i,offset);
                    new_node_count[i] = 1;
                    new_pre = true;
                }
            }
            u_int64_t value = this->getValue(node);
            memcpy(&(values[value_offset]),&value,this->valueLen);
            value_offset += this->valueLen;
            new_pre = false;
        }

        for(u_int8_t i = 0; i< key_l; ++i){
            if (new_node_count[i]==1 && i != key_l - 1){
                u_int32_t off = OFFSET_BIT;
                off += *(u_int32_t*)(&(layers[key_l - 1][layers[key_l-1].size()-sizeof(off)]));
                memcpy(&layers[i][layers[i].size()-sizeof(off)],&off,sizeof(off));
                for(u_int8_t j=i+1;j<key_l; ++j){
                    layers[j].resize(layers[j].length()-sizeof(PreIndexInt));
                }
                break;
            }
        }

        data += std::string((char*)&this->nodeNum,sizeof(u_int32_t));
        data += filter.bitArray;
        for(auto layer:layers){
            u_int32_t len = layer.size();
            // printf("len: %u\n",len);
            data += std::string((char*)&len,sizeof(u_int32_t));
        }
        for(auto layer:layers){
            data+=layer;
        }
        data+=values;

        return data;

    }
    std::string outputToCharCompressed(){
        std::string data = std::string();
        BloomFilter filter = BloomFilter(this->nodeNum, 3);

        std::vector<std::string> layers = std::vector<std::string>(this->keyLen,std::string());
        std::vector<u_int32_t> new_node_count = std::vector<u_int32_t>(this->keyLen,0);

        std::string values = std::string(this->nodeNum * this->valueLen,0);
        u_int32_t value_offset = 0;

        std::string last_key = std::string();

        bool new_pre = true;

        for(auto node = this->getNext(head,0); node!=nullptr; node = this->getNext(node,0)){
            std::string key = this->getKey(node);
            if(key == last_key){
                u_int64_t value = this->getValue(node);
                memcpy(&(values[value_offset]),&value,this->valueLen);
                value_offset += this->valueLen;
                new_pre = false;
                continue;
            }
            last_key = key;
            filter.insert(key);

            // for(auto c:key){
            //     printf("%02x",(u_int8_t)c);
            // }
            // printf("\n");

            for(u_int8_t i = 0; i< key.size(); ++i){
                
                u_int8_t pre = key[key.size() - i - 1];

                // printf("%02x\n",(u_int8_t)pre);

                if(new_pre){
                    // only one node
                    if (new_node_count[i]==1 && i != key.size()-1){
                        u_int32_t off = OFFSET_BIT;
                        off += *(u_int32_t*)(&(layers[key.size()-1][layers[key.size()-1].size()-sizeof(off)]));
                        memcpy(&layers[i][layers[i].size()-sizeof(off)],&off,sizeof(off));
                        for(u_int8_t j=i+1;j<key.size(); ++j){
                            layers[j].resize(layers[j].length()-sizeof(PreIndex));
                            new_node_count[j] = 0;
                        }
                    }

                    layers[i].push_back(pre);
                    u_int32_t offset = (i == key.size() - 1)? value_offset/this->valueLen : layers[i+1].size()/sizeof(PreIndex);
                    layers[i] += std::string((char*)&offset,sizeof(u_int32_t));
                    // printf("layer: %u, offset: %u\n",i,offset);
                    new_node_count[i] = 1;
                    new_pre = true;
                }else{
                    u_int8_t last_pre = layers[i][layers[i].size()-sizeof(PreIndex)];
                    if (last_pre == pre){
                        new_node_count[i]++;
                        continue;
                    }

                    // only one node
                    if (new_node_count[i]==1 && i != key.size()-1){
                        u_int32_t off = OFFSET_BIT;
                        off += *(u_int32_t*)(&(layers[key.size()-1][layers[key.size()-1].size()-sizeof(off)]));
                        memcpy(&layers[i][layers[i].size()-sizeof(off)],&off,sizeof(off));
                        for(u_int8_t j=i+1;j<key.size(); ++j){
                            layers[j].resize(layers[j].length()-sizeof(PreIndex));
                            new_node_count[j] = 0;
                        }
                    }

                    layers[i].push_back(pre);
                    u_int32_t offset = (i == key.size() - 1)? value_offset/this->valueLen : layers[i+1].size()/sizeof(PreIndex);
                    layers[i] += std::string((char*)&offset,sizeof(u_int32_t));
                    // printf("layer: %u, offset: %u\n",i,offset);
                    new_node_count[i] = 1;
                    new_pre = true;
                }
            }
            u_int64_t value = this->getValue(node);
            memcpy(&(values[value_offset]),&value,this->valueLen);
            value_offset += this->valueLen;
            new_pre = false;
        }

        for(u_int8_t i = 0; i< this->keyLen; ++i){
            if (new_node_count[i]==1 && i != this->keyLen - 1){
                u_int32_t off = OFFSET_BIT;
                off += *(u_int32_t*)(&(layers[this->keyLen - 1][layers[this->keyLen-1].size()-sizeof(off)]));
                memcpy(&layers[i][layers[i].size()-sizeof(off)],&off,sizeof(off));
                for(u_int8_t j=i+1;j<this->keyLen; ++j){
                    layers[j].resize(layers[j].length()-sizeof(PreIndex));
                }
                break;
            }
        }

        data += std::string((char*)&this->nodeNum,sizeof(u_int32_t));
        data += filter.bitArray;
        for(auto layer:layers){
            u_int32_t len = layer.size();
            // printf("len: %u\n",len);
            data += std::string((char*)&len,sizeof(u_int32_t));
        }
        for(auto layer:layers){
            data+=layer;
        }
        data+=values;

        return data;
    }
    ZOrderIPv4Meta* outputToZOrderIPv4(){
        // if(this->writeThreadCount){
        //     std::cerr << "Skip list error: it is used by certain w-thread." << std::endl;
        //     return nullptr;
        // }
        u_int64_t buffer_len = this->nodeNum;
        ZOrderIPv4Meta* data = new ZOrderIPv4Meta[buffer_len];

        u_int64_t offset = 0;
        for(auto node = this->getNext(head,0); node!=nullptr; node = this->getNext(node,0)){
            data[offset].zorder = *(ZOrderIPv4*)(this->getKey(node).c_str());
            data[offset].value = this->getValue(node);
            offset ++;
        }
        return data;
    }
};

#endif