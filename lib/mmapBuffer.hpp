#ifndef MMAPBUFFER_HPP_
#define MMAPBUFFER_HPP_
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "util.hpp"

class MmapBuffer{
    char* buffer;
    std::string fileName;
    int fileFD;
    u_int64_t fileSize;
public:
    MmapBuffer(){
        this->buffer = nullptr;
        this->fileFD = -1;
        this->fileSize = 0;
        this->fileName = std::string();
    }
    ~MmapBuffer(){
        if(this->fileFD != -1){
            printf("Mmap buffer warning: destroy with open file.\n");
            munmap(this->buffer, this->fileSize);
            close(this->fileFD);
        }
    }
    void changeFileName(std::string file_name){
        this->fileName = file_name;
    }
    bool openFile(){
        if(this->fileFD != -1){
            printf("Mmap buffer error: openFile with open file!\n");
            return false;
        }
        if(this->fileName.size()==0){
            printf("Mmap buffer error: openFile without file name!\n");
            return false;
        }
        this->fileFD = open(this->fileName.c_str(), O_RDONLY);
        if(this->fileFD == -1){
            printf("Mmap buffer error: openFile failed while open!\n");
            return false;
        }
        struct stat fileStat;
        if (fstat(this->fileFD, &fileStat) == -1) {
            printf("Mmap buffer error: openFile failed while fstat!\n");
            close(this->fileFD);
            return false;
        }
        this->fileSize = fileStat.st_size;
        this->buffer = static_cast<char*>(mmap(nullptr, this->fileSize, PROT_READ, MAP_PRIVATE, this->fileFD, 0));
        if (this->buffer == MAP_FAILED) {
            printf("Mmap buffer error: openFile failed while mmap!\n");
            close(this->fileFD);
            return false;
        }
        return true;
    }
    void closeFile(){
        if(this->fileFD == -1){
            printf("Mmap buffer warning: closeFile without open file.\n");
            return;
        }
        if(munmap(this->buffer, this->fileSize) == -1) {
            printf("Mmap buffer warning: closeFile failed while munmap.\n");
        }
        if(close(this->fileFD)==-1){
            printf("Mmap buffer warning: closeFile failed while close.\n");
        }
        this->fileFD = -1;
        this->buffer = nullptr;
        this->fileSize = 0;
    }
    char* getPointer(u_int64_t pos) const{
        if(pos >= this->fileSize){
            return nullptr;
        }
        return this->buffer+pos;
    }
    std::string getFileName(){
        return this->fileName;
    }
};

#endif