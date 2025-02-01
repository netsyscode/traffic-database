#ifndef TAGAGGREGATOR_HPP_
#define TAGAGGREGATOR_HPP_
#include "packetAggregator.hpp"
#include <list>

struct TagAttr{
    std::list<u_int64_t> values;
    u_int64_t beginOffset;
    u_int8_t agg;
};

class TagAggregator{
    //hash table
    std::unordered_map<FlowMetadata, TagAttr, FlowMetadata::hash>* aggMaps;
    u_int32_t size;

    void sum(u_int64_t* ori, u_int64_t value){
        * ori += value;
    }
    void max(u_int64_t* ori, u_int64_t value){
        if(value > *ori){
            * ori = value;
        }
    }
    void min(u_int64_t* ori, u_int64_t value){
        if(value < *ori){
            * ori = value;
        }
    }
    bool contain(std::list<u_int64_t>* oris, u_int64_t value){
        for(auto x:*oris){
            if(x == value){
                return false;
            }
        }
        oris->push_back(value);
        return true;
    }

public:
    TagAggregator(u_int32_t size){
        this->aggMaps = new std::unordered_map<FlowMetadata, TagAttr, FlowMetadata::hash>[size]();
        this->size = size;
    }
    ~TagAggregator(){
        delete[] this->aggMaps;
    }
    std::pair<u_int64_t,u_int64_t> addTag(FlowMetadata meta, u_int8_t tag_id, u_int8_t tag_agg, u_int64_t value, u_int64_t offset, u_int64_t last){
        
        auto it = this->aggMaps[tag_id].find(meta);
        if(it == this->aggMaps[tag_id].end()){
            TagAttr tagAttr = {
                .values = std::list<u_int64_t>(),
                .beginOffset = offset,
                .agg = tag_agg,
            };
            tagAttr.values.push_back(value);
            this->aggMaps[tag_id].insert(std::make_pair(meta,tagAttr));
            if(tag_agg == 6){
                return std::make_pair(offset,value);
            }
            return std::make_pair(std::numeric_limits<uint64_t>::max(),value);
        }
        switch (tag_agg){
            case 2:
                this->sum(&(it->second.values.front()),value);
                break;
            case 4:
                this->max(&(it->second.values.front()),value);
                break;
            case 5:
                this->min(&(it->second.values.front()),value);
                break;
            case 6:
                if(this->contain(&(it->second.values),value)){
                    return std::make_pair(it->second.beginOffset,value);
                }
                break;
            default:
                return std::make_pair(std::numeric_limits<uint64_t>::max(),value);
                break;
        }
        if (last == std::numeric_limits<uint64_t>::max() && tag_agg != 6){
            u_int64_t ret = it->second.beginOffset;
            it->second.beginOffset = offset;
            return std::make_pair(ret,it->second.values.front());
        }
        return std::make_pair(std::numeric_limits<uint64_t>::max(),value);
    }

    std::list<std::pair<u_int32_t,std::pair<u_int64_t,u_int64_t>>> getAll(){
        std::list<std::pair<u_int32_t,std::pair<u_int64_t,u_int64_t>>> ret;
        for(u_int32_t i = 0; i<this->size;++i){
            for(auto it = this->aggMaps[i].begin();it != this->aggMaps[i].end();++it){
                if (it->second.agg == 6){
                    break;
                }
                ret.push_back(std::make_pair(i,std::make_pair(it->second.beginOffset,it->second.values.front())));
            }
        }
        return ret;
    }
};

#endif