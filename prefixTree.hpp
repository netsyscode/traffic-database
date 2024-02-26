#ifndef PREFIXTREE_HPP_
#define PREFIXTREE_HPP_
#include <list>
#include <string>
#include <iostream>
using namespace std;

struct PrefixNode{
    u_int8_t indexSlice;
    bool endNode;
    list<void*> childrenList;
};

class PrefixTree{
    struct PrefixNode* root;
    list<u_int8_t> prefixLength;
    u_int8_t prefixTotalLength;
    void clearTree(struct PrefixNode* node);
    u_int8_t getSlice(u_int64_t index, u_int8_t endPos, u_int8_t sliceLen);
public:
    PrefixTree(u_int8_t prefixTotalLength){
        root = nullptr;
        this->prefixTotalLength = prefixTotalLength;
        this->prefixLength = list<u_int8_t>();
        for(int i=0; i<this->prefixTotalLength;++i){
            this->prefixLength.push_back(1);
        }
    }
    ~PrefixTree(){
        clearTree(root);
    }
    list<void*> getDataByIndex(u_int64_t index);
    void putData(u_int64_t index, void* data);
    bool setPrefixLength(list<u_int8_t> length);
    void print();
    list<void*> getDataByRange(u_int64_t start, u_int64_t end);
    list<void*> getDataByPrefix(u_int64_t prefix, u_int8_t prefixLen);
};

#endif