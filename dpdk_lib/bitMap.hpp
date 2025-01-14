#ifndef BITMAP_HPP_
#define BITMAP_HPP_
#include <iostream>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <algorithm>
#include <climits>
#include <list>

// 使用位图来表示索引
class BitmapIndex {
private:
    // 最大记录数，这可以是数据表的记录数
    const int max_records = 1024*1024;

    // 存储每个值的位图
    // std::unordered_map<int, std::vector<bool>> bitmap_map;

    // 存储压缩后的位图
    std::unordered_map<std::string, std::vector<std::pair<u_int32_t, u_int32_t>>> compressed_map;

    std::list<u_int64_t> values;

    // 运行长度编码（RLE）实现
    // std::vector<std::pair<int, int>> runLengthEncode(const std::vector<bool>& bitmap) {
    //     std::vector<std::pair<int, int>> compressed;
    //     int count = 1;
    //     for (size_t i = 1; i < bitmap.size(); ++i) {
    //         if (bitmap[i] == bitmap[i - 1]) {
    //             ++count;
    //         } else {
    //             compressed.push_back({bitmap[i - 1] ? 1 : 0, count});
    //             count = 1;
    //         }
    //     }
    //     // 最后一段
    //     compressed.push_back({bitmap[bitmap.size() - 1] ? 1 : 0, count});
    //     return compressed;
    // }
public:
    BitmapIndex(){
        this->compressed_map = std::unordered_map<std::string, std::vector<std::pair<u_int32_t, u_int32_t>>>();
        this->values = std::list<u_int64_t>();
    }

    // 插入操作
    void insert(int value, int recordId) {
        if (bitmap_map.find(value) == bitmap_map.end()) {
            // 如果没有该值的位图，则创建一个新的位图
            bitmap_map[value] = std::vector<bool>(max_records, false);
        }
        // 设置对应位图的位为1，表示该记录中该值存在
        bitmap_map[value][recordId] = true;
    }

    // 压缩操作 - 使用简单的运行长度编码（RLE）
    void compress() {
        for (auto &entry : bitmap_map) {
            std::vector<bool> &bitmap = entry.second;
            compressed_map[entry.first] = runLengthEncode(bitmap);
        }
    }

    // 打印未压缩的位图
    void print() {
        for (const auto &entry : bitmap_map) {
            std::cout << "Value: " << entry.first << " -> ";
            for (bool bit : entry.second) {
                std::cout << bit;
            }
            std::cout << std::endl;
        }
    }

    // 打印压缩后的位图
    void printCompressed() {
        for (const auto &entry : compressed_map) {
            std::cout << "Value: " << entry.first << " -> ";
            for (auto &run : entry.second) {
                std::cout << "[" << run.first << "," << run.second << "] ";
            }
            std::cout << std::endl;
        }
    }
};


#endif