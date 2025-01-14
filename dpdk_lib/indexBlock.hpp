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
    std::list<u_int64_t> queryPrefix(std::string key){
        std::list<u_int64_t> ret = std::list<u_int64_t>();
        // if(!this->bloomCheck(key)){
        //     return ret;
        // }

        u_int32_t slice_num = key.size()/sizeof(PRE_TYPE);

        PRE_TYPE* pre_ptr = (PRE_TYPE*)(&key[0]);
        u_int32_t layerLen = this->layerLens[0]/sizeof(PreIndexInt);
        u_int32_t pos = 0;

        u_int32_t next = std::numeric_limits<uint32_t>::max();

        // printf("slice num:%u\n",slice_num);

        for (u_int32_t i=0; i<slice_num; ++i){
            // printf("layerlen:%d\n",layerLen);
            auto real_tail_len = this->layerLens[i]/sizeof(PreIndexInt) - pos;
            auto [res_id,next_id] = this->binarySearch(this->layers[i] + pos,layerLen,pre_ptr[slice_num - i - 1],real_tail_len);
            // printf("res_id:%u, next_id:%u\n",res_id,next_id);
            if(res_id == layerLen){
                return ret;
            }
            if(next_id != std::numeric_limits<uint32_t>::max() && next_id != res_id + 1){
                next = min((this->layers[i] + pos + res_id + 1)->offset & OFFSET_MASK,next);
                // printf("next:%u\n",next);
            }
            auto off = (this->layers[i] + pos + res_id)->offset;
            // printf("off:%x, %u\n",off,off);
            // if(off & OFFSET_BIT || i == this->layerNum - 1){
            if(off & OFFSET_BIT ){
                next = min(this->getNextValue(i,pos+res_id),next);
                // printf("next1:%u\n",next);
                
                off &= OFFSET_MASK;
                for(u_int32_t j = off;j<next;++j){
                    ret.push_back(this->values[j]);
                }
                return ret;
            }
            if (i == slice_num - 1){
                next = min(this->getNextValue(i,pos+res_id),next);
                // printf("next2:%u\n",next);
                if (i==this->layerNum-1){
                    for(u_int32_t k = off;k<next;++k){
                        ret.push_back(this->values[k]);
                    }
                    return ret;
                }
                auto bg = off;
                for(u_int32_t j=slice_num;j<this->layerNum;++j){
                    bg = (this->layers[j] + bg)->offset;
                    if(bg & OFFSET_BIT || j==this->layerNum-1){
                        bg &= OFFSET_MASK;
                        for(u_int32_t k = bg;k<next;++k){
                            ret.push_back(this->values[k]);
                        }
                        return ret;
                    }
                }
                // printf("wrong.\n");
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

class IndexBlockCompact{
    u_int32_t key_len;
    u_int32_t indexLen;

    u_int64_t* nodeNum;
    char* keys;
    u_int64_t* values;

    template <class KeyType>
    std::list<u_int64_t> binarySearch(char* index, u_int32_t index_len, KeyType key){
        std::list<u_int64_t> ret = std::list<u_int64_t>();
        u_int32_t ele_len = sizeof(KeyType) + sizeof(u_int32_t);
        u_int32_t left = 0;
        u_int32_t right = index_len/ele_len;
        u_int32_t mid;

        while (left < right) {
            mid = left + (right - left) / 2;

            KeyType key_mid = *(KeyType*)(index + mid * ele_len);
            if(key_mid == key){
                break;
            }
            if (key_mid < key) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        KeyType key_now = *(KeyType*)(index + mid * ele_len);

        if(key_now != key){
            return ret;
        }

        u_int32_t begin = *(u_int32_t*)(index + mid * ele_len + sizeof(KeyType));

        u_int32_t end = mid == index_len/ele_len - 1 ? (u_int32_t)*(this->nodeNum):*(u_int32_t*)(index + (mid + 1) * ele_len + sizeof(KeyType)); 

        for (u_int32_t i = begin;i<end;++i){
            ret.push_back(this->values[i]);
        }

        return ret;
    }

    template <class KeyType>
    std::list<u_int64_t> binarySearchRange(char* index, u_int32_t index_len, KeyType key_left, KeyType key_right){
        std::list<u_int64_t> ret = std::list<u_int64_t>();
        u_int32_t ele_len = sizeof(KeyType) + sizeof(u_int32_t);
        u_int32_t left = 0;
        u_int32_t right = index_len/ele_len;
        u_int32_t mid;

        while (left < right) {
            mid = left + (right - left) / 2;

            KeyType key_mid = *(KeyType*)(index + mid * ele_len);
            if(key_mid == key_left){
                left = mid;
                break;
            }
            if (key_mid < key_left) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        // KeyType key_now = *(KeyType*)(index + left * ele_len);

        // if(key_now != key){
        //     return ret;
        // }


        for(;left<index_len/ele_len;left++){
            KeyType key_now = *(KeyType*)(index + left * ele_len);
            if(key_now > key_right){
                break;
            }
            // u_int64_t value = *(u_int64_t*)(index + left * ele_len + sizeof(KeyType));
            // ret.push_back(value);
            u_int32_t begin = *(u_int32_t*)(index + left * ele_len + sizeof(KeyType));
            u_int32_t end = left == index_len/ele_len - 1 ? (u_int32_t)*(this->nodeNum):*(u_int32_t*)(index + (left + 1) * ele_len + sizeof(KeyType)); 
            for (u_int32_t i = begin;i<end;++i){
                ret.push_back(this->values[i]);
            }
        }

        

        

        return ret;
    }

public:
    IndexBlockCompact(u_int32_t key_len,char* data, u_int64_t data_len){
        this->key_len = key_len;
        this->nodeNum = (u_int64_t*)data;
        this->keys = data + sizeof(u_int64_t);
        this->indexLen = data_len - *(this->nodeNum) * sizeof(u_int64_t);
        this->values = (u_int64_t*)(data + this->indexLen);
    }
    ~IndexBlockCompact() = default;
    std::list<u_int64_t> query(std::string key_str){

        if(this->key_len == 2){
            u_int16_t key = *(u_int16_t*)&key_str[0];
            return this->binarySearch(this->keys,this->indexLen,key);
        }else if(this->key_len == 4){
            u_int32_t key = *(u_int32_t*)&key_str[0];
            return this->binarySearch(this->keys,this->indexLen,key);
        }

        std::list<u_int64_t> ret = std::list<u_int64_t>();
        return ret;
    }

    std::list<u_int64_t> queryRange(std::string key_str_left,std::string key_str_right){

        if(this->key_len == 2){
            u_int16_t key_left = *(u_int16_t*)&key_str_left[0];
            u_int16_t key_right = *(u_int16_t*)&key_str_right[0];
            return this->binarySearchRange(this->keys,this->indexLen,key_left,key_right);
        }else if(this->key_len == 4){
            u_int32_t key_left = *(u_int32_t*)&key_str_left[0];
            u_int32_t key_right = *(u_int32_t*)&key_str_right[0];
            return this->binarySearchRange(this->keys,this->indexLen,key_left,key_right);
        }

        std::list<u_int64_t> ret = std::list<u_int64_t>();
        return ret;
    }

};

#endif