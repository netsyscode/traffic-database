#include "storage.hpp"
#include <chrono>
#define ADDRESS_TYPE u_int32_t
#define PORT_TYPE u_int16_t
#define TIME_TYPE u_int64_t

void intersect(list<u_int32_t>& la, list<u_int32_t>& lb){
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

template <typename KeyType>
list<u_int32_t> binarySearch(char* arr, u_int32_t len, KeyType target){
    u_int32_t ele_size = sizeof(KeyType)+sizeof(u_int32_t);
    u_int32_t arr_len = len/ele_size;
    list<u_int32_t> res = list<u_int32_t>();
    int left = 0;
    int right = arr_len - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (*(KeyType*)(arr + mid * ele_size) == target) {
            res.push_back(*(u_int32_t*)(arr + mid * ele_size + sizeof(KeyType)));
            int i = mid - 1;
            while (i >= 0 && *(KeyType*)(arr + i * ele_size) == target) {
                res.push_back(*(u_int32_t*)(arr + i * ele_size + sizeof(KeyType)));
                i--;
            }
            int j = mid + 1;
            while (j < arr_len && *(KeyType*)(arr + j * ele_size) == target) {
                res.push_back(*(u_int32_t*)(arr + j * ele_size + sizeof(KeyType)));
                j++;
            }
            return res;
        } else if (*(KeyType*)(arr + mid * ele_size) < target) {
            left = mid + 1; // 目标值在右侧，更新左边界
        } else {
            right = mid - 1; // 目标值在左侧，更新右边界
        }
    }

    return res;
}

template <typename KeyType>
list<u_int32_t> binarySearchRange(char* arr, u_int32_t len, KeyType begin, KeyType end){
    u_int32_t ele_size = sizeof(KeyType)+sizeof(u_int32_t);
    u_int32_t arr_len = len/ele_size;
    list<u_int32_t> res = list<u_int32_t>();
    int left = 0;
    int right = arr_len - 1;
    if(begin > end){
        return res;
    }

    while (left <= right) {
        int mid = left + (right - left) / 2;

        //cout << begin << " " << end << " " << *(KeyType*)(arr + mid * ele_size) << endl;

        if (*(KeyType*)(arr + mid * ele_size) >= begin && *(KeyType*)(arr + mid * ele_size) < end) {
            res.push_back(*(u_int32_t*)(arr + mid * ele_size + sizeof(KeyType)));
            int i = mid - 1;
            while (i >= 0 && *(KeyType*)(arr + i * ele_size) >= begin) {
                res.push_back(*(u_int32_t*)(arr + i * ele_size + sizeof(KeyType)));
                i--;
            }
            int j = mid + 1;
            while (j < arr_len && *(KeyType*)(arr + j * ele_size) < end) {
                res.push_back(*(u_int32_t*)(arr + j * ele_size + sizeof(KeyType)));
                j++;
            }
            // for(auto x:res){
            //     printf("%u ",x);
            // }
            // printf("\n");
            return res;
        } else if (*(KeyType*)(arr + mid * ele_size) < begin) {
            left = mid + 1; // 目标值在右侧，更新左边界
        } else {
            right = mid - 1; // 目标值在左侧，更新右边界
        }
    }

    return res;
}

void StorageOperator::changeFile(string filename){
    this->filename = filename;
    this->fileOffset = 0;
}

bool StorageOperator::writeIndex(char* buffer){
    u_int32_t total_length = *(u_int32_t*)buffer;

    ofstream outFile(this->filename);
    if (!outFile.is_open()) {
        printf("Open Failed!");
        return false;
    }

    outFile.seekp(0, ios::end);
    outFile.write(buffer, total_length);
    outFile.close();
    delete[] buffer;
    return true;
}

bool StorageOperator::writeIndex(char* buffer, u_int64_t startTime, u_int64_t endTime){
    u_int32_t total_length = *(u_int32_t*)buffer;
    StorageMetadata metadata = {
        .start_time = startTime,
        .end_time = endTime,
        .filename = filename,
        .offset = this->fileOffset,
    };

    ofstream outFile(this->filename);
    if (!outFile.is_open()) {
        printf("Open Failed!");
        return false;
    }

    outFile.seekp(0, ios::end);
    outFile.write(buffer, total_length);
    outFile.close();
    delete[] buffer;

    this->fileOffset += total_length;
    this->storageList.push_back(metadata);
    return true;
}

