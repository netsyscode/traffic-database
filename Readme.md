# 流量索引数据库（重构代码中）

## 大致思路

![](./doc/fig/thinking.png)

## lib
### arrayList.hpp
* Packet Pointer底层结构
* value部分只能单线程写入（trace cacher），next部分可多线程写入（packet aggregator，每个线程只能读取和修改特定id的next）
* 需修改（id使用）
	* id是否意味着线程数量不能即时增减？
		* 否，每个线程只需要记录自己持有的id，当出现新线程时，将已有线程的id分配给新线程；删除线程时，将该线程的id分配给其余线程

### header.hpp
* 用于指针转换
* 包括pcap头、ip头、udp头、tcp头

### ringBuffer.hpp
* 暂时无作用
* 未测试

### shareBuffer.hpp
* Packet Buffer底层结构
* 单线程写入（trace cacher），多线程只读（packet aggregator）
* 待完成

## component
### controller
* 组件控制器
* 生成其余组件，调整可并行部分线程数量
* 待完成