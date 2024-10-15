#ifndef BLOOMFILTER_HPP_
#define BLOOMFILTER_HPP_
#include <iostream>
#include <vector>
#include <functional>
#include <bitset>
#define HASH_SEED 0x5bd1e995

class BloomFilter {
public:
    std::string bitArray; // 使用 uint8_t 作为位数组
    BloomFilter(size_t size, size_t numHashFunctions) 
        : bitArray(size,0), k(numHashFunctions) {}

    BloomFilter(char* data, u_int32_t len, size_t numHashFunctions){
        this->bitArray = std::string(data,len);
        this->k = numHashFunctions;
    }

    // 插入元素
    void insert(const std::string& element) {
        for (size_t i = 0; i < k; ++i) {
            size_t hash = hashFunction(element, i);
            setBit(hash % (bitArray.size() * 8));
        }
    }

    // 查询元素
    bool contains(const std::string& element) const {
        for (size_t i = 0; i < k; ++i) {
            size_t hash = hashFunction(element, i);
            if (!getBit(hash % (bitArray.size() * 8))) {
                return false; // 一定不在
            }
        }
        return true; // 可能在
    }

private:
    
    size_t k; // 哈希函数的数量

    // 设置某一位为 1
    void setBit(size_t index) {
        bitArray[index / 8] |= (1 << (index % 8));
    }

    // 获取某一位的值
    bool getBit(size_t index) const {
        return (bitArray[index / 8] & (1 << (index % 8))) != 0;
    }

    // 简单的哈希函数
    size_t hashFunction(const std::string& element, size_t i) const {
        std::hash<std::string> hasher;
        return hasher(element) + i * 0x5bd1e995; // 使用不同的种子
    }
};
#endif