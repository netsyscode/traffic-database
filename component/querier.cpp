#include "querier.hpp"
#include <regex>
#include <stack>
#include <fstream>

#define PUSH_TMP if(tmp.size()){op = tmp.back();if(op=='='||op=='|'||op=='&'||op=='^'){return std::list<std::string>();}exp_list.push_back(tmp);tmp.clear();}

const std::string connector[] = {"&&", "||", "^^", "in"};
const char leftBracket[] = "({[\"";
const char rightBracket[] = ")}]\"";
const std::string opt[] = {"==", "!=", ">=", "<=", "contains", ">", "<"};

bool isNumeric(const std::string& s) {
    bool numSeen = false;
    bool dotSeen = false;
    bool signSeen = false; // 用于跟踪正负号是否已经出现

    for (int i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (isdigit(c)) {
            numSeen = true;
        } else if (c == '.') {
            if (dotSeen) return false; // 小数点只能出现一次
            dotSeen = true;
        } else if (c == '-') {
            if (signSeen || numSeen || (dotSeen && i > 0 && s[i-1] != '.')) return false;
            signSeen = true;
        } else {
            return false;
        }
    }
    return numSeen; // 必须至少包含一个数字
}
bool isValidIPv4(const std::string& ip) {
    //cout << "ipv4" << endl;
    std::regex ipv4Regex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
                         "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                         "(/(3[0-2]|[12]?[0-9]))?$"); // 匹配 /0 到 /32 的子网掩码
    return regex_match(ip, ipv4Regex);
}
bool isValidIPv6(const std::string& ip) {
    //cout << "ipv6" << endl;
    std::regex ipv6Regex("^((\\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:)))(%.+)?\\s*)(/(([1-9])|([1-9][0-9])|(1[0-1][0-9]|12[0-8]))){0,1})*$");

    return regex_match(ip, ipv6Regex);
    //return false;
}
bool isValidMACAddress(const std::string& address) {
    //cout << "mac" << endl;
    std::regex macRegex("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$"
                        "|^([0-9A-Fa-f]{12})$"); // 匹配冒号、短横线分隔或无分隔的格式
    return regex_match(address, macRegex);
}
bool isBoolean(const std::string& s){
    return s=="true"||s=="false"||s=="1"||s=="0";
}
bool isString(const std::string& s){
    return s.size()>=2 && s.front()=='\"' && s.back()=='\"';
}
bool isVar(const std::string& str) {
    std::regex formatRegex("^[a-zA-Z][a-zA-Z0-9]*(\\.[a-zA-Z][a-zA-Z0-9]*)*$");
    return regex_match(str, formatRegex);
}
bool isValue(const std::string& s){
    return isNumeric(s) || isValidIPv4(s) || isValidIPv6(s) || isValidMACAddress(s) || isBoolean(s) || isString(s);
}
bool isSlice(const std::string& s){
    std::regex formatRegex("^\\d*:\\d*(:\\d*)?$"); 
    return regex_match(s, formatRegex);
}

enum LastType{
    EMPTY = 0, //next with var, !, exp, bracket
    VAR, //next with op, con, square_bracket, in,
    OP, //next with normal value
    CON, //next with var, !, exp, bracket
    EXP, //next with con, )
    BRACKET, //left, next with var, !, exp, bracket, )
    SQUARE_BRACKET, //left, next with slice
    SLICE, //next with ]
    IN, //next with brace
    BRACE, //left, next with value
    VALUE, //just in BRACE, next with comma, }
    COMMA, //next with value
    LAST_TYPE_NUM,
};

