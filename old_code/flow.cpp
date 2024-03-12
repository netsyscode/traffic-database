#include "flow.hpp"

void FlowAggregator::clearFlowList(list<Flow*>* flow_list){
    for (auto it:(*flow_list))
        delete it;
    flow_list->clear();
    delete flow_list;
}

void FlowAggregator::changeFileName(string filename){
    this->filename = filename;
}

bool FlowAggregator::isEmpty(){
    return this->aggMap.empty();
}

bool FlowAggregator::hasFlow(){
    return this->nowFlow != nullptr;
}

void FlowAggregator::checkNowFlow(){
    if(this->nowFlow == nullptr){
        this->nowFlow = new list<Flow*>();
    }
}

void FlowAggregator::putPacket(struct PacketMetadata* pkt_meta){
    if(pkt_meta == nullptr){
        printf("NULL!\n");
        return;
    }

    FlowMetadata flow_meta = {
        .sourceAddress = pkt_meta->sourceAddress,
        .destinationAddress = pkt_meta->destinationAddress,
        .sourcePort = pkt_meta->sourcePort,
        .destinationPort = pkt_meta->destinationPort,
    };

    auto it = this->aggMap.find(flow_meta);
    if(it == this->aggMap.end()){
        FlowData flow_data = {
            .packet_index = list<u_int32_t>(),
            .start_ts_h = pkt_meta->ts_h,
            .start_ts_l = pkt_meta->ts_l,
            .end_ts_h = pkt_meta->ts_h,
            .end_ts_l = pkt_meta->ts_l,
            .packet_num = 1,
            .byte_num = pkt_meta->length,
        };
        flow_data.packet_index.push_back(pkt_meta->offset);
        this->aggMap.insert(make_pair(flow_meta,flow_data));
    }
    else{
        it->second.packet_index.push_back(pkt_meta->offset);
        it->second.end_ts_h = pkt_meta->ts_h;
        it->second.end_ts_l = pkt_meta->ts_l;
        it->second.packet_num ++;
        it->second.byte_num += pkt_meta->length;
    }

    //delete pkt_meta; // it shouldn't be deleted here
}

void FlowAggregator::truncateAll(){
    checkNowFlow();
    for(auto it:this->aggMap){
        Flow* flow = new Flow();
        flow->filename = this->filename;
        flow->meta = it.first;
        flow->data = it.second;
        this->nowFlow->push_back(flow);
    }
    this->aggMap.clear();
}

list<Flow*>* FlowAggregator::outputByNow(){
    auto tmp = this->nowFlow;
    this->nowFlow = nullptr;
    return tmp;
}