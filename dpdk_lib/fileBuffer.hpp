#ifndef FILEBUFFER_HPP_
#define FILEBUFFER_HPP_
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "util.hpp"

class FileBuffer{
    char* buffer;
    std::string fileName;
    u_int32_t fileID;
    int fileFD;
    u_int64_t offset; // offset of buffer
    u_int64_t fileCapacity; // max length of buffer;
    u_int64_t length; // length of buffer
    u_int64_t ts_first;
    u_int64_t ts_last;
public:
    FileBuffer(u_int32_t id, u_int64_t capacity, u_int64_t offset = 0){
        this->buffer = nullptr;
        this->fileID = id;
        this->fileFD = -1;
        this->offset = offset;
        this->fileCapacity = capacity;
        if(capacity % 1024){
            printf("File buffer warning: capacity should be aligned to pages(1024)\n");
        }
        this->fileName = std::string();
        this->length = 0;
        this->ts_first = 0;
        this->ts_last = 0;
    }
    ~FileBuffer(){
        if(this->fileFD != -1){
            printf("File buffer warning: destroy with open file.\n");
            munmap(this->buffer, this->fileCapacity);
            close(this->fileFD);
        }
    }
    void changeFileName(std::string file_name){
        this->fileName = file_name;
    }
    void changeFileID(u_int32_t id){
        this->fileID = id;
    }
    void changeFileOffset(u_int32_t offset_){
        this->offset = offset_;
    }
    void changeFileLength(u_int32_t len){
        this->length = len;
    }
    bool openFile(){
        if(this->fileFD != -1){
            printf("File buffer error: openFile with opened file!\n");
            return false;
        }
        if(this->fileName.size()==0){
            printf("File buffer error: openFile without file name!\n");
            return false;
        }
        this->fileFD = open(this->fileName.c_str(), O_RDWR);
        if(this->fileFD == -1){
            printf("File buffer error: openFile failed while open!\n");
            return false;
        }
        // struct stat fileStat;
        // if (fstat(this->fileFD, &fileStat) == -1) {
        //     printf("File buffer error: openFile failed while fstat!\n");
        //     close(this->fileFD);
        //     return false;
        // }
        if (fallocate(this->fileFD, 0, this->offset, this->fileCapacity) == -1) {
            printf("File buffer error: openFile failed while fallocate!\n");
            close(this->fileFD);
            return false;
        }
        this->buffer = static_cast<char*>(mmap(nullptr, this->fileCapacity, PROT_READ | PROT_WRITE, MAP_SHARED, this->fileFD, this->offset));
        if (this->buffer == MAP_FAILED) {
            printf("File buffer error: openFile failed while mmap!\n");
            close(this->fileFD);
            return false;
        }
        this->length = 0;
        return true;
    }
    void closeFile(){
        if(this->fileFD == -1){
            printf("File buffer warning: closeFile without open file.\n");
            return;
        }
        if(munmap(this->buffer, this->fileCapacity) == -1) {
            printf("File buffer warning: closeFile failed while munmap.\n");
        }
        if(close(this->fileFD)==-1){
            printf("File buffer warning: closeFile failed while close.\n");
        }
        this->fileFD = -1;
        this->buffer = nullptr;
        this->fileCapacity = 0;
        this->length = 0;
    }
    bool expandFile(){
        if(this->fileFD == -1){
            printf("File buffer warning: expandFile without open file.\n");
            return false;
        }
  
        if(munmap(this->buffer, this->fileCapacity) == -1) {
            printf("File buffer error: expandFile failed while munmap!\n");
            return false;
        }

        // this->offset = (this->length/1024 + 1)*1024 + this->offset;

        this->offset = this->fileCapacity + this->offset;
        if (fallocate(this->fileFD, 0, this->offset, this->fileCapacity) == -1) {
            printf("File buffer error: expandFile failed while fallocate!\n");
            close(this->fileFD);
            return false;
        }
        this->buffer = static_cast<char*>(mmap(nullptr, this->fileCapacity, PROT_READ | PROT_WRITE, MAP_SHARED, this->fileFD, this->offset));
        if (this->buffer == MAP_FAILED) {
            printf("File buffer error: expandFile failed while mmap!\n");
            close(this->fileFD);
            return false;
        }
        this->length = 0;
        printf("File buffer log: expand succeed, offset is %llu.\n",this->offset);
        return true;
    }
    char* getPointer(u_int64_t pos)const{ // no use?
        if(pos >= this->offset + this->length){
            return nullptr;
        }
        return this->buffer + (pos-this->offset);
    }
    bool writePointer(char* data, u_int32_t len){
        if(this->length + len > this->fileCapacity){
            u_int32_t tmp = this->fileCapacity - this->length;
            memcpy(this->buffer + this->length, data, tmp);
            if(!this->expandFile()){
                return false;
            }
            memcpy(this->buffer + this->length, data, len - tmp);
            this->length = len - tmp;
            return true;
        }
        memcpy(this->buffer + this->length,data,len);
        this->length += len;
        return true;
    }
    std::string getFileName(){
        return this->fileName;
    }
    u_int64_t getOffset()const{
        return this->offset;
    }
    u_int64_t getLength()const{
        return this->length;
    }
    u_int32_t getFileID()const{
        return this->fileID;
    }
    // bool checkPos(u_int64_t pos){
    //     if(this->lastWriteSize < pos){
    //         this->lastWriteSize = pos;
    //         return true;
    //     }
    //     return false;
    // }
    void updateTS(u_int64_t ts){
        if(ts < this->ts_first || this->ts_first == 0){
            this->ts_first = ts;
        }
        if(ts > this->ts_last || this->ts_last == 0){
            this->ts_last = ts;
        }
    }
};

#endif