void QueryTree::clearTree(QueryTreeNode* node){
    if(node == nullptr){
        return;
    }
    for(auto child:node->children){
        clearTree(child);
    }
    node->children.clear();
    delete node;
}
std::list<std::string> QueryTree::splitExpression(){
    std::list<std::string> exp_list = std::list<std::string>();

    std::string tmp = std::string();
    char op = ' ';
    for(auto c:this->originExpression){
        // cout << c << ":";
        // for(auto s:exp_list){
        //     printf("%s ",s.c_str());
        // }
        // printf("\n");
        op = ' ';
        op = tmp.back();
        if(c!='='&&(op=='!' || op=='<' || op=='>')){
            exp_list.push_back(tmp);
            tmp.clear();
        }

        switch (c){
            case ' ':
                PUSH_TMP
                break;
            case '(':
            case ')':
            case '{':
            case '}':
            case '[':
            case ']':
            case ',':
                PUSH_TMP
                tmp.push_back(c);
                exp_list.push_back(tmp);
                tmp.clear();
                break;
            case '!':
            case '<':
            case '>':
                PUSH_TMP
                tmp.push_back(c);
                break;
            case '=':
                op = tmp.back();
                if(op == '!' || op == '<' || op == '>' || op == '='){
                    tmp.push_back(c);
                    exp_list.push_back(tmp);
                    tmp.clear();
                    break;
                }
                PUSH_TMP
                tmp.push_back(c);
                break;
            case '|':
            case '&':
            case '^':
                op = tmp.back();
                if(op == c){
                    tmp.push_back(c);
                    exp_list.push_back(tmp);
                    tmp.clear();
                    break;
                }
                PUSH_TMP
                tmp.push_back(c);
                break;
            default:
                tmp.push_back(c);
                break;
        }
    }
    PUSH_TMP
    return exp_list;
}
bool QueryTree::grammarVerify(std::list<std::string> exp_list){
    std::stack<std::string> bracket_stack = std::stack<std::string>();
    LastType lastType = LastType::EMPTY;

    for(auto s:exp_list){
        // cout << s << endl;
        if(s=="!"){
            if(lastType != LastType::EMPTY && lastType != LastType::CON && lastType != LastType::BRACKET)
                return false;
            lastType = LastType::CON;
            continue;
        }
        if(s=="("){
            if(lastType != LastType::EMPTY && lastType != LastType::CON && lastType != LastType::BRACKET)
                return false;
            bracket_stack.push(s);
            lastType = LastType::BRACKET;
            continue;
        }
        if(s=="{"){
            if(lastType != LastType::IN)
                return false;
            bracket_stack.push(s);
            lastType = LastType::BRACE;
            continue;
        }
        if(s=="["){
            if(lastType != LastType::VAR)
                return false;
            bracket_stack.push(s);
            lastType = LastType::SQUARE_BRACKET;
            continue;
        }
        if(s==")"){
            if(bracket_stack.top() != "(" && lastType != LastType::EXP && lastType != LastType::BRACKET)
                return false;
            bracket_stack.pop();
            lastType = LastType::EXP;
            continue;
        }
        if(s=="}"){
            if(bracket_stack.top() != "(" && lastType != LastType::VALUE)
                return false;
            bracket_stack.pop();
            lastType = LastType::EXP;
            continue;
        }
        if(s=="]"){
            if(bracket_stack.top() != "(" && lastType != LastType::SLICE)
                return false;
            bracket_stack.pop();
            lastType = LastType::VAR;
            continue;
        }
        if(s=="==" || s=="!=" || s==">" || s=="<" || s==">=" || s=="<=" || s=="contains" ){
            if(lastType != LastType::VAR)
                return false;
            lastType = LastType::OP;
            continue;
        }
        if(s=="||" || s=="&&" || s=="^^"){
            if(lastType != LastType::VAR && lastType != LastType::EXP)
                return false;
            lastType = LastType::CON;
            continue;
        }
        if(s=="in"){
            if(lastType != LastType::VAR)
                return false;
            lastType = LastType::IN;
            continue;
        }
        if(s==","){
            if(lastType != LastType::VALUE)
                return false;
            lastType = LastType::COMMA;
            continue;
        }
        if(isSlice(s)){
            if(lastType != LastType::SQUARE_BRACKET)
                return false;
            lastType = LastType::SLICE;
            continue;
        }
        if(isVar(s)){
            if(lastType != LastType::EMPTY && lastType != LastType::CON && lastType != LastType::BRACKET)
                return false;
            lastType = LastType::VAR;
            continue;
        }
        if(isValue(s)){
            if(lastType == LastType::OP){
                lastType = LastType::EXP;
                continue;
            }
            if(lastType == LastType::BRACE || lastType == LastType::COMMA){
                lastType = LastType::VALUE;
                continue;
            }
            return false;
        }
        return false;
    }
    if(!bracket_stack.empty()){
        return false;
    }
    return true;
}

