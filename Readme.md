# 流量索引数据库（重构代码中）

## 大致思路

![](./doc/fig/thinking.png)

* 重构目标：将原先的组件内功能拆解到数据结构中，组件尽量只需要调用数据结构接口，同时支持并行
* 索引生成系统当前架构

![](./doc/fig/index.png)

## lib
### arrayList.hpp
* Packet Pointer底层结构
* value部分只能单线程写入（trace cacher），next部分可多线程写入（packet aggregator，每个线程只能读取和修改特定id的next）
* 需修改（id使用）——完成
	* id是否意味着线程数量不能即时增减？
		* 否，每个线程只需要记录自己持有的id，当出现新线程时，将已有线程的id分配给新线程；删除线程时，将该线程的id分配给其余线程
* 考虑到截断，需要保证容量小于32位非负整型最大值的一半
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
* 写入完成测试，查询待测试
* thinking：计数排序方法或许更适合长度2B以内的索引！

## component
### controller
* 组件控制器
* 生成各类monitor
* multithread分支原有功能拆分到memoryMonitor中

### memoryMonitor
* 索引生成系统内存部分工作的控制器（将原controller的工作拆分出来）
* 生成索引生成系统组件
* 截断操作设计：
	* (i)触发：packet buffer~~或packet pointer~~使用大小超过警告（调整两者大小，保证packet pointer不会先触发）
	* (ii)~~申请新共享内存~~（使用内存池，提前申请新内存以节省时间）
	* (iii)通过线程asynchronousPause接口替换旧内存
	* (iv)线程内部操作
		* trace catcher：直接替换，旧内存交由controller管理
		* packet aggregator：packet buffer与index buffer直接替换（因为当前只考虑流索引，不需要考虑截断问题），packet pointer保留直至下一次截断
		* index generater：不由外部截断，当index buffer的写线程为空，且读取结束后直接停止
		* 问题：当前设计中需要等上游线程替换完成，下游线程再开始替换。如果有一个线程（特别是packet aggregator）显著慢于其他线程，会导致下游截断滞后，是否会造成严重后果（如ring buffer阻塞）？
			* ~~controller负载平衡？~~未解决本质问题
			* ~~一个ring buffer对一个数据包聚合器？~~index cache依旧会遇到该问题
			* 连同index generator一同替换？这个应该最合适，而且可以通过线程池优化线程建立过程
			* ~~ring buffer残余收集器？~~难以实现残余区分
			* ~~不替换ring buffer？~~无法区分新旧流
			* ~~保留index buffer，直至其write thread清零，期间不等待旧index buffer读取？~~难以实现不等待读取
	* (v)线程旧内存替换后，修改pause变量
	* (vi)将旧内存整理后交给存储系统
	* (vii)回收旧内存
* 待测试

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
* 截断：如果新数据包属于截断前的旧流：
	* 将旧的packet pointer中，流的最后位置指向新数据包位置+旧buffer长度
* 完成测试

### indexGenerator
* 从index buffer中读取数据，存入index cache
* 初始化所需成员
	* 共享数据结构index buffer
	* 共享数据结构index cache
* 每一个index generator只会属于一个index buffer
* 完成测试

### querier
* 查询接口
* 当前为初级版
	* 仅支持ip与端口查询
	* 不支持范围查询（解析接口没有打开，内部数据结构支持）
	* 仅支持&&, ||运算
	* 仅在index cache中查询（storage还没有实现）
* 初级版完成测试