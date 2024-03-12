#ifndef QUERIER_HPP_
#define QUERIER_HPP_
#include <vector>
#include <string>
#include <list>
#include "../lib/skipList.hpp"
#include "../lib/shareBuffer.hpp"
#include "../lib/arrayList.hpp"

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

    // shared memory, read only
    ShareBuffer* packetBuffer;
    ArrayList<u_int32_t>* packetPointer;
    std::vector<SkipList*>* flowMetaIndexCaches;
    
    QueryTree tree;

    void intersect(std::list<u_int32_t>& la, std::list<u_int32_t>& lb);
    void join(std::list<u_int32_t>& la, std::list<u_int32_t>& lb);

    std::list<std::string> decomposeExpression();
    std::list<u_int32_t> getPointerByFlowMetaIndex(AtomKey key);
    std::list<u_int32_t> searchExpression(std::list<std::string> exp_list);
    void outputPacketToFile(std::list<u_int32_t> flowHeadList);
    void runUnit();
public:
    Querier(ShareBuffer* packetBuffer, ArrayList<u_int32_t>* packetPointer, std::vector<SkipList*>* flowMetaIndexCaches, std::string pcapHeader){
        std::cout << "Querier construct." <<std::endl;
        this->packetBuffer = packetBuffer;
        this->packetPointer = packetPointer;
        this->flowMetaIndexCaches = flowMetaIndexCaches;
        this->expression = std::string();
        this->outputFilename = std::string();
        this->pcapHeader = pcapHeader;
        this->tree = QueryTree();
    }
    ~Querier()=default;
    void input(std::string expression, std::string outputFilename);
    void run();
};

#endif