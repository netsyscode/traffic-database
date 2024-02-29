#include "index.hpp"
#define FLAG_VARIABLE (flags >> num) & 1
#define INTERSECT tmp.sort(); intersect(res,tmp); if(res.size() == 0) return res;
#define TIMES sizeof(u_int32_t)/sizeof(char)
#define MOVE_INDEX_POINTER index_length = *((u_int32_t*)(storage_buffer + index_offset_now)) + sizeof(u_int32_t); index_offset_now += index_length;

void IndexGenerator::clear(){
    delete this->sourceAddressIndex;
    delete this->destinationAddressIndex;
    delete this->sourcePortIndex;
    delete this->destinationPortIndex;
    this->sourceAddressIndex = new SkipList<u_int32_t,u_int32_t>(ADDRESS_LEVEL);
    this->destinationAddressIndex = new SkipList<u_int32_t,u_int32_t>(ADDRESS_LEVEL);
    this->sourcePortIndex = new SkipList<u_int16_t,u_int32_t>(PORT_LEVEL);
    this->destinationPortIndex = new SkipList<u_int16_t,u_int32_t>(PORT_LEVEL);
    this->offset = 0;
    this->startTime = 0;
    this->endTime = 0;
}

void IndexGenerator::intersect(list<u_int32_t>& la, list<u_int32_t>& lb){
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
    // for(auto value: lb){
    //     while(it!=la.end() && value > *it){
    //         printf("value:%u,it:%u\n",value,*it);
    //         it = la.erase(it);
    //     }
    //     if(it==la.end()){
    //         return;
    //     }
    //     if(value < *it){
    //         continue;
    //     }
    //     if(value == *it){
    //         printf("value:%u\n",value);
    //         it++;
    //         continue;
    //     }
    // }
    // if(it==la.end()){
    //     return;
    // }
    // it++;
    // while(it!=la.end()){
    //     it = la.erase(it);
    // }
}

string IndexGenerator::getFilenName(){
    return this->filename;
}

bool IndexGenerator::putFlow(const Flow* flow){
    u_int32_t packet_num = flow->data.packet_num;
    if(this->offset + (packet_num + 1)*TIMES >= this->buffer_len){
        return false;
    }

    if(this->startTime == 0){
        this->startTime = ((u_int64_t)(flow->data.start_ts_h) << 32)+ (u_int64_t)(flow->data.start_ts_l);
    }

    this -> endTime = max(this->endTime, ((u_int64_t)(flow->data.start_ts_h) << 32)+ (u_int64_t)(flow->data.start_ts_l));

    u_int32_t* buffer_pointer = (u_int32_t*)(this->buffer + this->offset);
    *buffer_pointer = packet_num;
    u_int32_t num = 1;
    for(auto i:flow->data.packet_index){
        buffer_pointer = (u_int32_t*)(this->buffer + this->offset + num * TIMES);
        *buffer_pointer = i;
        num++;
    }

    this->sourceAddressIndex->insert(ntohl(flow->meta.sourceAddress.s_addr),this->offset);
    this->destinationAddressIndex->insert(ntohl(flow->meta.destinationAddress.s_addr),this->offset);
    this->sourcePortIndex->insert(flow->meta.sourcePort,this->offset);
    this->destinationPortIndex->insert(flow->meta.destinationPort,this->offset);
    // this->startTimeIndex->insert(((u_int64_t)(flow->data.start_ts_h) << 32)+ (u_int64_t)(flow->data.start_ts_l),this->offset);
    // this->endTimeIndex->insert(((u_int64_t)(flow->data.end_ts_h) << 32)+ (u_int64_t)(flow->data.end_ts_l),this->offset);

    this->offset += (packet_num + 1) * TIMES;
    return true;
}

