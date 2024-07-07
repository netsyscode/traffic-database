#ifndef QUERIER_HPP_
#define QUERIER_HPP_
#include <vector>
#include <string>
#include <list>
#include <chrono>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../dpdk_lib/header.hpp"
// #include "../lib/shareBuffer.hpp"
// #include "../lib/arrayList.hpp"
#include "storage.hpp"

struct Answer{
    u_int32_t block_id;
    std::list<u_int64_t> pointers;
};

struct AtomKey{
    u_int32_t cachePos;
    std::string key;
};

struct QueryTreeNode{
    std::string exp;
    std::list<QueryTreeNode*> children;
};

class QueryTree{
    std::string originExpression;
    QueryTreeNode* root;
    std::list<std::string> expList;

    void clearTree(QueryTreeNode* node);
    std::list<std::string> splitExpression();
    bool grammarVerify(std::list<std::string> exp_list);
    bool grammarVerifySimply(std::list<std::string> exp_list);
    // std::list<std::string> ExpressionFormat(std::list<std::string> exp_list);
    // void treeConstruct(std::list<std::string> exp_list, QueryTree* treeRoot);
public:
    QueryTree(){
        this->originExpression = std::string();
        this->root = nullptr;
    }
    ~QueryTree(){
        this->clearTree(this->root);
    }
    bool inputExpression(std::string exp);
    std::list<std::string> getExpList();
    // std::string to_string();
};

class Querier{
    std::string expression;
    std::string outputFilename;
    std::string pcapHeader;

    u_int64_t startTime;
    u_int64_t endTime;
    u_int64_t packet_count;

    // const std::string index_name[FLOW_META_INDEX_NUM] = {
    //     "./data/index/pcap.pcap_srcip_idx",
    //     "./data/index/pcap.pcap_dstip_idx",
    //     "./data/index/pcap.pcap_srcport_idx",
    //     "./data/index/pcap.pcap_dstport_idx",
    // };
    // const std::string pointer_name = "./data/index/pcap.pcappt";
    // const std::string data_name = "./data/index/pcap.pcap";

    // shared memory, read only
    // ShareBuffer* packetBuffer;
    // ArrayList<u_int32_t>* packetPointer;
    // std::vector<SkipList*>* flowMetaIndexCaches;
    std::vector<StorageMeta>* storageMetas;
    
    QueryTree tree;

    void intersect(std::list<u_int64_t>& la, std::list<u_int64_t>& lb);
    void join(std::list<u_int64_t>& la, std::list<u_int64_t>& lb);
    void intersect(std::list<Answer>& la, std::list<Answer>& lb);
    void join(std::list<Answer>& la, std::list<Answer>& lb);

    std::list<std::string> decomposeExpression();
    std::list<Answer> getPointerByFlowMetaIndex(AtomKey key);
    std::list<Answer> getPointerByFlowMetaRange(AtomKey startKey,AtomKey endKey);
    std::list<Answer> searchExpression(std::list<std::string> exp_list);
    char* mmapFile(int fileFD, u_int64_t fileSize);
    void closeFile(int fileFD, char* buffer, u_int64_t fileSize);
    u_int64_t getFileSize(int fileFD);
    void outputPacketToFile(std::list<Answer> flowHeadList);
    bool runUnit();
public:
    Querier(std::vector<StorageMeta>* storageMetas, std::string pcapHeader){
        // std::cout << "Querier construct." <<std::endl;
        // this->packetBuffer = packetBuffer;
        // this->packetPointer = packetPointer;
        // this->flowMetaIndexCaches = flowMetaIndexCaches;
        this->expression = std::string();
        this->outputFilename = std::string();
        this->pcapHeader = pcapHeader;
        this->storageMetas = storageMetas;
        this->tree = QueryTree();
        // std::cout << "Querier construct end." <<std::endl;
    }
    ~Querier()=default;
    void input(std::string expression, std::string outputFilename, std::string start_time, std::string end_time);
    void run();
};

#endif