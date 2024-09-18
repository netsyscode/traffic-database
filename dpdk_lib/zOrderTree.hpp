#ifndef Z_ORDER_TREE_HPP_
#define Z_ORDER_TREE_HPP_

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <list>
#include <string.h>
#include "util.hpp"

#define PREFIX_LEN 2
#define DIM 2
#define BOARD_NUM (1 << DIM)
#define PREFIX_TOTAL_LEN (PREFIX_LEN*PREFIX_LEN)
#define BIT_COUNT (1 << PREFIX_TOTAL_LEN)
#define PREFIX_MASK ((1 << PREFIX_TOTAL_LEN) - 1)

class ZOrderIPv4{
private:
    uint32_t interleaveBits(uint16_t x) {
        uint32_t result = 0;
        for (int i = 0; i < sizeof(u_int16_t)*8; ++i) {
            result |= (((u_int32_t)x & (1u << i)) >> i) << (2 * i);
        }
        return result;
    }
    uint64_t interleaveBits(uint32_t x) {
        uint64_t result = 0;
        for (int i = 0; i < sizeof(u_int32_t)*8; ++i) {
            result |= (((u_int64_t)x & (1u << i)) >> i) << (2 * i);
        }
        return result;
    }
    uint32_t computeZOrder(uint16_t x, uint16_t y) {
        uint32_t zOrder = 0;
        uint32_t interleavedX = interleaveBits(x);
        uint32_t interleavedY = interleaveBits(y);

        zOrder = (interleavedX << 1) | interleavedY;  // 交错组合 x 和 y
        return zOrder;
    }
    uint64_t computeZOrder(uint32_t x, uint32_t y) {
        uint64_t zOrder = 0;
        uint64_t interleavedX = interleaveBits(x);
        uint64_t interleavedY = interleaveBits(y);

        zOrder = (interleavedX << 1) | interleavedY;  // 交错组合 x 和 y
        return zOrder;
    }
public:
    u_int32_t low;
    u_int32_t mid;
    u_int32_t high;
    ZOrderIPv4(){}
    ZOrderIPv4(IndexTMP* index){
        this->low = this->computeZOrder(index->meta.sourcePort,index->meta.destinationPort);
        u_int64_t tmp = this->computeZOrder(*(u_int32_t*)(index->meta.sourceAddress.c_str()), *(u_int32_t*)(index->meta.destinationAddress.c_str()));
        this->mid = tmp & 0xffffffff;
        this->high = tmp >> sizeof(this->mid)*8;
    }
    // u_int8_t getTail(){
    //     u_int8_t ret = this->low & PREFIX_MASK;
    //     this->low >>= PREFIX_TOTAL_LEN;
    //     u_int32_t tmp = (this->high & PREFIX_MASK) << (sizeof(low)*8 - PREFIX_TOTAL_LEN);
    //     this->low |= tmp;
    //     this->high >>= PREFIX_TOTAL_LEN;
    //     return ret;
    // }
    u_int8_t getSuffix(u_int8_t id){
        if(id < sizeof(low) * 8 / PREFIX_TOTAL_LEN){
            return (this->low >> (id*PREFIX_TOTAL_LEN)) & PREFIX_MASK;
        }
        if(id < (sizeof(low) + sizeof(mid)) * 8 / PREFIX_TOTAL_LEN){
            return (this->mid >> ((id - (sizeof(low) * 8 / PREFIX_TOTAL_LEN))*PREFIX_TOTAL_LEN)) & PREFIX_MASK;
        }
        if(id >= sizeof(ZOrderIPv4) * 8 / PREFIX_TOTAL_LEN){
            return 0;
        }
        return (this->high >> ((id - ((sizeof(low) + sizeof(mid)) * 8 / PREFIX_TOTAL_LEN))*PREFIX_TOTAL_LEN)) & PREFIX_MASK;
    }
    bool prefixCompare(const ZOrderIPv4& other, u_int8_t id){
        if(id < sizeof(low) * 8 / PREFIX_TOTAL_LEN){
            u_int32_t mask = 1<< (id*PREFIX_TOTAL_LEN);
            return this->high == other.high && this->mid == other.mid && (this->low ^ other.low) < mask;
        }
        if(id < (sizeof(low) + sizeof(mid)) * 8 / PREFIX_TOTAL_LEN){
            u_int32_t mask = 1<< ((id - (sizeof(low) * 8 / PREFIX_TOTAL_LEN))*PREFIX_TOTAL_LEN);
            // printf("mask:%u, xor:%u\n",mask,this->mid ^ other.mid);
            return this->high == other.high && (this->mid ^ other.mid) < mask;
        }
        if(id >= sizeof(ZOrderIPv4) * 8 / PREFIX_TOTAL_LEN){
            return true;
        }
        u_int32_t mask = 1<< ((id - ((sizeof(low) + sizeof(mid)) * 8 / PREFIX_TOTAL_LEN))*PREFIX_TOTAL_LEN);
        return (this->high ^ other.high) < mask;
    }
    // 重载比较运算符 "<"
    bool operator<(const ZOrderIPv4& other) const {
        if (high < other.high) {
            return true;
        } else if (high > other.high) {
            return false;
        }
        if (mid < other.mid) {
            return true;
        } else if (mid > other.mid) {
            return false;
        }
        // 如果高位相等，比较低位
        return low < other.low;
    }
    // 重载比较运算符 ">"
    bool operator>(const ZOrderIPv4& other) const {
        if (high > other.high) {
            return true;
        } else if (high < other.high) {
            return false;
        }

        if (mid > other.mid) {
            return true;
        } else if (mid < other.mid) {
            return false;
        }
        // 如果高位相等，比较低位
        return low > other.low;
    }
    // 重载比较运算符 "=="
    bool operator==(const ZOrderIPv4& other) const {
        return high == other.high && mid == other.mid && low == other.low;
    }
    // 重载比较运算符 "!="
    bool operator!=(const ZOrderIPv4& other) const {
        return !(*this == other);
    }
    // 重载比较运算符 "<="
    bool operator<=(const ZOrderIPv4& other) const {
        return !(*this > other);
    }
    // 重载比较运算符 ">="
    bool operator>=(const ZOrderIPv4& other) const {
        return !(*this < other);
    }
};