list<u_int32_t> IndexGenerator::getPacketOffsetByIndex(struct FlowIndex& index){
    //u_int8_t num = 0;
    u_int8_t flags = index.flags;

    list<u_int32_t> res, tmp;
    bool has_one = false; 

    // auto start = chrono::high_resolution_clock::now();
    // auto end = chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    for(int i = 0;i<INDEX_NUM;++i){
        //if(i<2)continue;
        if(!((flags >> i) & 1))continue;
        // start = chrono::high_resolution_clock::now();
        if(i==0){
            tmp = this->sourceAddressIndex->findByKey(ntohl(index.sourceAddress.s_addr));
        }else if(i==1){
            tmp = this->destinationAddressIndex->findByKey(ntohl(index.destinationAddress.s_addr));
        }else if(i==2){
            tmp = this->sourcePortIndex->findByKey(index.sourcePort);
        }else{
            tmp = this->destinationPortIndex->findByKey(index.destinationPort);
        }
        tmp.sort(); 
        if(!has_one){
            res = tmp;
            has_one = true;
        }else{
            intersect(res,tmp);
            if(res.size() == 0) 
                return res;
        }
        // end = chrono::high_resolution_clock::now();
        // duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        // cout << i << "Index search time: " << duration.count() << " us" << endl;
    }
    
    // auto res = this->startTimeIndex->findByRange(index.startTime,index.endTime);
    // res.sort();
    // auto tmp = this->endTimeIndex->findByRange(index.startTime,index.endTime);
    
    // INTERSECT

    // if(FLAG_VARIABLE){
    //     tmp = this->sourceAddressIndex->findByKey(ntohl(index.sourceAddress.s_addr));
    //     INTERSECT
    // }
    // num++;
    // if(FLAG_VARIABLE){
    //     tmp = this->destinationAddressIndex->findByKey(ntohl(index.destinationAddress.s_addr));
    //     INTERSECT
    // }
    // num++;
    // if(FLAG_VARIABLE){
    //     tmp = this->sourcePortIndex->findByKey(index.sourcePort);
    //     INTERSECT
    // }
    // num++;
    // if(FLAG_VARIABLE){
    //     tmp = this->destinationPortIndex->findByKey(index.destinationPort);
    //     INTERSECT
    // }
    // num++;

    list<u_int32_t> packet_index_list = list<u_int32_t>();
    if(!has_one){
        return packet_index_list;
    }
    for(auto index:res){
        u_int32_t* buffer_pointer = (u_int32_t*)(buffer+index);
        u_int32_t length = buffer_pointer[0];
        packet_index_list.insert(packet_index_list.end(),buffer_pointer + 1, buffer_pointer + length + 1);
    }
    
    return packet_index_list;
}

list<u_int32_t> IndexGenerator::getPacketOffsetByRange(struct FlowIndex& startIndex, struct FlowIndex& endIndex){
    if(startIndex.flags != endIndex.flags || startIndex.startTime != endIndex.startTime || startIndex.endTime != endIndex.endTime){
        return list<u_int32_t>();
    }

    u_int8_t flags = startIndex.flags;

    list<u_int32_t> res, tmp;
    bool has_one = false; 
    
    for(int i = 0;i<INDEX_NUM;++i){
        // if(i<2)continue;
        if(!((flags >> i) & 1))continue;
        if(i==0){
            tmp = this->sourceAddressIndex->findByRange(ntohl(startIndex.sourceAddress.s_addr),ntohl(endIndex.sourceAddress.s_addr));
        }else if(i==1){
            tmp = this->destinationAddressIndex->findByRange(ntohl(startIndex.destinationAddress.s_addr),ntohl(endIndex.sourceAddress.s_addr));
        }else if(i==2){
            tmp = this->sourcePortIndex->findByRange(startIndex.sourcePort,endIndex.sourcePort);
        }else{
            tmp = this->destinationPortIndex->findByRange(endIndex.destinationPort,endIndex.destinationPort);
        }
        tmp.sort(); 
        if(!has_one){
            res = tmp;
            has_one = true;
        }else{
            intersect(res,tmp);
            if(res.size() == 0) 
                return res;
        }
    }

    //u_int8_t num = 0;
    // u_int8_t flags = startIndex.flags;
    // auto res = this->startTimeIndex->findByRange(startIndex.startTime,startIndex.endTime);
    // res.sort();

    // auto tmp = this->endTimeIndex->findByRange(startIndex.startTime,startIndex.endTime);
    // INTERSECT

    // if(FLAG_VARIABLE){
    //     tmp = this->sourceAddressIndex->findByRange(ntohl(startIndex.sourceAddress.s_addr),ntohl(endIndex.sourceAddress.s_addr));
    //     INTERSECT
    // }
    // num++;
    // if(FLAG_VARIABLE){
    //     tmp = this->destinationAddressIndex->findByRange(ntohl(startIndex.destinationAddress.s_addr),ntohl(endIndex.destinationAddress.s_addr));
    //     INTERSECT
    // }
    // num++;
    // if(FLAG_VARIABLE){
    //     tmp = this->sourcePortIndex->findByRange(startIndex.sourcePort,endIndex.sourcePort);
    //     INTERSECT
    // }
    // num++;
    // if(FLAG_VARIABLE){
    //     tmp = this->destinationPortIndex->findByRange(startIndex.destinationPort,endIndex.destinationPort);
    //     INTERSECT
    // }
    // num++;

    list<u_int32_t> packet_index_list = list<u_int32_t>();
    if(!has_one){
        return packet_index_list;
    }
    for(auto index:res){
        u_int32_t* buffer_pointer = (u_int32_t*)(buffer+index);
        u_int32_t length = buffer_pointer[0];
        packet_index_list.insert(packet_index_list.end(),buffer_pointer + 1, buffer_pointer + length + 1);
    }
    return packet_index_list;
}

