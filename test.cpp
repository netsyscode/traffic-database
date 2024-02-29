#include <iostream>
#include <chrono>
#include "extractor.hpp"
#include "flow.hpp"
#include "index.hpp"
#include "storage.hpp"
#include "querier.hpp"
using namespace std;

bool fileExists(const std::string& filename) {
    ifstream file(filename);
    return file.good(); // 如果文件成功打开，则返回 true；否则返回 false
}

void test_storage(){
    string filename = "./source_data/caida.pcap";
    string storagename = "./dest_data/caida.pcapidx";
    Extractor extractor(filename);
    //IndexGenerator generator(32000,filename);
    StorageOperator storage(storagename);

    // char* buffer = storage.readIndex(storagename,0);
    // if(buffer == nullptr){
    //     printf("Wrong!\n");
    // }
    //generator.readStorage(buffer);

    struct in_addr ipAddress[2];

    inet_aton("47.123.210.92", &ipAddress[0]);
    inet_aton("199.152.10.148", &ipAddress[1]);

    u_int64_t sts = 1521118773;
    sts <<= 32;
    sts += 100000;

    u_int64_t ets = 1521118777;
    ets <<= 32;
    ets += 400000;

    struct FlowIndex index = {
        .flags = 0b1111,
        .sourceAddress = ipAddress[0],
        .destinationAddress = ipAddress[1],
        .sourcePort = 9339,
        .destinationPort = 26927,
        .startTime = sts,
        .endTime = ets,
    };

    // struct FlowIndex index2 = {
    //     .flags = 0b1111,
    //     .sourceAddress = ipAddress[2],
    //     .destinationAddress = ipAddress[3],
    //     .sourcePort = 50508,
    //     .destinationPort = 2781,
    //     .startTime = sts,
    //     .endTime = ets,
    // };
    
    auto start = chrono::high_resolution_clock::now();
    IndexReturn index_list = storage.getPacketOffsetByIndex(index,storagename,0);
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<std::chrono::microseconds>(end - start);
    cout << "Storage search time: " << duration.count() << " us" << endl;
    

    extractor.openFile();
    printf("filename:%s\n",index_list.filename.c_str());
    printf("packets_length:%lu\n",index_list.packetList.size());

    // list<u_int32_t> il = index_list.packetList;
    // // for(auto i:il){
    // //     cout << i << " ";
    // // }
    // // cout<<endl;
    // il.sort();
    // for(auto i:il){
    //     cout << i << " ";
    // }
    // cout<<endl;
    //index_list.packetList.sort();
    PacketMetadata* meta = extractor.readPacketByOffset(index_list.packetList.front());
    printf("len:%u\n",meta->length);
    printf("\n");

    // index_list = storage.getPacketOffsetByRange(index,index2,storagename,0);

    // printf("filename:%s\n",index_list.filename.c_str());
    // printf("packets_length:%lu\n",index_list.packetList.size());

    // il = index_list.packetList;
    // il.sort();
    // //index_list.packetList.sort();
    // meta = extractor.readPacketByOffset(il.front());
    // printf("len:%u\n",meta->length);
    // printf("\n");
    extractor.closeFile();
}

