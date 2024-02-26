#ifndef SKIPLIST_HPP_
#define SKIPLIST_HPP_
#include <vector>
#include <list>
#include <iostream>
#include <random>
#include <ctime>
using namespace std;

template <class KeyType, class ValueType>
class SkipListNode{
public:
    KeyType key;
    ValueType value;
    vector<SkipListNode*> next;
    SkipListNode(KeyType key, ValueType val, int level) : next(level, nullptr) {
        this->key = key;
        this->value = val;
    }
    ~SkipListNode(){
        this->next.clear();
    }
};

template <class KeyType, class ValueType>
class SkipList{
    int maxLevel; // 跳表的最大层数
    int level; // 当前跳表的层数
    SkipListNode<KeyType,ValueType>* head; // 头节点
    int length;
    int randomLevel(){
        int lvl = 1;
        while (rand() % 2 == 0 && lvl < maxLevel)
            lvl++;
        return lvl;
    }
public:
    SkipList(int maxLvl) : maxLevel(maxLvl), level(0), length(0) {
        head = new SkipListNode<KeyType, ValueType>(KeyType(), ValueType(), maxLevel);
        //srand(static_cast<int>(time(nullptr)));
    }
    ~SkipList(){
        SkipListNode<KeyType,ValueType>* node = head;
        while(node != nullptr){
            SkipListNode<KeyType,ValueType>* node_tmp = node->next[0];
            delete node;
            //node = nullptr;
            node = node_tmp;
        }
    }
    void insert(KeyType key, ValueType value){
        SkipListNode<KeyType, ValueType>* curr = head;
        vector<SkipListNode<KeyType, ValueType>*> update(maxLevel, nullptr);
        for (int i = level - 1; i >= 0; i--) {
            while (curr->next[i] != nullptr && curr->next[i]->key <= key)
                curr = curr->next[i];
            update[i] = curr;
        }

        int newLevel = randomLevel();
        if (newLevel > level) {
            for (int i = level; i < newLevel; i++)
                update[i] = head;
            level = newLevel;
        }

        SkipListNode<KeyType, ValueType>* newNode = new SkipListNode<KeyType, ValueType>(key, value, newLevel);
        for (int i = 0; i < newLevel; i++) {
            newNode->next[i] = update[i]->next[i];
            update[i]->next[i] = newNode;
        }
        this->length++;
    }
    list<ValueType> findByKey(KeyType key){
        SkipListNode<KeyType, ValueType>* curr = head;
        list<ValueType> res = list<ValueType>();
        SkipListNode<KeyType,ValueType>* beginNode = nullptr;
        for (int i = level - 1; i >= 0; i--) {
            while (curr->next[i] != nullptr && curr->next[i]->key < key)
                curr = curr->next[i];
        }

        beginNode = curr->next[0];
        for (auto node = beginNode; node!=nullptr; node = node->next[0]){
            if(node->key > key)
                break;
            res.push_back(node->value);
        }
        return res;
    }
    list<ValueType> findByRange(KeyType begin, KeyType end){
        SkipListNode<KeyType, ValueType>* curr = head;
        list<ValueType> res = list<ValueType>();
        SkipListNode<KeyType,ValueType>* beginNode = nullptr;
        for (int i = level - 1; i >= 0; i--) {
            while (curr->next[i] != nullptr && curr->next[i]->key < begin)
                curr = curr->next[i];
        }
        
        beginNode = curr->next[0];
        for (auto node = beginNode; node!=nullptr; node = node->next[0]){
            if(node->key >= end)
                break;
            res.push_back(node->value);
        }
        return res;
    }
    u_int32_t getLength(){
        return this->length;
    }
    // void traverse(){
    //     for(auto node = head->next[0]; node!=nullptr; node = node->next[0]){
    //         printf("key:%llu,value:%llu\n",node->key,node->value);
    //         printf("level:%d\n",node->next.size());
    //     }
    // }
    char* outputToBuffer(){
        u_int32_t buffer_len = this->length * (sizeof(KeyType) + sizeof(ValueType));
        char* buffer = new char[buffer_len+sizeof(u_int32_t)];
        *(u_int32_t*)buffer = buffer_len;
        u_int32_t offset = sizeof(u_int32_t);
        //printf("buffer_len:%u\n",buffer_len);
        for(auto node = head->next[0]; node!=nullptr; node = node->next[0]){
            KeyType* key_pointer = (KeyType*)(buffer+offset);
            *key_pointer = node->key;
            //printf("key_pos:%u,",offset);
            offset += sizeof(KeyType);
            ValueType* value_pointer = (ValueType*)(buffer+offset);
            *value_pointer = node->value;
            //printf("value_pos:%u\n",offset);
            offset += sizeof(ValueType);
            //cout << "key:" << *key_pointer << ",value:" << *value_pointer << endl;
        }
        return buffer;
    }
    void readBuffer(char* buffer){
        u_int32_t buffer_len = *(u_int32_t*)buffer;
        //printf("buffer_len:%u\n",buffer_len);
        for(int offset = sizeof(u_int32_t); offset<buffer_len + sizeof(u_int32_t);){
            KeyType* key_pointer = (KeyType*)(buffer+offset);
            //printf("key_pos:%u,",offset);
            offset += sizeof(KeyType);
            ValueType* value_pointer = (ValueType*)(buffer+offset);
            //printf("value_pos:%u\n",offset);
            offset += sizeof(ValueType);
            cout << "key:" << *key_pointer << ",value:" << *value_pointer << endl;
            //printf("key:%llu,value:%u\n",*key_pointer,*value_pointer);
        }
    }
};

#endif