char* IndexGenerator::outputToStorage(){
    u_int32_t total_length = 0;
    u_int32_t index_offset = 0;
    u_int32_t data_offset = 0;
    u_int32_t filename_length = this->filename.size();
    u_int32_t index_offsets[INDEX_NUM];

    index_offset = META_LEN + filename_length; // total_length, index_offset, data_offset, filename
    data_offset = index_offset;

    char** index_buffer = new char*[INDEX_NUM];
    // index_buffer[0] = this->startTimeIndex->outputToBuffer();
    // index_buffer[1] = this->endTimeIndex->outputToBuffer();
    index_buffer[0] = this->sourceAddressIndex->outputToBuffer();
    index_buffer[1] = this->destinationAddressIndex->outputToBuffer();
    index_buffer[2] = this->sourcePortIndex->outputToBuffer();
    index_buffer[3] = this->destinationPortIndex->outputToBuffer();

    for(int i=0; i<INDEX_NUM; i++){
        index_offsets[i] = data_offset;
        data_offset += (*((u_int32_t*)(index_buffer[i]))) + sizeof(u_int32_t);
    }

    total_length = data_offset;
    total_length += this->offset;

    char* storage_buffer = new char[total_length];

    // meta data
    *(u_int32_t*)storage_buffer = total_length;
    //*(u_int32_t*)(storage_buffer+sizeof(u_int32_t)) = index_offset;
    for(int i=0; i<INDEX_NUM; i++){
        *(u_int32_t*)(storage_buffer+sizeof(u_int32_t)*(i+1)) = index_offsets[i];
    }
    *(u_int32_t*)(storage_buffer+sizeof(u_int32_t)*(INDEX_NUM+1)) = data_offset;
    memcpy(storage_buffer+META_LEN,this->filename.c_str(),filename_length);

    //index
    u_int32_t index_offset_now = index_offset;
    for(int i=0; i<INDEX_NUM; i++){
        u_int32_t index_length = *((u_int32_t*)(index_buffer[i])) + sizeof(u_int32_t);
        memcpy(storage_buffer+index_offset_now,index_buffer[i],index_length);
        index_offset_now += index_length;
    }

    //printf("a:%u,b:%u\n",index_offset_now,data_offset);
    //data
    memcpy(storage_buffer+data_offset,this->buffer,this->offset);

    for(int i=0; i<INDEX_NUM; i++){
        delete[] index_buffer[i];
    }
    delete[] index_buffer;

    //this->clear();
    //this->startTime = 0;
    return storage_buffer;
}

