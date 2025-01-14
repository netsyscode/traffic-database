#ifndef BINARYTREE_HPP_
#define BINARYTREE_HPP_
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

#define CHILD_NUM 256

class ValueList{
    std::string key;
    std::list<u_int64_t> values;
public:
    ValueList(std::string key,u_int64_t value){
        this->key = key;
        this->values = std::list<u_int64_t>();
        this->values.push_back(value);
    }
    ~ValueList()=default;
    void insert(u_int64_t value){
        this->values.push_back(value);
    }
};

class TreeNode{
public:
    u_int32_t maxLayer;
    u_int32_t layer;
    std::vector<TreeNode*> children;
    // TreeNode* left;
    // TreeNode* right;
    TreeNode(u_int32_t max_layer,u_int32_t l):maxLayer(max_layer),layer(l){
        this->children = std::vector<TreeNode*>(CHILD_NUM,nullptr);
        // for(u_int32_t i=0;i<CHILD_NUM;++i){
        //     if(this->children[i]!=nullptr){
        //         printf("wrong %u\n",i);
        //     }
        // }
        // printf("new done.\n");
    }
    ~TreeNode(){
        if(this->layer == this->maxLayer){
            return;
        }
        for(auto c:this->children){
            if(c!=nullptr){
                if(this->layer == this->maxLayer){
                    delete (ValueList*)c;
                }else{
                    delete c;
                }
            }
        }
        this->children.clear();
    }
};


class BinaryTree{
    const u_int32_t maxLayer;
    const u_int32_t keyLen; // length of key
    const u_int32_t valueLen; 
    
    std::atomic_uint64_t nodeNum;

    TreeNode* root;
    
public:

    BinaryTree(u_int32_t maxLayer, u_int32_t keyLen, u_int32_t valueLen):
    maxLayer(maxLayer),keyLen(keyLen),valueLen(valueLen){
        // printf("begin.\n");
        this->nodeNum = 0;
        this->root = new TreeNode(this->maxLayer,0);
        // printf("end.\n");
    }

    ~BinaryTree(){
        delete this->root;
    }
    bool insert(std::string key, u_int64_t value){
        // construct new node
        
        if(key.size()!=this->keyLen){
            printf("Skip list error: insert with wrong key length %lu-%u!\n",key.size(),this->keyLen);
            // std::cerr << "Skip list error: insert with wrong key length!" <<std::endl;
            return true;
        }

        // printf("put one.\n");

        this->nodeNum++;

        // if(this->nodeNum % 10000 == 0){
        //     printf("node num:%lu\n",this->nodeNum.load());
        // }

        auto node = this->root;
        u_int32_t layer = 0;

        for(auto c:key){
            layer++;
            u_int8_t uc = (u_int8_t)c;
            // printf("node %lu.\n",(u_int64_t)node);
            // printf("layer: %u %u.\n",layer,(u_int8_t)c);
            if(layer == this->maxLayer){
                if(node->children[uc]==nullptr){
                    ValueList* values = new ValueList(key,value);
                    node->children[uc] = (TreeNode*)values;
                }else{
                    ((ValueList*)(node->children[uc]))->insert(value);
                }
                break;
            }
            if(node->children[uc]==nullptr){
                // printf("new.\n");
                node->children[uc] = new TreeNode(this->maxLayer,layer);
            }
            node = node->children[uc];
        }       
        return true; 
    }
    
};
#endif
