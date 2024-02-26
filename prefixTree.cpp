#include "prefixTree.hpp"

#define MAX_LENGTH 64

void PrefixTree::clearTree(struct PrefixNode* node){
    if(node == nullptr)
        return;
    if(node->endNode){
        node->childrenList.clear();
        delete node;
        return;
    }

    for(auto it: node->childrenList){
        clearTree((struct PrefixNode*)it);
    }
    node->childrenList.clear();
    delete node;
}

u_int8_t PrefixTree::getSlice(u_int64_t index, u_int8_t endPos, u_int8_t sliceLen){
    //if (endPos > this->prefixTotalLength || sliceLen > endPos)
    //printf("index: %llu, endPos: %d, sliceLen: %d\n",index,endPos,sliceLen);
    u_int8_t realPos = this->prefixTotalLength - endPos;
    u_int64_t mask = (1ULL << sliceLen) - 1;
    //printf("realPos:%d, pre: %llu, mask: %llu\n",realPos,index>>realPos,mask);
    return (uint8_t)((index >> realPos) & mask);
}

bool PrefixTree::setPrefixLength(list<u_int8_t> length){
    if(root != nullptr)
        return false;
    
    u_int8_t sum = 0;
    for(auto l : length)
        sum += l;
    if(sum != this->prefixTotalLength)
        return false;
    this->prefixLength.clear();
    this->prefixLength.assign(length.begin(),length.end());
    return true;
}

void PrefixTree::putData(u_int64_t index, void* data){
    if(root == nullptr){
        root = new PrefixNode();
        root->endNode = false;
        root->indexSlice = 0;
        root->childrenList = list<void*>();
    }
    u_int8_t now_pos = 0;
    PrefixNode* nowNode = root;
    for(auto l : this->prefixLength){
        now_pos += l;
        
        u_int8_t indexSlice = this->getSlice(index, now_pos, l);
        //printf("indexSlice:%d\n",indexSlice);
        bool has_found = false;
        auto it = nowNode->childrenList.begin();
        for (it; it != nowNode->childrenList.end(); ++it){
            PrefixNode* tmpNode = (PrefixNode*)(*it);
            if(tmpNode->indexSlice == indexSlice){
                nowNode = tmpNode;
                has_found = true;
                break;
            }
            if(tmpNode->indexSlice > indexSlice)
                break;
        }
        // for(auto c: nowNode->childrenList){
        //     PrefixNode* tmpNode = (PrefixNode*)c;
        //     if(tmpNode->indexSlice == indexSlice){
        //         nowNode = tmpNode;
        //         has_found = true;
        //         break;
        //     }
        // }
        if(has_found){
            continue;
        }
        
        PrefixNode* newNode = new PrefixNode();
        newNode->indexSlice = indexSlice;
        newNode->endNode = false;
        newNode->childrenList = list<void*>();

        nowNode->childrenList.insert(it,(void*)newNode);
        nowNode = newNode;
    }
    nowNode->endNode = true;
    nowNode->childrenList.push_back(data);
}

list<void*> PrefixTree::getDataByIndex(u_int64_t index){
    if(root == nullptr)
        return list<void*>();
    u_int8_t now_pos = 0;
    PrefixNode* nowNode = root;
    for(auto l : this->prefixLength){
        now_pos += l;
        u_int8_t indexSlice = this->getSlice(index, now_pos, l);
        bool has_found = false;
        for(auto c: nowNode->childrenList){
            PrefixNode* tmpNode = (PrefixNode*)c;
            if(tmpNode->indexSlice == indexSlice){
                nowNode = tmpNode;
                has_found = true;
                break;
            }
            if(tmpNode->indexSlice > indexSlice)
                break;
        }
        if(!has_found)
            return list<void*>();
    }
    
    return nowNode->childrenList;
}

void PrefixTree::print(){
    list<void*> now_list = list<void*>();
    list<void*> tmp_list = list<void*>();
    now_list.push_back(root);
    while(!now_list.empty()){
        tmp_list.clear();
        printf("node:");
        for(auto node:now_list){
            printf("%d ",((PrefixNode*)node)->indexSlice);
            if(((PrefixNode*)node)->endNode)
                continue;
            tmp_list.insert(tmp_list.end(), ((PrefixNode*)node)->childrenList.begin(), ((PrefixNode*)node)->childrenList.end());
        }
        now_list.clear();
        now_list.assign(tmp_list.begin(),tmp_list.end());
        printf("\n");
    }
}