void IndexGenerator::readStorage(char* storage_buffer){
    u_int32_t total_length = *(u_int32_t*)storage_buffer;
    u_int32_t index_offset = *(u_int32_t*)(storage_buffer+sizeof(u_int32_t));
    u_int32_t data_offset = *(u_int32_t*)(storage_buffer+sizeof(u_int32_t)*2);

    string filename(META_LEN,index_offset - META_LEN);

    printf("total_length:%u\n",total_length);
    printf("index_offset:%u\n",index_offset);
    printf("data_offset:%u\n",data_offset);
    printf("filename:%s\n",filename.c_str());

    u_int32_t index_offset_now = index_offset;
    u_int32_t index_length = 0;

    // printf("starttime\n");
    // this->startTimeIndex->readBuffer(storage_buffer + index_offset_now);
    // printf("endtime\n");
    // MOVE_INDEX_POINTER
    // this->endTimeIndex->readBuffer(storage_buffer + index_offset_now);
    // MOVE_INDEX_POINTER
    printf("srcip\n");
    this->sourceAddressIndex->readBuffer(storage_buffer + index_offset_now);
    printf("dstip\n");
    MOVE_INDEX_POINTER
    this->destinationAddressIndex->readBuffer(storage_buffer + index_offset_now);
    printf("srcport\n");
    MOVE_INDEX_POINTER
    this->sourcePortIndex->readBuffer(storage_buffer + index_offset_now);
    printf("dstport\n");
    MOVE_INDEX_POINTER
    this->destinationPortIndex->readBuffer(storage_buffer + index_offset_now);
    MOVE_INDEX_POINTER
}

u_int64_t IndexGenerator::getStartTime(){
    return this->startTime;
}

u_int64_t IndexGenerator::getEndTime(){
    return this->endTime;
}

list<IndexReturn> IndexGenerator::getPacketOffsetByIndex(KeyMode mode, u_int64_t key){
    list<u_int32_t> res;

    switch (mode){
    case KeyMode::SRCIP:
        res = this->sourceAddressIndex->findByKey((u_int32_t)key);
        break;
    case KeyMode::DSTIP:
        res = this->destinationAddressIndex->findByKey((u_int32_t)key);
        break;
    case KeyMode::SRCPORT:
        res = this->sourcePortIndex->findByKey((u_int16_t)key);
        break;
    case KeyMode::DSTPORT:
        res = this->destinationPortIndex->findByKey((u_int16_t)key);
        break;
    default:
        break;
    }

    IndexReturn ret = {
        .filename = this->filename,
        .packetList = list<u_int32_t>(),
    };
    for(auto index:res){
        u_int32_t* buffer_pointer = (u_int32_t*)(buffer+index);
        u_int32_t length = buffer_pointer[0];
        ret.packetList.insert(ret.packetList.end(),buffer_pointer + 1, buffer_pointer + length + 1);
    }
    list<IndexReturn> ret_list = list<IndexReturn>();
    ret_list.push_back(ret);
    return ret_list;
}

list<IndexReturn> IndexGenerator::getPacketOffsetByRange(KeyMode mode, u_int64_t startKey, u_int64_t endKey){
    list<u_int32_t> res;

    switch (mode){
    case KeyMode::SRCIP:
        res = this->sourceAddressIndex->findByRange((u_int32_t)startKey, (u_int32_t)endKey);
        break;
    case KeyMode::DSTIP:
        res = this->destinationAddressIndex->findByRange((u_int32_t)startKey, (u_int32_t)endKey);
        break;
    case KeyMode::SRCPORT:
        res = this->sourcePortIndex->findByRange((u_int16_t)startKey,(u_int16_t)endKey);
        break;
    case KeyMode::DSTPORT:
        res = this->destinationPortIndex->findByRange((u_int16_t)startKey,(u_int16_t)endKey);
        break;
    default:
        break;
    }

    IndexReturn ret = {
        .filename = this->filename,
        .packetList = list<u_int32_t>(),
    };
    for(auto index:res){
        u_int32_t* buffer_pointer = (u_int32_t*)(buffer+index);
        u_int32_t length = buffer_pointer[0];
        ret.packetList.insert(ret.packetList.end(),buffer_pointer + 1, buffer_pointer + length + 1);
    }
    list<IndexReturn> ret_list = list<IndexReturn>();
    ret_list.push_back(ret);
    return ret_list;
}