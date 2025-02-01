# Traffic database (CraftIndex)

## 1.Background
### goals
* Real time capture of high-speed traffic
* High parallelism index construction
* Index storage compression
* Index key customization

### Technology Dependencies
* Programming Language: C++, version 17
* DPDK, version 21.11.6

## 2.Build & run
### Dependencies library install
* libpcap-dev: `apt install libpcap-dev`
* build-essential: `apt install build-essential`
* meson: `apt install meson`
* python3-pyelftools: `apt install python3-pyelftools`
* pkg-config: `apt install pkg-config`

### DPDK install (Reference: [https://doc.dpdk.org/guides/linux_gsg/index.html](https://doc.dpdk.org/guides/linux_gsg/index.html))
* Download DPDK (version 21.11.6) from [https://core.dpdk.org/download/](https://core.dpdk.org/download/)
* Compile & build DPDK

```
tar xJf dpdk-<version>.tar.xz
cd dpdk-<version>
meson build
cd build
ninja
(sudo) meson install
(sudo) ldconfig
```

* NIC bind (Reference: [https://doc.dpdk.org/guides/linux_gsg/linux_drivers.html](https://doc.dpdk.org/guides/linux_gsg/linux_drivers.html))

```
# To see the status of all network ports on the system
./usertools/dpdk-devbind.py --status

# [NIC] is the high speed NIC that needs to receive traffic
./usertools/dpdk-devbind.py --bind=vfio-pci [NIC]
```

### System build & run

```
mkdir build
mkdir data
mkdir data/source
mkdir data/index
mkdir data/output

make
(sudo) ./build/dpdkControllerTest
```

### Parameter Description
* Users can input parameters to set:
	* whether binding to cores
	* the number of DPDK packet capture threads (also the number of packet processing threads)
	* the number of indexing threads in the thread pool
	* the cores to bind (if needed)
* Example 1 (without binding cores)

```
Do you want to bind to cores? (y/n)
n
Enter number of DPDK packet capture threads
2
Enter number of indexing thread
4
...
[Press any key to quit]
```
* Example 2 (with binding cores, note that the core 0 is remained for DPDK TX thread)

```
Do you want to bind to cores? (y/n)
y
Enter the controller core number (0 is remained)
20
Enter number of DPDK packet capture threads
2
Enter the core number for each DPDK packet capture threads (0 is remained)
2 4
Enter the core number for each packet processing threads (0 is remained)
6 8
Enter number of indexing thread
4
Enter the core number for each indexing threads (0 is remained)
10 12 14 16
...
[Press any key to quit]
```

## 4.The structure of codes
* **dpdk_lib**: the manually written dependency libraries required for the project, including indexed data structures, lock free read-write ring structures, etc
* **dpdk_component**: the main code of project components
* **bpf**: package tagging code in BPF format
* **test**: test code, includes:
	* dpdkControllerTest.cpp: main file of the program
	* indexCompressTest.cpp: the code to test the performance of index compression
	* indexTest.cpp: the code to test the query latency of indexes
	* ringTest.cpp: the code to test read/write speed of lock-free read-write ring
	* (All code in this folder needs to be compiled using the corresponding makefile; Except for dpdkControlTest.cpp, all other files do not require dpdk dependencies for compilation and operation)
* **experiment**: the experiment data is stored in this folder