char* StorageOperator::readIndex(string filename, u_int32_t offset){
    ifstream inFile(filename);
    if (!inFile.is_open()) {
        printf("Open Failed!");
        return nullptr;
    }

    inFile.seekg(offset,ios::beg);

    u_int32_t total_length;
    inFile.read((char*)&total_length,sizeof(u_int32_t));
    if(total_length > 1e8){
        printf("Buffer Too Large!");
        return nullptr;
    }

    char* buffer = new char[total_length];

    inFile.seekg(offset,ios::beg);
    inFile.read(buffer,total_length);

    return buffer;
}

IndexReturn StorageOperator::getPacketOffsetByIndex(struct FlowIndex& index, string filename, u_int32_t offset){
    IndexReturn res = {
        .filename = "",
        .packetList = list<u_int32_t>(),
    };

    ifstream inFile(filename);
    if (!inFile.is_open()) {
        printf("Open Failed!");
        return res;
    }

    u_int32_t offset_now = offset;
    u_int64_t key[INDEX_NUM];

    // key[0]=index.startTime;
    // key[1]=index.endTime;
    key[0]=ntohl(index.sourceAddress.s_addr);
    key[1]=ntohl(index.destinationAddress.s_addr);
    key[2]=index.sourcePort;
    key[3]=index.destinationPort;

    inFile.seekg(offset_now,ios::beg);
    u_int32_t total_length;
    inFile.read((char*)&total_length,sizeof(u_int32_t));
    u_int32_t index_offsets[INDEX_NUM];
    for(int i=0;i<INDEX_NUM;++i){
        inFile.read((char*)&index_offsets[i],sizeof(u_int32_t));
    }
    u_int32_t data_offset;
    inFile.read((char*)&data_offset,sizeof(u_int32_t));
    u_int32_t filename_len = index_offsets[0] - META_LEN;

    res.filename = string(filename_len,'\0');
    inFile.read(&(res.filename[0]), filename_len);

    for(int i=0;i<INDEX_NUM;++i){
        index_offsets[i] += offset;
    }
    data_offset += offset;

    u_int32_t index_length = 0;
    char* buffer;
    list<u_int32_t> idx;
    bool has_one = false;

    auto start = chrono::high_resolution_clock::now();
    auto end = chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    for(int i=0;i<INDEX_NUM; i++){
        //printf("key%d:%llu\n",i,key[i]);
        //printf("index_len:%u\n",index_length);

        // if(i==0){
        //     idx = binarySearchRange<u_int64_t>(buffer,index_length,key[0],key[1]);
        //     idx.sort();
        // }else{
        //     list<u_int32_t> tmp;
        //     if(i==1){
        //         tmp = binarySearchRange<u_int64_t>(buffer,index_length,key[0],key[1]);
        //     }else if(i==2 || i==3 && ((index.flags >> (i-2)) & 1)){ 
        //         tmp = binarySearch<u_int32_t>(buffer,index_length,(u_int32_t)key[i]);
        //     }else if((index.flags >> (i-2)) & 1){
        //         tmp = binarySearch<u_int16_t>(buffer,index_length,(u_int16_t)key[i]);
        //     }
        //     tmp.sort();
        //     intersect(idx,tmp);
        // }

        if(((index.flags >> i) & 1)){
            inFile.seekg(index_offsets[i],ios::beg);
            inFile.read((char*)&index_length,sizeof(u_int32_t));
            buffer = new char[index_length];
            inFile.read(buffer,index_length);
            start = chrono::high_resolution_clock::now();
            list<u_int32_t> tmp;
            if (i==0 | i==1){
                tmp = binarySearch<u_int32_t>(buffer,index_length,(u_int32_t)key[i]);
            }else{
                tmp = binarySearch<u_int16_t>(buffer,index_length,(u_int16_t)key[i]);
            }
            tmp.sort();
            if(!has_one){
                idx = tmp;
                has_one = true;
            }else{
                intersect(idx,tmp);
            }
            end = chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            cout << "Index search time: " << duration.count() << " us" << endl;
            delete[] buffer;
        }
        
        // index_offset += index_length + sizeof(u_int32_t);
        //printf("i:%d, idx_len:%lu\n",i,idx.size());
    }

    if(!has_one){
        return res;
    }

    for(auto id:idx){
        u_int32_t data_length;
        inFile.seekg(data_offset+id,ios::beg);
        inFile.read((char*)&data_length,sizeof(u_int32_t));
        buffer = new char[data_length * sizeof(u_int32_t)];
        inFile.read(buffer,data_length * sizeof(u_int32_t));
        res.packetList.insert(res.packetList.begin(),(u_int32_t*)buffer,((u_int32_t*)buffer) + data_length);
        delete[] buffer;
    }
    return res;
}