void test_main(){
    string filename = "./source_data/caida_10000.pcap";
    string storagename = "./dest_data/caida_10000.pcapidx";
    Extractor extractor(filename);
    FlowAggregator agg(filename,FlowMode::MANUAL);
    IndexGenerator generator(32000,filename);
    StorageOperator storage(storagename);
    extractor.openFile();
    //extractor.changeOffset(80);
    // for(int i=0;i<5;++i){
    //     PacketMetadata* meta = extractor.readCurrentPacketAndChangeOffset();
    //     if(meta == nullptr)
    //         break;
    //     printf("srcIP:%s\n",inet_ntoa(meta->sourceAddress));          
    //     printf("dstIP:%s\n",inet_ntoa(meta->destinationAddress));
    //     printf("sport:%hu\n",meta->sourcePort);
    //     printf("dport:%hu\n",meta->destinationPort);
    //     printf("ack:%u\n",meta->ack);
    //     printf("len:%u\n",meta->length);
    //     printf("ts:%u.%u\n",meta->ts_h,meta->ts_l);
    //     printf("file:%s\n",meta->filename.c_str());
    //     printf("offset:%u\n",meta->offset);
    //     delete meta;
    // }
    int count = 0;

    for(int i=0;i<10000;++i){
        PacketMetadata* meta = extractor.readCurrentPacketAndChangeOffset();
        if(meta == nullptr)
            break;
        agg.putPacket(meta);
        delete meta;
        count++;
    }
    printf("%d\n",count);
    agg.truncateAll();
    //printf("truncate.\n");
    list<Flow*>* flow_list = agg.outputByNow();

    count = 0;
    for (auto it:(*flow_list)){
        count++;

        // if(count>=8)
        //     break;

        if(!generator.putFlow(it))
            break;

            //continue;

        printf("length:%u\n",it->data.packet_num);  
        printf("srcIP:%s\n",inet_ntoa(it->meta.sourceAddress));          
        printf("dstIP:%s\n",inet_ntoa(it->meta.destinationAddress));
        printf("sport:%hu\n",it->meta.sourcePort);
        printf("dport:%hu\n",it->meta.destinationPort);
        printf("pkt len:%u\n",it->data.packet_num);
        printf("byte len:%llu\n",it->data.byte_num);
        printf("start ts:%u.%u\n",it->data.start_ts_h,it->data.start_ts_l);
        printf("end ts:%u.%u\n",it->data.end_ts_h,it->data.end_ts_l);
        
        PacketMetadata* meta = extractor.readPacketByOffset(it->data.packet_index.front());
        printf("len:%u\n",meta->length);
        printf("\n");
        if(count>6)
            continue;
    }
    printf("%d\n",count);
    printf("%s\n",generator.getFilenName().c_str());

    struct in_addr ipAddress[4];

    inet_aton("53.93.33.77", &ipAddress[0]);
    inet_aton("159.21.229.180", &ipAddress[1]);
    inet_aton("53.93.33.78", &ipAddress[2]);
    inet_aton("159.21.229.200", &ipAddress[3]);

    u_int64_t sts = 1695872100;
    sts <<= 32;
    sts += 100000;

    u_int64_t ets = 1695872400;
    ets <<= 32;
    ets += 262000;

    struct FlowIndex index = {
        .flags = 0b1111,
        .sourceAddress = ipAddress[0],
        .destinationAddress = ipAddress[1],
        .sourcePort = 50497,
        .destinationPort = 2780,
        .startTime = sts,
        .endTime = ets,
    };

    struct FlowIndex index2 = {
        .flags = 0b1111,
        .sourceAddress = ipAddress[2],
        .destinationAddress = ipAddress[3],
        .sourcePort = 50508,
        .destinationPort = 2781,
        .startTime = sts,
        .endTime = ets,
    };
    

    list<u_int32_t> index_list = generator.getPacketOffsetByIndex(index);

    printf("packets_length:%lu\n",index_list.size());
    // for(auto i:index_list){
    //     cout << i << " ";
    // }
    // cout<<endl;
    index_list.sort();
    // for(auto i:index_list){
    //     cout << i << " ";
    // }
    // cout<<endl;
    PacketMetadata* meta = extractor.readPacketByOffset(index_list.front());
    printf("len:%u\n",meta->length);
    printf("\n");

    index_list = generator.getPacketOffsetByRange(index,index2);

    printf("packets_length:%lu\n",index_list.size());
    index_list.sort();
    meta = extractor.readPacketByOffset(index_list.front());
    printf("len:%u\n",meta->length);
    printf("\n");

    char* buffer = generator.outputToStorage();
    // // if(storage.writeIndex(buffer)){
    // //     buffer = storage.readIndex(storagename,0);
    // //     generator.readStorage(buffer);
    // // }
    generator.readStorage(buffer);
    

    agg.clearFlowList(flow_list);
    extractor.closeFile();
}

// void test_prefix(){
//     PrefixTree tree(32);
//     list<u_int8_t> lenList;
//     for(int i=0;i<4;++i){
//         lenList.push_back(8);
//     }
//     if(!tree.setPrefixLength(lenList))
//         return;

//     string index[4] = {"11.0.0.1","10.0.0.5","11.0.0.1","10.0.0.1"};
//     struct in_addr ipAddress[4];
//     int data[4] = {1,2,3,4};
    
//     for(int i=0;i<4;++i){
//         inet_aton(index[i].c_str(), &ipAddress[i]);
//         tree.putData(ntohl(ipAddress[i].s_addr),data+i);
//     }

