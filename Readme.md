# 流量索引数据库

## 1.项目概述
### 项目目的
* 对网络中的流量进行实时全镜像抓取，建立索引，并将流量与索引持久化存储；
* 提供用户查询接口，可从抓取的历史网络流量中快速检索特定网络流量，为网络流量取证提供支持。

### 项目目标
* **高速流量实时抓取**：开发对高吞吐量链路流量的全镜像捕获方法，并对流量进行初步过滤与处理；
* **高并行度索引构建**：采用可高度并行化的数据结构完成数据包的初步聚合和索引构建，使索引构建速率与数据包抓取速率相匹配；
* **高压缩比流量数据存储**：设计适合流量数据特征的高压缩比压缩方法，以较低的存储开销存储尽可能长时间的流量数据；
* **低延迟、交互友好的查询接口**：除建立低查询延迟的查询算法外，查询接口也应满足用户自定义索引的需求，并尽可能给出有效的流量查询信息或指导

### 技术栈
* 前端技术
	* HTML：用来设计用户界面框架
	* JavaScript：用于实现可动态交互的查询接口页面
* 后端技术
	* 编程语言：Python
	* 编程框架：Flask
* 数据库技术
	* 编程语言：C++

## 2.项目构建与运行
### 项目构建
* 在macOS上构建项目

```
(sudo) ./prepare_macos.sh
```
* 在ubuntu上构建项目

```
(sudo) ./prepare_ubuntu.sh
```

### 项目运行
* 将待建立索引的pcap文件放在./data/source文件夹下
* 启动数据库（请确保12345端口没有被占用）
	* -f为待建立索引的文件路径
	* -e为数据链路层包头长度，默认为14（用于测试的caida数据集以太网包头因隐私考虑被删去，此时包头长度设为0）

```
./build/controllerTest -f [source file path] -e [eth length]
```

* 启动后端（新工作台，请确保5000端口没有被占用，且数据库已启动）
	* -e意义同上

```
python ./ui/backend/backend.py
```

* 启动前端（新工作台）

```
python -m http.server [port] --directory ./ui/frontend
```

### 退出
* 使用`^c`退出前端与后端程序，此时数据库会自动退出

## 3.网页接口使用
* 打开浏览器，访问http://localhost:[port]
* 在输入框输入检索表达式
	* 目前支持四种查询键：`srcip`、`dstip`、`srcport`、`dstport`，分别为源IP地址、目的IP地址、源端口号和目的端口号
	* 键与值之间通过`==`连接，如`dstport == 80`
	* IP地址支持前缀查询，如`192.168.1.0/24`
	* 表达式之间可以通过`||`、`&&`和`()`连接为复杂表达式，如`srcip = 10.1.0.0/16 || (srcport == 80 && dstip == 192.1.100.1)`
* （可选）在开始与结束时间输入框输入搜索限定时间范围
	* 时间范围格式为`年-月-日 时:分:秒`，如`2023-09-28 11:38:20`

## 4.代码结构
```
.
├── Makefile					# c++编译脚本
├── Readme.md					# 说明文档
├── build						# 二进制文件
├── component					# 组件
│   ├── controller.cpp			# 全局控制器，用于生成内存控制器、存储控制器、查询器
│   ├── controller.hpp
│   ├── indexGenerator.cpp		# 索引生成器
│   ├── indexGenerator.hpp
│   ├── memoryMonitor.cpp		# 内存控制器，用于控制索引建立阶段组件和数据结构的建立、截断与删除
│   ├── memoryMonitor.hpp
│   ├── packetAggregator.cpp	# 数据包聚合器
│   ├── packetAggregator.hpp
│   ├── pcapReader.cpp			# pcap文件读取器
│   ├── pcapReader.hpp
│   ├── querier.cpp				# 查询器
│   ├── querier.hpp
│   ├── storageMonitor.cpp		# 存储控制器
│   ├── storageMonitor.hpp
│   ├── storageOperator.cpp		# 存储组件（当前未实现，使用存储控制器单线程承担功能）
│   └── storageOperator.hpp
├── data
│   ├── index					# 索引文件
│   ├── output					# 查询结果
│   └── source					# 源pcap文件
├── doc							# 设计文档
├── lib							# 数据结构头文件
│   ├── arrayList.hpp			# 数组列表
│   ├── header.hpp				# 协议与pcap头部
│   ├── ringBuffer.hpp			# 并行环状缓冲区
│   ├── shareBuffer.hpp			# 线性共享缓冲区
│   ├── skipList.hpp			# 并行跳表
│   └── util.hpp				# 其他
├── old_code					# 早期代码，无用
├── prepare_macos.sh			# 构建脚本（macOS）
├── prepare_ubuntu.sh			# 构建脚本（Ubuntu）
├── test						# 测试运行接口
│   ├── controllerTest.cpp
│   └── memoryMonitorTest.cpp
└── ui
    ├── backend				
    │   ├── backend.py			# 后端服务器
    │   ├── dataTrans.py		# 日期格式转换
    │   ├── pcapReader.py		# pcap文件读取
    │   └── socketConnect.py	# 与数据库连接
    └── frontend
        ├── component
        │   ├── download.js		# 下载按钮
        │   ├── global.js		# 全局变量
        │   └── search.js		# 查询接口
        └── index.html			# 网页框架
```

## 5.开发文档记录
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
* 目前sleep+强制停止，需要把握睡眠时间（实际应用中不该有这一操作）

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
			* 连同index generator一同替换
	* (v)线程旧内存替换后，修改pause变量
	* (vi)将旧内存整理后交给存储系统
	* (vii)回收旧内存
* 已测试，~~在index generator线程过多时会偶发错误，~~已修复

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

### storageMonitor
* 将截断后的索引、指针与数据存入文件
* 当前为最简化实现，将原始数据直接存储
* 数据块有时间范围作为索引（假设packet时间顺序分布）

### querier
* 查询接口
* 当前功能
	* 仅支持ip与端口查询，支持ip前缀查询
	* 不支持不等号（<>等)查询（解析接口没有打开，且没有运算简化接口会导致大量无用查询，内部数据结构支持）
	* 支持&&, ||运算
	* 支持时间范围筛选
* 完成测试

## UI
* 后端
	* python代码实现，功能包括
		* 与querier传递消息（在querier上增加一个网络接口）
		* 解析并暂存pcap文件
	* 完成
* 前端
	* html + js实现
	* 主要功能：
		* 点击搜索按钮，传递搜索表达式，获得搜索结果列表
		* 点击列表，获得详细信息
		* 点击下载文件，获得文件
	* 完成

## Todo List
* 0. **修复并行bug!!!**
* 1. **ui优化**（前置8）
* 2. 存储并行化
* 3. 延迟截断
* 4. 线程数自动调整
* 5. ARP、IPv6协议解析
* 6. 抓包接口（前置：5）
* 7. ～～自定义协议解析～～协议解析库
* 8. **自定义索引建立**（前置：7）
* 9. 存储压缩（前置：2，3）
* 10. DPDK抓包（前置：6）
* 11. **性能测试**（前置2）