bool QueryTree::grammarVerifySimply(std::list<std::string> exp_list){ //only for (),||,&&,==
    int exp_status = 0;
    for(auto s:exp_list){
        if(isVar(s)){
            if(exp_status!=0){
                return false;
            }
            exp_status = 1;
            continue;
        }
        if(s=="=="){
            if(exp_status!=1){
                return false;
            }
            exp_status = 2;
            continue;
        }
        if(isValue(s)){
            if(exp_status!=2){
                return false;
            }
            exp_status = 0;
            continue;
        }
        if(s=="("||s==")"||s=="||"||s=="&&"){
            if(exp_status!=0){
                return false;
            }
            continue;
        }
        return false;
    }
    return true;
}
bool QueryTree::inputExpression(std::string exp){
    this->originExpression = exp;
    // std::cout << "Query Tree log: inputExpression: " <<this->originExpression << std::endl;

    std::list<std::string> exp_list = this->splitExpression();
    if(exp_list.empty()){
        return false;
    }
    // for(auto s:exp_list){
    //     printf("%s ",s.c_str());
    // }
    // printf("\n");
    if(!grammarVerify(exp_list)){
        return false;
    }
    if(!grammarVerifySimply(exp_list)){
        return false;
    }
    this->expList = exp_list;
    return true;
}
std::list<std::string> QueryTree::getExpList(){
    return this->expList;
}