//     tree.print();

//     for(int i=0;i<4;++i){
//         list<void*> da = tree.getDataByIndex(ntohl(ipAddress[i].s_addr));
//         for(auto d:da){
//             printf("data of %s : %d\n",index[i].c_str(),*((int*)d));
//         }
//     }
// }

void test_skip_list(){
    SkipList<u_int32_t,int> sk(32);

    string index[4] = {"53.93.33.77","53.93.33.78","53.93.33.77","53.93.33.77"};
    struct in_addr ipAddress[4];
    int data[4] = {1,2,3,4};
    
    for(int i=0;i<4;++i){
        inet_aton(index[i].c_str(), &ipAddress[i]);
        sk.insert(ntohl(ipAddress[i].s_addr),data[i]);
    }

    printf("len:%u\n",sk.getLength());

    for(int i=0;i<4;++i){
        list<int> da = sk.findByKey(ntohl(ipAddress[i].s_addr));
        for(auto d:da){
            printf("data of %s : %d\n",index[i].c_str(),d);
        }
    }

    list<int> da = sk.findByRange(ntohl(ipAddress[0].s_addr),ntohl(ipAddress[1].s_addr));
    for(auto d:da){
        printf("data: %d\n",d);
    }
    //sk.traverse();
}

void test_performance(string filename, string storagename, string outputname, u_int32_t buffersize){
    if (fileExists(storagename)){
        if (remove(storagename.c_str()) != 0) {
            perror("Error deleting file");
            return; // 删除失败
        }
    }

    auto start = chrono::high_resolution_clock::now();

    Extractor extractor(filename);
    FlowAggregator agg(filename,FlowMode::MANUAL);
    IndexGenerator generator(buffersize,filename);
    StorageOperator storage(storagename);
    Querier querier = Querier(&generator, &storage);

    auto end = chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    cout << "Initial time: " << duration.count() << " us" << endl;


    extractor.openFile();
    int count = 0;

    start = chrono::high_resolution_clock::now();

    while(true){
        PacketMetadata* meta = extractor.readCurrentPacketAndChangeOffset();
        if(meta == nullptr)
            break;
        agg.putPacket(meta);
        delete meta;
        count++;
    }
    extractor.closeFile();

    end = chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    cout << "Packet extract and Flow aggrate time: " << duration.count() << " us" << endl;

    // printf("%d\n",count);

    start = chrono::high_resolution_clock::now();
    agg.truncateAll();

    //printf("truncate.\n");
    list<Flow*>* flow_list = agg.outputByNow();
    end = chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    cout << "Flow output time: " << duration.count() << " us" << endl;

    count = 0;
    start = chrono::high_resolution_clock::now();
    for (auto it:(*flow_list)){
        count++;
        if(!generator.putFlow(it))
            break;

        // printf("length:%u\n",it->data.packet_num);  
        // printf("srcIP:%s\n",inet_ntoa(it->meta.sourceAddress));          
        // printf("dstIP:%s\n",inet_ntoa(it->meta.destinationAddress));
        // printf("sport:%hu\n",it->meta.sourcePort);
        // printf("dport:%hu\n",it->meta.destinationPort);
        // printf("pkt len:%u\n",it->data.packet_num);
        // printf("byte len:%llu\n",it->data.byte_num);
        // printf("start ts:%u.%u\n",it->data.start_ts_h,it->data.start_ts_l);
        // printf("end ts:%u.%u\n",it->data.end_ts_h,it->data.end_ts_l);
        
        // PacketMetadata* meta = extractor.readPacketByOffset(it->data.packet_index.front());
        // printf("len:%u\n",meta->length);
        // printf("\n");
        // if(count>6)
        //     continue;
    }
    //cout<< "count:" << count << endl;
    end = chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    cout << "Index generate time: " << duration.count() << " us" << endl;
    // printf("%d\n",count);
    // printf("%s\n",generator.getFilenName().c_str());

    struct in_addr ipAddress[2];

    inet_aton("47.123.210.92", &ipAddress[0]);
    inet_aton("199.152.10.148", &ipAddress[1]);

    u_int64_t sts = 1521118773;
    sts <<= 32;
    sts += 100000;

    u_int64_t ets = 1521118777;
    ets <<= 32;
    ets += 400000;

    struct FlowIndex index = {
        .flags = 0b1111,
        .sourceAddress = ipAddress[0],
        .destinationAddress = ipAddress[1],
        .sourcePort = 9339,
        .destinationPort = 26927,
        .startTime = sts,
        .endTime = ets,
    };
    string exp = "(srcip == 47.123.210.92 && dstip == 199.152.10.148) && (srcport == 9339 || dstport == 26927 || dstport == 26928)";
    //string exp = "(srcip == 47.123.210.92 && dstip == 199.152.10.148)";
    
    list<AtomKey> key_list = list<AtomKey>();
    AtomKey key = {
        .keyMode = KeyMode::SRCIP,
        .key = ntohl(index.sourceAddress.s_addr),
    };
    key_list.push_back(key);

    start = chrono::high_resolution_clock::now();
    //list<u_int32_t> index_list = generator.getPacketOffsetByIndex(index);
    //list<IndexReturn> index_list = generator.getPacketOffsetByIndex(KeyMode::SRCIP,ntohl(index.sourceAddress.s_addr));
    //list<IndexReturn> index_list = generator.getPacketOffsetByIndex(KeyMode::SRCPORT,index.sourcePort);
    //list<IndexReturn> index_list = querier.getOffsetByIndex(key_list,index.startTime,index.endTime);
    querier.queryWithExpression(exp,"./output_data/caida_memory.pcap",index.startTime,index.endTime);
    end = chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    cout << "Memory search time: " << duration.count() << " us" << endl;
    //printf("packets_length:%lu\n",index_list.front().packetList.size());
    // PacketMetadata* meta = extractor.readPacketByOffset(index_list.front());
    // printf("len:%u\n",meta->length);
    // printf("\n");

    //querier.outputPacketToNewFile("./output_data/caida_memory.pcap",index_list);

    // printf("packets_length:%lu\n",index_list.size());

    start = chrono::high_resolution_clock::now();
    char* buffer = generator.outputToStorage();
    if(storage.writeIndex(buffer,generator.getStartTime(),generator.getEndTime())){
        generator.clear();
        end = chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        cout << "Index storage time: " << duration.count() << " us" << endl;

        start = chrono::high_resolution_clock::now();
        // list<IndexReturn> res_list = storage.getPacketOffsetByIndex(KeyMode::DSTPORT,index.destinationPort,index.startTime,index.endTime);
        //list<IndexReturn> res_list = storage.getPacketOffsetByRange(KeyMode::DSTPORT,index.destinationPort,index.destinationPort+10,index.startTime,index.endTime);
        //list<IndexReturn> res_list = querier.getOffsetByIndex(key_list,index.startTime,index.endTime);
        querier.queryWithExpression(exp,"./output_data/caida.pcap",index.startTime,index.endTime);
        end = chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        cout << "Storage search time: " << duration.count() << " us" << endl;
        // printf("filename:%s\n",res_list.front().filename.c_str());
        // printf("packets_length:%lu\n",res_list.front().packetList.size());
        // meta = extractor.readPacketByOffset(res_list.front().packetList.front());
        // printf("len:%u\n",meta->length);
        // printf("\n");
        // for(auto off:res_list.front().packetList){
        //     cout<<off<<" ";
        // }
        // cout<<endl;
        //Querier querier = Querier();
        //querier.outputPacketToNewFile(outputname,res_list);
    }

    agg.clearFlowList(flow_list);
    
}

void test_query(){
    //string exp = "(ip.src == 192.168.1.100 && ip.dst[0:32] == 192.168.1.200) && (tcp.port == 80 || udp.port in {53,54}) && http.request.method == \"GET\" && !(dns.qry.name == \"example.com\" && frame.len > 100)";
    string exp = "(srcip == 47.123.210.92 && dstip == 199.152.10.148) && (srcport == 9339 || dstport == 26927 || dstport == 26928)";
    QueryTree tree = QueryTree();
    if(tree.inputExpression(exp)){
        printf("Done.\n");
    }
    //tree.constructExpressionTree();
}

int main(){
    //test_skip_list();
    //test_storage();
    //test_main();
    test_performance("./source_data/caida.pcap","./dest_data/caida.pcapidx","./output_data/caida.pcap",8000000);
    //test_query();
    return 0;
}