IndexReturn StorageOperator::getPacketOffsetByRange(struct FlowIndex& startIndex, struct FlowIndex& endIndex, string filename, u_int32_t offset){
    IndexReturn res = {
        .filename = "",
        .packetList = list<u_int32_t>(),
    };

    if(startIndex.flags != endIndex.flags || startIndex.startTime != endIndex.startTime || startIndex.endTime != endIndex.endTime){
        return res;
    }

    ifstream inFile(filename);
    if (!inFile.is_open()) {
        printf("Open Failed!");
        return res;
    }

    u_int32_t offset_now = offset;
    u_int64_t startKey[INDEX_NUM], endKey[INDEX_NUM];

    // startKey[0]=startIndex.startTime;
    // startKey[1]=startIndex.endTime;
    startKey[0]=ntohl(startIndex.sourceAddress.s_addr);
    startKey[1]=ntohl(startIndex.destinationAddress.s_addr);
    startKey[2]=startIndex.sourcePort;
    startKey[3]=startIndex.destinationPort;

    // endKey[0]=endIndex.startTime;
    // endKey[1]=endIndex.endTime;
    endKey[0]=ntohl(endIndex.sourceAddress.s_addr);
    endKey[1]=ntohl(endIndex.destinationAddress.s_addr);
    endKey[2]=endIndex.sourcePort;
    endKey[3]=endIndex.destinationPort;

    inFile.seekg(offset_now,ios::beg);
    u_int32_t total_length;
    inFile.read((char*)&total_length,sizeof(u_int32_t));
    u_int32_t index_offsets[INDEX_NUM];
    for(int i=0;i<INDEX_NUM;++i){
        inFile.read((char*)&index_offsets[i],sizeof(u_int32_t));
    }
    u_int32_t data_offset;
    inFile.read((char*)&data_offset,sizeof(u_int32_t));
    u_int32_t filename_len = index_offsets[0] - META_LEN;

    res.filename = string(filename_len,'\0');
    inFile.read(&(res.filename[0]), filename_len);

    for(int i=0;i<INDEX_NUM;++i){
        index_offsets[i] += offset;
    }
    data_offset += offset;

    u_int32_t index_length = 0;
    char* buffer;
    list<u_int32_t> idx;
    bool has_one = false;

    for(int i=0;i<INDEX_NUM; i++){
        //printf("key%d:%llu\n",i,key[i]);
        //printf("index_len:%u\n",index_length);


        // if(i==0){
        //     idx = binarySearchRange<u_int64_t>(buffer,index_length,startKey[0],startKey[1]);
        //     idx.sort();
        // }else{
        //     list<u_int32_t> tmp;
        //     if(i==1){
        //         tmp = binarySearchRange<u_int64_t>(buffer,index_length,startKey[0],startKey[1]);
        //     }else if(i==2 || i==3 && ((startIndex.flags >> (i-2)) & 1)){
        //         tmp = binarySearchRange<u_int32_t>(buffer,index_length,(u_int32_t)startKey[i],(u_int32_t)endKey[i]);
        //     }else if((startIndex.flags >> (i-2)) & 1){
        //         tmp = binarySearchRange<u_int16_t>(buffer,index_length,(u_int16_t)startKey[i],(u_int16_t)endKey[i]);
        //     }
        //     tmp.sort();
        //     intersect(idx,tmp);
        // }

        if(((startIndex.flags >> i) & 1)){
            inFile.seekg(index_offsets[i],ios::beg);
            inFile.read((char*)&index_length,sizeof(u_int32_t));
            buffer = new char[index_length];
            inFile.read(buffer,index_length);
            list<u_int32_t> tmp;
            if (i==0 || i==1){
                tmp = binarySearchRange<u_int32_t>(buffer,index_length,(u_int32_t)startKey[i],(u_int32_t)endKey[i]);
            }else{
                tmp = binarySearchRange<u_int16_t>(buffer,index_length,(u_int16_t)startKey[i],(u_int16_t)endKey[i]);
            }
            tmp.sort();
            if(!has_one){
                idx = tmp;
                has_one = true;
            }else{
                intersect(idx,tmp);
            }
            delete[] buffer;
        }


        // index_offset += index_length + sizeof(u_int32_t);
        //printf("i:%d, idx_len:%lu\n",i,idx.size());
    }

    if(!has_one){
        return res;
    }

    for(auto id:idx){
        u_int32_t data_length;
        inFile.seekg(data_offset+id,ios::beg);
        inFile.read((char*)&data_length,sizeof(u_int32_t));
        buffer = new char[data_length * sizeof(u_int32_t)];
        inFile.read(buffer,data_length * sizeof(u_int32_t));
        res.packetList.insert(res.packetList.begin(),(u_int32_t*)buffer,((u_int32_t*)buffer) + data_length);
        delete[] buffer;
    }
    return res;
}

