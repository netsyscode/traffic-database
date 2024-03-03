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
* 待测试

### header.hpp
* 用于指针转换
* 包括pcap头、ip头、udp头、tcp头

### ringBuffer.hpp
* 暂时无作用，原本试图用于Packet Buffer
* 未测试

### shareBuffer.hpp
* Packet Buffer底层结构，核心为char数组
* 单线程写入（trace cacher），多线程只读（packet aggregator）
* 支持数据格式：
	* pcap格式
	* todo：rte_mbuf格式（DPDK）
* 完成，待测试

## component
### controller
* 组件控制器
* 生成其余组件，调整可并行部分线程数量
* 待完成