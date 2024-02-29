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
    list<string> expList;

    void clearTree(QueryTreeNode* node);
    list<string> splitExpression();
    bool grammarVerify(list<string> exp_list);
    bool grammarVerifySimply(list<string> exp_list);
    list<string> ExpressionFormat(list<string> exp_list);
    void treeConstruct(list<string> exp_list, QueryTree* treeRoot);
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
    list<string> getExpList();
    string to_string();
};

struct AtomKey{
    enum KeyMode keyMode;
    u_int64_t key;
};

class Querier{
    IndexGenerator* generator;
    StorageOperator* storage;
    QueryTree tree;
    void intersect(list<u_int32_t>& la, list<u_int32_t>& lb);
    void join(list<u_int32_t>& la, list<u_int32_t>& lb);
    list<IndexReturn> intersect(list<IndexReturn>& la, list<IndexReturn>& lb);
    list<IndexReturn> join(list<IndexReturn>& la, list<IndexReturn>& lb);
public:
    Querier(IndexGenerator* generator,StorageOperator* storage){
        this->generator = generator;
        this->storage = storage;
        this->tree = QueryTree();
    }
    ~Querier()=default;
    // void atomize();
    list<IndexReturn> getOffsetByIndex(list<AtomKey> keyList, u_int64_t startTime, u_int64_t endTime);
    list<IndexReturn> getOffsetByIndex(AtomKey key, u_int64_t startTime, u_int64_t endTime);
    // void getPacketFile();
    void outputPacketToNewFile(string new_filename, list<IndexReturn> index_list);
    void queryWithExpression(string expression, string outputFileName, u_int64_t startTime, u_int64_t endTime);
};