void Querier::intersect(std::list<u_int32_t>& la, std::list<u_int32_t>& lb){
    la.sort();
    lb.sort();
    auto ita = la.begin();
    auto itb = lb.begin();
    while (ita != la.end() && itb != lb.end()) {
        if (*ita < *itb) {
            ita = la.erase(ita);
        } else if (*ita > *itb) {
            ++itb;
        } else {
            ++ita;
            ++itb;
        }
    }
    la.erase(ita,la.end());
}
void Querier::join(std::list<u_int32_t>& la, std::list<u_int32_t>& lb){
    la.sort();
    lb.sort();
    std::list<u_int32_t> ret = std::list<u_int32_t>();
    auto ita = la.begin();
    auto itb = lb.begin();
    while (ita != la.end() && itb != lb.end()) {
        if (*ita < *itb) {
            ret.push_back(*ita);
            ++ita;
        } else if (*ita > *itb) {
            ret.push_back(*itb);
            ++itb;
        } else {
            ret.push_back(*ita);
            ++ita;
            ++itb;
        }
    }
    while (ita != la.end()){
        ret.push_back(*ita);
        ++ita;
    }
    while (itb != lb.end()){
        ret.push_back(*itb);
        ++itb;
    }
    la = ret;
}
std::list<std::string> Querier::decomposeExpression(){
    if(!this->tree.inputExpression(expression)){
        return std::list<std::string>();
    }
    return this->tree.getExpList();
}
std::list<u_int32_t> Querier::getPointerByFlowMetaIndex(AtomKey key){
    return (*(this->flowMetaIndexCaches))[key.cachePos]->findByKey(key.key);
}
std::list<u_int32_t> Querier::searchExpression(std::list<std::string> exp_list){
    std::stack<std::list<u_int32_t>> before_lists = std::stack<std::list<u_int32_t>>();
    std::stack<std::string> ops = std::stack<std::string>();
    std::list<u_int32_t> left_list = std::list<u_int32_t>();
    std::list<u_int32_t> right_list = std::list<u_int32_t>();
    AtomKey key;
    int bracket_count = 0;
    int wait_for_value = 0;
    std::string op_now = "||";
    for(auto s:exp_list){
        if(s=="("){
            bracket_count++;
            ops.push(op_now);
            op_now = "||";
            before_lists.push(left_list);
            left_list = std::list<u_int32_t>();
            continue;
        }
        if(s=="srcip"||s=="dstip"){
            key.cachePos = s=="srcip"?0:1;
            wait_for_value = 1;
            continue;
        }
        if(s=="srcport"||s=="dstport"){
            key.cachePos = s=="srcport"?2:3;
            wait_for_value = 2;
            continue;
        }
        if(s=="=="){
            continue;
        }
        if(s=="&&" || s=="||"){
            op_now = s;
            continue;
        }
        if(s==")"){
            op_now = ops.top();
            ops.pop();
            right_list = left_list;
            left_list = before_lists.top();
            before_lists.pop();
            if(op_now == "||"){
                this->join(left_list,right_list);
            }else{
                this->intersect(left_list,right_list);
            }
            continue;
        }
        if(wait_for_value){
            if(wait_for_value==2){
                u_int16_t port = (u_int16_t)stoul(s);
                key.key = std::string((char*)&port,sizeof(port));
                // std::cout << "key: " << port << " pos: " << key.cachePos <<std::endl;
            }else{
                struct in_addr tmp;
                inet_aton(s.c_str(), &tmp);
                u_int32_t ipv4 = ntohl(tmp.s_addr);
                key.key = std::string((char*)&ipv4,sizeof(ipv4));
            }
            right_list = this->getPointerByFlowMetaIndex(key);
            if(op_now == "||"){
                this->join(left_list,right_list);
            }else{
                this->intersect(left_list,right_list);
            }
            continue;
        }
    }
    return left_list;
}
void Querier::outputPacketToFile(std::list<u_int32_t> flowHeadList){
    std::ofstream outputFile(this->outputFilename, std::ios::binary);
    if (!outputFile.is_open()) {
        std::cerr << "Querier error: outputPacketToFile to non-exist file name " << this->outputFilename << "!" << std::endl;
        return;
    }
    outputFile.write(this->pcapHeader.c_str(),this->pcapHeader.size());
    // char buffer[2000];

    u_int32_t pos = 0;
    u_int32_t next = 0;
    for(auto flow_head:flowHeadList){
        next = flow_head;   
        while(true){
            if(next == std::numeric_limits<uint32_t>::max()){
                break;
            }
            pos = next;
            // std::cout << "pos: " << pos << std::endl;
            u_int32_t offset = this->packetPointer->getValueReadOnly(pos);
            std::string data = this->packetBuffer->readPcap(offset);
            outputFile.write(data.c_str(),data.size());
            next = this->packetPointer->getNext(pos);
        }
    }
    outputFile.close();
}
void Querier::input(std::string expression, std::string outputFilename){
    this->expression = expression;
    this->outputFilename = outputFilename;
}
void Querier::runUnit(){
    std::list<std::string> exp_list = this->decomposeExpression();
    if(exp_list.size()==0){
        std::cerr<<"Querier error: run with wrong expression!" << std::endl;
        return;
    }
    std::list<u_int32_t> flow_header_list = this->searchExpression(exp_list);
    this->outputPacketToFile(flow_header_list);
}
void Querier::run(){
    std::cout << "Querier log: query begin, enter your request below (q for QUIT)." <<std::endl;
    std::string filename;
    std::string expression;
    while(true){
        // std::getline(std::cin, filename);
        filename = "./data/output/pcap_multithread.pcap"; // just as test
        if(filename == "q"){
            break;
        }
        std::getline(std::cin, expression);
        if(expression == "q"){
            break;
        }
        this->input(expression,filename);
        this->runUnit();
    }
    std::cout << "Controller log: query end." <<std::endl;
}