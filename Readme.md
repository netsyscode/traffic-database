# 流量索引数据库（重构代码中）

## 大致思路

![](./doc/fig/thinking.png)

* 重构目标：将原先的组件内功能拆解到数据结构中，组件尽量只需要调用数据结构接口，同时支持并行

## lib
### arrayList.hpp
* Packet Pointer底层结构
* value部分只能单线程写入（trace cacher），next部分可多线程写入（packet aggregator，每个线程只能读取和修改特定id的next）
* 需修改（id使用）——完成
	* id是否意味着线程数量不能即时增减？
		* 否，每个线程只需要记录自己持有的id，当出现新线程时，将已有线程的id分配给新线程；删除线程时，将该线程的id分配给其余线程
* 完成测试

### header.hpp
* 用于指针转换
* 包括pcap头、ip头、udp头、tcp头

### ringBuffer.hpp
* 用于在暂存index的键值（index buffer，见./doc/fig/input.png）
* 支持多线程竞争写入与读出
* 为支持多线程快速写入与读出，使用原子操作改变writePos和readPos，不取模
	* 使用std::atomic_uint64_t，最大支持16EB的总数据量
	* 对于数据库而言仍需要（截断时）刷新
* 完成测试

### shareBuffer.hpp
* Packet Buffer底层结构，核心为char数组
* 单线程写入（trace cacher），多线程只读（packet aggregator）
* 支持数据格式：
	* pcap格式
	* todo：rte_mbuf格式（DPDK）
* 完成测试

### skipList.hpp
* index cache底层结构
* 多线程写入（index generator），多线程只读（querier）
	* 每一个节点拥有自己的锁，在对节点进行修改前，需要获取节点的锁
* 传入前，需要将数据转化为定长格式
* 待测试

## component
### controller
* 组件控制器
* 生成其余组件，调整可并行部分线程数量
* 可生成trace catch与packet aggregator，已测试，其余功能待完成

### pcapReader
* 从指定pcap文件中持续读取pcap格式数据包
* 作为Trace Catcher的当前替代
	* 与重构前相比，承担了extractor功能
* 初始化所需成员
	* pcap文件的头部字节长度（一般为24）
	* 该文件数据链路层头部字节长度（一般为14）
	* 共享数据结构packet buffer
	* 共享数据结构packet pointer
* 完成测试

### packetAggregator
* 从packet pointer和packet buffer中读取数据，将相同IP/端口聚合，并填写packet pointer的next字段，将索引值放入index buffer
* 初始化所需成员
	* 数据链路层头部字节长度（与pcapReader一致，一般为14）
	* 共享数据结构packet buffer
	* 共享数据结构packet pointer
	* 共享数据结构index buffer
* 完成测试

### indexGenerator
* 从index buffer中读取数据，存入index cache
* 初始化所需成员
	* 共享数据结构index buffer
	* 共享数据结构index cache
* 每一个index generator只会属于一个index buffer
* 完成读测试，写功能待实现