#pragma pack(1)
struct ZOrderIPv4Meta{
    ZOrderIPv4 zorder;
    u_int64_t value;
};
#pragma pack()

#pragma pack(1)
class ZOrderTreeNode{
public:
    u_int8_t prefix;
    u_int32_t offsets[BIT_COUNT];
    ZOrderTreeNode(){
        memset(this->offsets,0,BIT_COUNT*sizeof(u_int32_t));
    }
};
#pragma pack()

class ZOrderTreeIPv4{
    u_int64_t buffer_size;
    u_int8_t max_level;
    std::vector<ZOrderTreeNode> buffer;
    ZOrderIPv4Meta* data;
    u_int64_t num;
    void buildTreeLevel(std::vector<u_int64_t>& left, u_int64_t begin, u_int8_t level){
        if(level > sizeof(ZOrderIPv4)*8/PREFIX_TOTAL_LEN){
            return;
        }
        if(left.size() == 0){
            return;
        }
        std::vector<u_int64_t> new_left = std::vector<u_int64_t>();
        u_int64_t now_offset = this->buffer.size();
        u_int32_t j = 0;
        for(u_int32_t i = 0; i<left.size(); ++i){
            u_int8_t suffix = this->data[left[i]].zorder.getSuffix(level);
            if (j==0){
                j++;
                this->buffer.push_back(ZOrderTreeNode());
                buffer[now_offset + j - 1].prefix = suffix;
                // buffer[now_offset + j - 1].offsets[this->buffer[begin].prefix] = left.size() + j - 1 - i;
                buffer[now_offset + j - 1].offsets[this->buffer[begin + i].prefix] = begin + i + 1;
                // if(level >= 23){
                //     printf("diff: %u, %u, %u: %u, %u\n",level,suffix,this->buffer[begin + i].prefix,begin + i + 1,new_left.size());
                // }
                new_left.push_back(left[i]);
                continue;
            }
            // if (buffer[now_offset + j - 1].prefix != suffix){
            if (!this->data[left[i]].zorder.prefixCompare(this->data[new_left[j-1]].zorder,level)){
                j++;
                this->buffer.push_back(ZOrderTreeNode());
                buffer[now_offset + j - 1].prefix = suffix;
                // buffer[now_offset + j - 1].offsets[this->buffer[begin].prefix] = left.size() + j - 1 - i;
                buffer[now_offset + j - 1].offsets[this->buffer[begin + i].prefix] = begin + i + 1;
                // if(level >= 23){
                //     printf("diff: %u, %u, %u: %u, %u\n",level,suffix,this->buffer[begin + i].prefix,begin + i + 1,new_left.size());
                // }
                new_left.push_back(left[i]);
                continue;
            }
            // buffer[now_offset + j - 1].offsets[this->buffer[begin].prefix] = left.size() + j - 1 - i;
            buffer[now_offset + j - 1].offsets[this->buffer[begin + i].prefix] = begin + i + 1;
            // if(level >= 23){
            //     printf("same: %u, %u, %u: %u\n",level,suffix,this->buffer[begin + i].prefix,begin + i + 1);
            // }
        }
        // printf("size: %u\n", new_left.size());
        left.clear();
        this->buffer_size = this->buffer.size();
        this->max_level = level;
        this->buildTreeLevel(new_left, now_offset, level + 1);
    }
    u_int8_t getPrefix(u_int8_t level, u_int64_t value){
        return value >> (level * PREFIX_LEN) & 0b11;
    }
    std::pair<uint8_t, uint8_t> decodeZOrder(uint8_t zOrderValue) {
        uint8_t x = 0, y = 0;

        for (int i = 0; i < 4; ++i) {
            y |= (zOrderValue & (1ULL << (2 * i))) >> i;   // 偶数位对应 y
            x |= (zOrderValue & (1ULL << (2 * i + 1))) >> (i + 1); // 奇数位对应 x
        }
        return {x, y};
    }
    bool isInRectangle(uint8_t x, uint8_t y, uint8_t* board) {
        return (board[0] <= x && x <= board[2]  && board[1] <= y && y <= board[3]);
    }
    std::list<u_int64_t> search(u_int64_t node_offset, u_int8_t level , u_int64_t* board){
        std::list<u_int64_t> ret = std::list<u_int64_t>();
        u_int8_t board_pre[BOARD_NUM];
        for(u_int8_t i = 0;i<BOARD_NUM;++i){
            board_pre[i]=this->getPrefix(level,board[i]);
            // printf("pre: %u, %lu\n",board_pre[i],board[i]);
        }
        for(u_int8_t i = 0; i < BIT_COUNT; ++i){
            // printf("node_offset: %lu.\n",node_offset);
            if(this->buffer[node_offset].offsets[i] == 0){
                // printf("level %u, pre: %u, 0\n",level,i);
                continue;
            }

            // printf("level %u, pre: %u\n",level,i);
            auto [x, y] = decodeZOrder(i);
            if(!isInRectangle(x, y, board_pre)){
                // printf("x %u, y: %u\n",x,y);
                continue;
            }
            if(level == 0){
                // printf("get one: %u\n",this->buffer[node_offset].offsets[i]-1);
                ret.push_back(this->buffer[node_offset].offsets[i]-1);
                continue;
            }
            ret.splice(ret.end(),this->search(this->buffer[node_offset].offsets[i] - 1,level -1,board));
        }
        
        return ret ;
    }
public: 
    ZOrderTreeIPv4(ZOrderIPv4Meta* _data, u_int64_t _num){
        this->buffer = std::vector<ZOrderTreeNode>();
        this->data = _data;
        this->num = _num;
        this->buffer_size = 0;
    }
    void buildTree(){
        std::vector<u_int64_t> left = std::vector<u_int64_t>();
        u_int32_t j = 0;
        for(u_int32_t i = 0; i<num; ++i){
            u_int8_t suffix = this->data[i].zorder.getSuffix(1);
            // printf("low:%u mid:%u high:%u level:1 ret:%u\n",this->data[i].zorder.low,this->data[i].zorder.mid,this->data[i].zorder.high,suffix);
            u_int8_t tail = this->data[i].zorder.getSuffix(0);
            // printf("low:%u mid:%u high:%u level:0 ret:%u\n",this->data[i].zorder.low,this->data[i].zorder.mid,this->data[i].zorder.high,tail);
            if (j==0){
                j++;
                this->buffer.push_back(ZOrderTreeNode());
                buffer[j - 1].prefix = suffix;
                buffer[j - 1].offsets[tail] = i + 1; // use 0 to distingush non-exist point
                left.push_back(i);
                continue;
            }
            // if (buffer[j - 1].prefix != suffix){
            if (!this->data[i].zorder.prefixCompare(this->data[left[j-1]].zorder,1)){
                j++;
                this->buffer.push_back(ZOrderTreeNode());
                buffer[j - 1].prefix = suffix;
                buffer[j - 1].offsets[tail] = i + 1; // use 0 to distingush non-exist point
                left.push_back(i);
                continue;
            }
            buffer[j - 1].offsets[tail] = i + 1; // use 0 to distingush non-exist point
        }
        this->buffer_size = this->buffer.size();
        this->max_level = 1;
        this->buildTreeLevel(left, 0, 2);
        this->buffer_size = this->buffer.size();
    }
    std::list<u_int64_t> search(u_int64_t x_min, u_int64_t y_min, u_int64_t x_max, u_int64_t y_max){
        if(this->buffer_size == 0){
            return std::list<u_int64_t>();
        }
        u_int64_t board[BOARD_NUM] = {
            x_min,
            y_min,
            x_max,
            y_max,
        };
        return this->search(this->buffer_size - 1, this->max_level - 1, board);
    }
    const char* outputToChar(){
        return reinterpret_cast<const char*>(this->buffer.data());
    }
    u_int64_t getBufferSize()const{
        return this->buffer_size;
    }
    u_int8_t getMaxLevel()const{
        return this->max_level;
    }
};

#endif