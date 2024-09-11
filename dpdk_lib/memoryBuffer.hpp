#ifndef MEMORYBUFFER_HPP_
#define MEMORYBUFFER_HPP_
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <atomic>
#include <libaio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "util.hpp"

#define PAGE_SIZE 1024

class MemoryBuffer{
private:
    const u_int32_t total_block_num; // total number of blocks
    const u_int64_t block_size; // size of each block

    /*file to write*/
    u_int32_t fileID;
    int fileFD;
    std::string fileName;
    u_int64_t fileOffset;

    char** buffer_blocks; // blocks
    bool* block_flags; // true for dirty blocks
    u_int32_t block_id; // now written block id
    u_int64_t offset; // now written block offset

    void changeBlock(){
        while(this->block_flags[this->block_id]);
        this->block_flags[this->block_id] = true;
        this->block_id++;
        this->block_id %= this->total_block_num;
        this->offset = 0;
        printf("MemoryBuffer log: change block id to %u.\n",block_id);
    }
    void changeFileOffset(){
        this->fileOffset += this->block_size;
    }
public:
    MemoryBuffer(u_int32_t id, u_int64_t capacity, u_int32_t block_num, std::string file_name, u_int64_t offset = 0):total_block_num(block_num),block_size(capacity){
        if(capacity % PAGE_SIZE){
            printf("MemoryBuffer error: capacity should be aligned to pages(%d)!\n",PAGE_SIZE);
            return;
        }

        this->fileID = id;
        this->fileName = file_name;
        this->fileFD = open(this->fileName.c_str(), O_WRONLY | O_CREAT | O_DIRECT, 0644);
        if (this->fileFD < 0) {
            printf("MemoryBuffer error: fail to open file!\n");
            return;
        }
        this->fileOffset = 0;

        this->buffer_blocks = new char*[block_num];
        this->block_flags = new bool[block_num]();
        this->block_id = 0;
        this->offset = offset;
        for (u_int32_t i=0; i<this->total_block_num; ++i){
            if (posix_memalign(reinterpret_cast<void**>(&(this->buffer_blocks[i])), block_size, block_size)) {
                printf("MemoryBuffer error: failed to allocate aligned memory!\n");
                return;
            }
            this->block_flags[i] = false;
        }
    }
    ~MemoryBuffer(){
        for(u_int32_t i=0; i<this->total_block_num; ++i){
            if(this->block_flags[i]){
                printf("MemoryBuffer warning: block %u not write yet.\n",i);
            }
            free(this->buffer_blocks[i]);
        }
        close(this->fileFD);
    }
    void writePointer(const char* data, u_int32_t len){
        if(this->offset + len > this->block_size){
            u_int32_t tmp = this->block_size - this->offset;
            memcpy(this->buffer_blocks[this->block_id] + this->offset, data, tmp);
            this->changeBlock();
            memcpy(this->buffer_blocks[this->block_id] + this->offset, data, len - tmp);
            this->offset = len - tmp;
            return;
        }
        memcpy(this->buffer_blocks[this->block_id] + this->offset, data, len);
        this->offset += len;
    }
    bool checkAndWriteFile(u_int32_t id){
        // if(id == this->block_id){
        //     return false;
        // }
        if(!this->block_flags[id]){
            return false;
        }
        ssize_t bytes_written = pwrite(this->fileFD, this->buffer_blocks[id], this->block_size, this->fileOffset);
        if (bytes_written == -1) {
            printf("MemoryBuffer error: failed to pwrite!\n");
        }
        this->block_flags[id] = false;
        printf("MemoryBuffer log: write id %u.\n",id);
        return true;
    }
    void directWriteFile(u_int32_t id){
        if(id != this->block_id){
            printf("MemoryBuffer error: write id %u is not final id!\n",id);
        }
        ssize_t bytes_written = pwrite(this->fileFD, this->buffer_blocks[id], this->block_size, this->fileOffset);
        if (bytes_written == -1) {
            printf("MemoryBuffer error: failed to pwrite!\n");
        }
        printf("MemoryBuffer log: write id %u.\n",id);
    }
    std::string getFileName()const{
        return this->fileName;
    }
    u_int64_t getOffset()const{
        return this->offset;
    }
    u_int32_t getBlockID()const{
        return this->block_id;
    }
    u_int32_t getFileID()const{
        return this->fileID;
    }
    u_int64_t getNum()const{
        return this->total_block_num;
    }
    u_int64_t getSize()const{
        return this->block_size;
    }
};


#endif