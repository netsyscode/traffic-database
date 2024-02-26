#include <iostream>
#include <string>
#include <list>
#include "storage.hpp"
using namespace std;

struct QueryTreeNode{
    string exp;
    list<QueryTreeNode*> children;
};

class QueryTree{
    string originExpression;
    QueryTreeNode* root;
    void clearTree(QueryTreeNode* node);
    list<string> splitExpression();
    bool grammarVerify(list<string> exp_list);
    void treeConstruct(list<string> exp_list);
public:
    QueryTree(){
        this->originExpression = string();
        this->root = nullptr;
    }
    ~QueryTree(){
        this->clearTree(this->root);
    }
    bool inputExpression(string exp);
    // void constructExpressionTree();
    string to_string();
};

struct AtomKey{
    enum KeyMode keyMode;
    u_int64_t key;
};

class Querier{
    IndexGenerator* generator;
    StorageOperator* storage;
    void intersect(list<u_int32_t>& la, list<u_int32_t>& lb);
public:
    Querier(IndexGenerator* generator,StorageOperator* storage){
        this->generator = generator;
        this->storage = storage;
    }
    ~Querier()=default;
    // void atomize();
    list<IndexReturn> getOffsetByIndex(list<AtomKey> keyList, u_int64_t startTime, u_int64_t endTime);
    // void getPacketFile();
    void outputPacketToNewFile(string new_filename, list<IndexReturn> index_list);
};
