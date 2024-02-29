#include "querier.hpp"
#include <stack>
#include <cctype>
#include <regex>
#include <sstream>
#define PCAP_HEAD_LEN 24
#define PUSH_TMP if(tmp.size()){op = tmp.back();if(op=='='||op=='|'||op=='&'||op=='^'){return list<string>();}exp_list.push_back(tmp);tmp.clear();}

const u_int8_t pcap_head[] = {0xd4,0xc3,0xb2,0xa1,0x02,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                            0x00,0x00,0x04,0x00,0x65,0x00,0x00,0x00};

const string connector[] = {"&&", "||", "^^", "in"};
const char leftBracket[] = "({[\"";
const char rightBracket[] = ")}]\"";
const string opt[] = {"==", "!=", ">=", "<=", "contains", ">", "<"};

bool isNumeric(const string& s) {
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
bool isValidIPv4(const string& ip) {
    //cout << "ipv4" << endl;
    regex ipv4Regex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
                         "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                         "(/(3[0-2]|[12]?[0-9]))?$"); // 匹配 /0 到 /32 的子网掩码
    return regex_match(ip, ipv4Regex);
}
bool isValidIPv6(const string& ip) {
    //cout << "ipv6" << endl;
    regex ipv6Regex("^((\\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)(\\.(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)){3}))|:)))(%.+)?\\s*)(/(([1-9])|([1-9][0-9])|(1[0-1][0-9]|12[0-8]))){0,1})*$");

    return regex_match(ip, ipv6Regex);
    //return false;
}
bool isValidMACAddress(const string& address) {
    //cout << "mac" << endl;
    regex macRegex("^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$"
                        "|^([0-9A-Fa-f]{12})$"); // 匹配冒号、短横线分隔或无分隔的格式
    return regex_match(address, macRegex);
}
bool isBoolean(const string& s){
    return s=="true"||s=="false"||s=="1"||s=="0";
}
bool isString(const string& s){
    return s.size()>=2 && s.front()=='\"' && s.back()=='\"';
}
bool isVar(const string& str) {
    regex formatRegex("^[a-zA-Z][a-zA-Z0-9]*(\\.[a-zA-Z][a-zA-Z0-9]*)*$");
    return regex_match(str, formatRegex);
}
bool isValue(const string& s){
    return isNumeric(s) || isValidIPv4(s) || isValidIPv6(s) || isValidMACAddress(s) || isBoolean(s) || isString(s);
}
bool isSlice(const string& s){
    std::regex formatRegex("^\\d*:\\d*(:\\d*)?$"); 
    return std::regex_match(s, formatRegex);
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

list<string> QueryTree::splitExpression(){
    list<string> exp_list = list<string>();

    string tmp = string();
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

bool QueryTree::grammarVerify(list<string> exp_list){
    stack<string> bracket_stack = stack<string>();
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

bool QueryTree::grammarVerifySimply(list<string> exp_list){ //only for (),||,&&,==
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
// list<string> QueryTree::ExpressionFormat(list<string> exp_list){

// }

void QueryTree::treeConstruct(list<string> exp_list, QueryTree* treeRoot){ //Easy Implement
    //treeRoot = new QueryTree();
}

bool QueryTree::inputExpression(string exp){
    this->originExpression = exp;

    list<string> exp_list = this->splitExpression();
    if(exp_list.empty()){
        return false;
    }
    for(auto s:exp_list){
        printf("%s ",s.c_str());
    }
    printf("\n");
    if(!grammarVerify(exp_list)){
        return false;
    }
    if(!grammarVerifySimply(exp_list)){
        return false;
    }
    this->expList = exp_list;
    return true;
}

list<string> QueryTree::getExpList(){
    return this->expList;
}



void Querier::intersect(list<u_int32_t>& la, list<u_int32_t>& lb){
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

void Querier::join(list<u_int32_t>& la, list<u_int32_t>& lb){
    la.sort();
    lb.sort();
    list<u_int32_t> ret = list<u_int32_t>();
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

list<IndexReturn> Querier::intersect(list<IndexReturn>& la, list<IndexReturn>& lb){
    list<IndexReturn> ret = list<IndexReturn>();
    for(auto a:la){
        for(auto b:lb){
            if(a.filename==b.filename){
                this->intersect(a.packetList,b.packetList);
                ret.push_back(a);
            }
        }
    }
    return ret;
}
list<IndexReturn> Querier::join(list<IndexReturn>& la, list<IndexReturn>& lb){
    list<IndexReturn> ret = list<IndexReturn>();
    for(auto a:la){
        bool exist = false;
        for(auto b:lb){
            if(a.filename==b.filename){
                // cout<<"join"<<endl;
                join(b.packetList,a.packetList);
                exist = true;
                ret.push_back(b);
                // for(auto id:b.packetList){
                //     cout<<id<<" ";
                // }
                // cout<<endl;
                break;
            }
        }
        if(!exist){
            ret.push_back(a);
        }
    }
    for(auto b:lb){
        bool exist = false;
        for(auto a:la){
            exist = true;
            break;
        }
        if(!exist){
            ret.push_back(b);
        }
    }
    //ret.insert(ret.end(),lb.begin(),lb.end());
    // for(auto id:ret.front().packetList){
    //     cout<<id<<" ";
    // }
    // cout<<endl;
    return ret;
}

list<IndexReturn> Querier::getOffsetByIndex(list<AtomKey> keyList, u_int64_t startTime, u_int64_t endTime){
    list<IndexReturn> ret_list = list<IndexReturn>();
    for(auto key:keyList){
        u_int64_t trancateTime = this->generator->getStartTime();
        if(trancateTime != 0 && trancateTime < endTime){
            list<IndexReturn> tmp_list = this->generator->getPacketOffsetByIndex(key.keyMode,key.key);
            for(auto tmp:tmp_list){
                bool has_before = false;
                for(auto ret:ret_list){
                    if(ret.filename == tmp.filename){
                        this->intersect(ret.packetList,tmp.packetList);
                        has_before = true;
                    }
                }
                if(!has_before){
                    ret_list.push_back(tmp);
                }
            }
        }
        list<IndexReturn> tmp_list = this->storage->getPacketOffsetByIndex(key.keyMode,key.key,startTime,endTime);
        for(auto tmp:tmp_list){
            bool has_before = false;
            for(auto ret:ret_list){
                if(ret.filename == tmp.filename){
                    this->intersect(ret.packetList,tmp.packetList);
                    has_before = true;
                }
            }
            if(!has_before){
                ret_list.push_back(tmp);
            }
        }
    }
    return ret_list;
}

list<IndexReturn> Querier::getOffsetByIndex(AtomKey key, u_int64_t startTime, u_int64_t endTime){
    list<IndexReturn> ret_list = list<IndexReturn>();
    u_int64_t trancateTime = this->generator->getStartTime();
    //cout<<trancateTime<<" "<<endTime<<endl;
    if(trancateTime != 0 && trancateTime < endTime){
        //cout<<"wrong\n";
        ret_list = this->generator->getPacketOffsetByIndex(key.keyMode,key.key);
        // for(auto tmp:tmp_list){
        //     bool has_before = false;
        //     for(auto ret:ret_list){
        //         if(ret.filename == tmp.filename){
        //             this->intersect(ret.packetList,tmp.packetList);
        //             has_before = true;
        //         }
        //     }
        //     if(!has_before){
        //         ret_list.push_back(tmp);
        //     }
        // }
    }
    list<IndexReturn> tmp_list = this->storage->getPacketOffsetByIndex(key.keyMode,key.key,startTime,endTime);
    // cout << tmp_list.size()<<endl;
    for(auto tmp:tmp_list){
        bool has_before = false;
        for(auto ret:ret_list){
            if(ret.filename == tmp.filename){
                this->intersect(ret.packetList,tmp.packetList);
                has_before = true;
            }
        }
        if(!has_before){
            ret_list.push_back(tmp);
        }
    }
    //ret_list.insert(ret_list.end(),tmp_list.begin(),tmp_list.end());
    return ret_list;
}

void Querier::outputPacketToNewFile(string new_filename, list<IndexReturn> index_list){
    ofstream outputFile(new_filename, ios::binary);
    if (!outputFile.is_open()) {
        printf("Fail to Create File!\n");
        return;
    }
    outputFile.write((char*)pcap_head,PCAP_HEAD_LEN);
    char buffer[2000];

    for(auto res:index_list){
        ifstream inFile(res.filename, ios::binary);
        if(!inFile.is_open()){
            printf("Fail to Open File:%s!\n",res.filename.c_str());
            continue;
        }
        for(auto offset:res.packetList){
            inFile.seekg(offset,ios::beg);
            struct data_header data_head;
            inFile.read((char*)&data_head,sizeof(struct data_header));
            u_int32_t length = data_head.caplen;
            outputFile.write((char*)&data_head,sizeof(struct data_header));
            inFile.read(buffer,length);
            outputFile.write(buffer,length);
        }
        inFile.close();
    }
    outputFile.close();
}

void Querier::queryWithExpression(string expression, string outputFileName, u_int64_t startTime, u_int64_t endTime){//Only for Easy Implement
    if(!this->tree.inputExpression(expression)){
        return;
    }
    list<string> exp_list = this->tree.getExpList();
    stack<list<IndexReturn>> before_lists = stack<list<IndexReturn>>();
    stack<string> ops = stack<string>();
    list<IndexReturn> left_list = list<IndexReturn>();
    list<IndexReturn> right_list = list<IndexReturn>();
    AtomKey key;
    int bracket_count = 0;
    int wait_for_value = 0;
    string op_now = "||";
    for(auto s:exp_list){
        // cout<<s<<endl;
        if(s=="("){
            bracket_count++;
            ops.push(op_now);
            op_now = "||";
            before_lists.push(left_list);
            left_list = list<IndexReturn>();
            continue;
        }
        if(s=="srcip"||s=="dstip"){
            key.keyMode = s=="srcip"?KeyMode::SRCIP:KeyMode::DSTIP;
            wait_for_value = 1;
            continue;
        }
        if(s=="srcport"||s=="dstport"){
            key.keyMode = s=="srcport"?KeyMode::SRCPORT:KeyMode::DSTPORT;
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
            left_list = op_now == "||"?this->join(left_list,right_list):this->intersect(left_list,right_list);

            // for(auto id:left_list.front().packetList){
            //     cout<<id<<" ";
            // }
            // cout<<endl;
            continue;
        }
        if(wait_for_value){
            if(wait_for_value==2){
                key.key = stoull(s);
            }else{
                struct in_addr tmp;
                inet_aton(s.c_str(), &tmp);
                key.key = ntohl(tmp.s_addr);
            }
            right_list = this->getOffsetByIndex(key,startTime,endTime);
            left_list = op_now == "||"?this->join(left_list,right_list):this->intersect(left_list,right_list);

            continue;
        }
    }
    // for(auto id:left_list.front().packetList){
    //     cout<<id<<" ";
    // }
    // cout<<endl;
    this->outputPacketToNewFile(outputFileName,left_list);
}