IndexReturn StorageOperator::getPacketOffsetByIndex(KeyMode mode, u_int64_t key, string filename, u_int32_t offset){
    IndexReturn res = {
        .filename = "",
        .packetList = list<u_int32_t>(),
    };

    ifstream inFile(filename);
    if (!inFile.is_open()) {
        printf("Open Failed!");
        return res;
    }

    inFile.seekg(offset,ios::beg);
    u_int32_t total_length;
    inFile.read((char*)&total_length,sizeof(u_int32_t));
    u_int32_t index_offsets[INDEX_NUM];
    for(int i=0;i<INDEX_NUM;++i){
        inFile.read((char*)&index_offsets[i],sizeof(u_int32_t));
    }
    u_int32_t data_offset;
    inFile.read((char*)&data_offset,sizeof(u_int32_t));
    u_int32_t filename_len = index_offsets[0] - META_LEN;

    res.filename = string(filename_len,'\0');
    inFile.read(&(res.filename[0]), filename_len);

    for(int i=0;i<INDEX_NUM;++i){
        index_offsets[i] += offset;
    }
    data_offset += offset;


    u_int32_t offset_now = offset;
    u_int32_t index_length = 0;
    

    inFile.seekg(index_offsets[mode],ios::beg);
    inFile.read((char*)&index_length,sizeof(u_int32_t));

    char* buffer = new char[index_length];
    inFile.read(buffer,index_length);

    list<u_int32_t> idx;
    if(mode == KeyMode::SRCIP || mode == KeyMode::DSTIP){
        idx = binarySearch<u_int32_t>(buffer,index_length,(u_int32_t)key);
    }
    else{
        idx = binarySearch<u_int16_t>(buffer,index_length,(u_int16_t)key);
    }
    idx.sort();
    delete[] buffer;

    for(auto id:idx){
        u_int32_t data_length;
        inFile.seekg(data_offset+id,ios::beg);
        inFile.read((char*)&data_length,sizeof(u_int32_t));
        buffer = new char[data_length * sizeof(u_int32_t)];
        inFile.read(buffer,data_length * sizeof(u_int32_t));
        res.packetList.insert(res.packetList.begin(),(u_int32_t*)buffer,((u_int32_t*)buffer) + data_length);
        delete[] buffer;
    }
    return res;
}

