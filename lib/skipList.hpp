#ifndef SKIPLIST_HPP_
#define SKIPLIST_HPP_
#include <vector>
#include <list>
#include <iostream>
#include <random>
#include <ctime>
#include <atomic>
#include <mutex>

template <class KeyType, class ValueType>
class SkipListNode{
public:
    KeyType key;
    ValueType value;
    std::vector<SkipListNode*> next;
    std::mutex mutex;

    SkipListNode(KeyType key, ValueType val, int level) : next(level, nullptr) {
        this->key = key;
        this->value = val;
        this->mutex = std::mutex()
    }
    ~SkipListNode(){
        this->next.clear();
    }
};

template <class KeyType, class ValueType>
class SkipList{
    const u_int32_t keyLen; // length of key
    const u_int32_t valueLen; // length of value
    const u_int32_t maxLevel; // 跳表的最大层数

    std::atomic_uint32_t level; // 当前跳表的层数
    SkipListNode<KeyType,ValueType>* head; // 头节点
    std::atomic_uint64_t nodeNum;

    std::atomic_uint32_t writeThreadCount;
    std::atomic_uint32_t readThreadCount;

    u_int32_t randomLevel(){
        u_int32_t lvl = 1;
        while (rand() % 2 == 0 && lvl < maxLevel)
            lvl++;
        return lvl;
    }
public:
    SkipList(int maxLvl) : maxLevel(maxLvl), level(0), nodeNum(0), keyLen(sizeof(KeyType)), valueLen(sizeof(ValueType)) {
        this->head = new SkipListNode<KeyType, ValueType>(KeyType(), ValueType(), this->maxLevel);
        this->writeThreadCount = 0;
        this->readThreadCount = 0
        //srand(static_cast<int>(time(nullptr)));
    }
    ~SkipList(){
        if(this->readThreadCount || this->writeThreadCount){
            std::cout << "Skip list warning: it is used by certain thread." << std::endl;
        }
        SkipListNode<KeyType,ValueType>* node = this->head;
        while(node != nullptr){
            SkipListNode<KeyType,ValueType>* node_tmp = node->next[0];
            delete node;
            node = node_tmp;
        }
    }
    void addWriteThread(){
        this->writeThreadCount++;
    }
    void ereaseWriteThread(){
        this->writeThreadCount--;
    }
    void addReadThread(){
        this->readThreadCount++;
    }
    void ereaseReadThread(){
        this->readThreadCount--;
    }
    void insert(KeyType key, ValueType value){
        // construct new node
        int newLevel = randomLevel();
        SkipListNode<KeyType, ValueType>* newNode = new SkipListNode<KeyType, ValueType>(key, value, newLevel);

        // get last nodes
        SkipListNode<KeyType, ValueType>* curr = this->head;
        std::vector<SkipListNode<KeyType, ValueType>*> update(maxLevel, nullptr);
        u_int32_t nowLevel = this->level;
        for (int i = std::max(newLevel - 1, nowLevel - 1); i >= 0; i--) {
            while (curr->next[i] != nullptr && curr->next[i]->key <= key)
                curr = curr->next[i];
            update[i] = curr;
        }

        // insert
        for(int i=0; i<newLevel; ++i){
            while(true){
                update[i]->mutex.lock();
                if(update[i]->next[i] != nullptr && update[i]->next[i]->key < key){// there may be new node inserted.
                    update[i]->mutex.unlock();
                    update[i] = update[i]->next[i];
                    continue;
                }else{
                    newNode->next[i] = update[i]->next[i];
                    update[i]->next[i] = newNode;
                    update[i]->mutex.unlock();
                    break;
                }
            }
        }

        this->nodeNum++;
        while (this->level < newLevel) {// CAS update level
            if (this->level.compare_exchange_strong(this->level, newLevel)) {
                break;
            }
        }
    }
    list<ValueType> findByKey(KeyType key){
        SkipListNode<KeyType, ValueType>* curr = this->head;
        std::list<ValueType> res = std::list<ValueType>();
        SkipListNode<KeyType,ValueType>* beginNode = nullptr;
        for (int i = this->level - 1; i >= 0; i--) {
            while (curr->next[i] != nullptr && curr->next[i]->key < key)
                curr = curr->next[i];
        }

        beginNode = curr->next[0];
        for (auto node = beginNode; node!=nullptr; node = node->next[0]){
            if(node->key < key){ // there may be new node inserted.
                continue;
            }
            if(node->key > key){
                break;
            }
            res.push_back(node->value);
        }
        return res;
    }
    list<ValueType> findByRange(KeyType begin, KeyType end){
        SkipListNode<KeyType, ValueType>* curr = this->head;
        std::list<ValueType> res = std::list<ValueType>();
        SkipListNode<KeyType,ValueType>* beginNode = nullptr;
        for (int i = this->level - 1; i >= 0; i--) {
            while (curr->next[i] != nullptr && curr->next[i]->key < begin)
                curr = curr->next[i];
        }
        
        beginNode = curr->next[0];
        for (auto node = beginNode; node!=nullptr; node = node->next[0]){
            if(node->key < begin){ // there may be new node inserted.
                continue;
            }
            if(node->key >= end){
                break;
            }
            res.push_back(node->value);
        }
        return res;
    }
    u_int64_t getNodeNum()const{
        return this->nodeNum;
    }
    std::string outputToChar(){
        if(this->writeThreadCount){
            std::cerr << "Skip list error: it is used by certain w-thread." << std::endl;
            return std::string();
        }
        
        u_int64_t buffer_len = this->nodeNum * (this->keyLen + this->valueLen);
        std::string data = std::string(0,buffer_len);
        u_int64_t offset = 0;
        for(auto node = head->next[0]; node!=nullptr; node = node->next[0]){
            memcpy(&(data[offset]),&(node->key),this->keyLen);
            offset += this->keyLen;
            memcpy(&(data[offset]),&(node->value),this->valueLen);
            offset += this->valueLen;
        }
        return data;
    }
};

#endif