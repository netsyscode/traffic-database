#ifndef INDEXBLOCK_HPP_
#define INDEXBLOCK_HPP_

#include "skipList.hpp"
#include "bloomFilter.hpp"

u_int64_t min(u_int64_t x, u_int64_t y){
    return x < y ? x : y;
}

class IndexBlock{
    u_int32_t layerNum;

    u_int32_t* nodeNum;
    BloomFilter* filter;
    u_int32_t* layerLens;
    std::vector<PreIndexInt*> layers;
    u_int64_t* values;

    u_int64_t valueLen;

    bool bloomCheck(const std::string& key){
        return this->filter->contains(key);
    }
    /* res id, next id */
    std::pair<u_int32_t,u_int32_t> binarySearch(PreIndexInt* layer, u_int32_t layerLen, PRE_TYPE pre, u_int32_t real_tail_len){
        u_int32_t left = 0;
        u_int32_t right = layerLen;

        while (left < right) {
            u_int32_t mid = left + (right - left) / 2;
            PRE_TYPE key_mid = (layer + mid)->pre;
            if(key_mid == pre){
                if((layer + mid)->offset & OFFSET_BIT){
                    return std::make_pair(mid,std::numeric_limits<uint32_t>::max());
                }
                for(u_int32_t i = mid + 1; i<real_tail_len; ++i){
                    // auto offset = (layer + mid)->offset;
                    if(((layer + i)->offset & OFFSET_BIT) == 0){
                        // printf("get next:%u\n",i);
                        return std::make_pair(mid,i);
                    }
                }
                return std::make_pair(mid,real_tail_len);
            }
            if (key_mid < pre) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        return std::make_pair(layerLen, std::numeric_limits<uint32_t>::max());
    }
    u_int32_t getNextValue(u_int32_t layerID, u_int32_t pos){
        pos++;
        for(u_int32_t i = layerID;i<this->layerNum;++i){
            // printf("i:%u, pos:%u\n",i,pos);
            if(pos >= this->layerLens[i]/sizeof(PreIndexInt)){
                return this->valueLen / sizeof(u_int64_t);
            }
            auto off = this->layers[i][pos].offset;
            // printf("value off:%u\n",off);
            if(off & OFFSET_BIT){
                return off & OFFSET_MASK;
            }
            pos = off;
        }
        return pos;
    }
public:
    IndexBlock(u_int32_t len, char* data, u_int64_t data_len){
        this->layerNum = len/sizeof(PRE_TYPE);
        this->nodeNum = (u_int32_t*)data;
        this->filter = new BloomFilter(data + sizeof(u_int32_t),*(this->nodeNum),FILTER_K_LEN);
        this->layerLens = (u_int32_t*)(data + sizeof(u_int32_t) + *(this->nodeNum));
        this->layers = std::vector<PreIndexInt*>();
        PreIndexInt* key_ptr = (PreIndexInt*)(this->layerLens + this->layerNum);
        for (u_int32_t i=0; i<this->layerNum; ++i){
            this->layers.push_back(key_ptr);
            key_ptr += this->layerLens[i]/sizeof(PreIndexInt);
        }
        this->values = (u_int64_t*)key_ptr;
        this->valueLen = data_len - ((u_int64_t)(this->values) - (u_int64_t)data);
        // printf("value len:%lu\n",this->valueLen);
    }
    ~IndexBlock(){
        delete this->filter;
    }
    std::list<u_int64_t> query(std::string key){
        std::list<u_int64_t> ret = std::list<u_int64_t>();
        if(!this->bloomCheck(key)){
            return ret;
        }

        PRE_TYPE* pre_ptr = (PRE_TYPE*)(&key[0]);
        u_int32_t layerLen = this->layerLens[0]/sizeof(PreIndexInt);
        u_int32_t pos = 0;

        u_int32_t next = std::numeric_limits<uint32_t>::max();

        for (u_int32_t i=0; i<this->layerNum; ++i){
            // printf("layerlen:%d\n",layerLen);
            auto real_tail_len = this->layerLens[i]/sizeof(PreIndexInt) - pos;
            auto [res_id,next_id] = this->binarySearch(this->layers[i] + pos,layerLen,pre_ptr[this->layerNum - i - 1],real_tail_len);
            // printf("res_id:%d, next_id:%d\n",res_id,next_id);
            if(res_id == layerLen){
                return ret;
            }
            if(next_id != std::numeric_limits<uint32_t>::max() && next_id != res_id + 1){
                next = min((this->layers[i] + pos + res_id + 1)->offset & OFFSET_MASK,next);
                // printf("next:%u\n",next);
            }
            auto off = (this->layers[i] + pos + res_id)->offset;
            // printf("off:%x, %u\n",off,off);
            if(off & OFFSET_BIT || i == this->layerNum - 1){
                next = min(this->getNextValue(i,pos+res_id),next);
                // printf("next:%u\n",next);
                
                off &= OFFSET_MASK;
                for(u_int32_t j = off;j<next;++j){
                    ret.push_back(this->values[j]);
                }
                return ret;
            }
            
            if(next_id < real_tail_len){
                layerLen = (this->layers[i] + pos + next_id)->offset - off;
            }else{
                layerLen = this->layerLens[i+1]/sizeof(PreIndexInt) - off;
            }
            pos = off;
        }
        return ret;
    }
};

#endif