IndexReturn StorageOperator::getPacketOffsetByRange(KeyMode mode, u_int64_t startKey, u_int64_t endKey, string filename, u_int32_t offset){
    IndexReturn res = {
        .filename = "",
        .packetList = list<u_int32_t>(),
    };

    ifstream inFile(filename);
    if (!inFile.is_open()) {
        printf("Open Failed!");
        return res;
    }

    inFile.seekg(offset,ios::beg);
    u_int32_t total_length;
    inFile.read((char*)&total_length,sizeof(u_int32_t));
    u_int32_t index_offsets[INDEX_NUM];
    for(int i=0;i<INDEX_NUM;++i){
        inFile.read((char*)&index_offsets[i],sizeof(u_int32_t));
    }
    u_int32_t data_offset;
    inFile.read((char*)&data_offset,sizeof(u_int32_t));
    u_int32_t filename_len = index_offsets[0] - META_LEN;

    res.filename = string(filename_len,'\0');
    inFile.read(&(res.filename[0]), filename_len);

    for(int i=0;i<INDEX_NUM;++i){
        index_offsets[i] += offset;
    }
    data_offset += offset;


    u_int32_t offset_now = offset;
    u_int32_t index_length = 0;
    

    inFile.seekg(index_offsets[mode],ios::beg);
    inFile.read((char*)&index_length,sizeof(u_int32_t));

    char* buffer = new char[index_length];
    inFile.read(buffer,index_length);

    list<u_int32_t> idx;
    if(mode == KeyMode::SRCIP || mode == KeyMode::DSTIP){
        idx = binarySearchRange<u_int32_t>(buffer,index_length,(u_int32_t)startKey, (u_int32_t)endKey);
    }
    else{
        idx = binarySearchRange<u_int16_t>(buffer,index_length,(u_int16_t)startKey, (u_int16_t)endKey);
    }
    idx.sort();
    delete[] buffer;

    for(auto id:idx){
        u_int32_t data_length;
        inFile.seekg(data_offset+id,ios::beg);
        inFile.read((char*)&data_length,sizeof(u_int32_t));
        buffer = new char[data_length * sizeof(u_int32_t)];
        inFile.read(buffer,data_length * sizeof(u_int32_t));
        res.packetList.insert(res.packetList.begin(),(u_int32_t*)buffer,((u_int32_t*)buffer) + data_length);
        delete[] buffer;
    }
    return res;
}

list<IndexReturn> StorageOperator::getPacketOffsetByIndex(struct FlowIndex& index){
    list<IndexReturn> res = list<IndexReturn>();
    for(auto meta:this->storageList){
        if(meta.start_time < index.endTime && meta.end_time > index.startTime){
            res.push_back(this->getPacketOffsetByIndex(index,meta.filename,meta.offset));
        }
    }
    return res;
}

list<IndexReturn> StorageOperator::getPacketOffsetByRange(struct FlowIndex& startIndex, struct FlowIndex& endIndex){
    list<IndexReturn> res = list<IndexReturn>();
    if(startIndex.flags != endIndex.flags || startIndex.startTime != endIndex.startTime || startIndex.endTime != endIndex.endTime){
        return res;
    }
    for(auto meta:this->storageList){
        if(meta.start_time < startIndex.endTime && meta.end_time > startIndex.startTime){
            res.push_back(this->getPacketOffsetByRange(startIndex,endIndex,meta.filename,meta.offset));
        }
    }
    return res;
}

list<IndexReturn> StorageOperator::getPacketOffsetByIndex(KeyMode mode, u_int64_t key, u_int64_t startTime, u_int64_t endTime){
    list<IndexReturn> res = list<IndexReturn>();
    for(auto meta:this->storageList){
        if(meta.start_time < endTime && meta.end_time > startTime){
            res.push_back(getPacketOffsetByIndex(mode,key,meta.filename,meta.offset));
        }
    }
    return res;
}

list<IndexReturn> StorageOperator::getPacketOffsetByRange(KeyMode mode, u_int64_t startKey, u_int64_t endKey, u_int64_t startTime, u_int64_t endTime){
    list<IndexReturn> res = list<IndexReturn>();
    for(auto meta:this->storageList){
        if(meta.start_time < endTime && meta.end_time > startTime){
            res.push_back(getPacketOffsetByRange(mode,startKey,endKey,meta.filename,meta.offset));
        }
    }
    return res;
}