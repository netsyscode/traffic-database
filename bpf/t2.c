/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2018 Intel Corporation
 */

/*
 * eBPF program sample.
 * Accepts pointer to struct rte_mbuf as an input parameter.
 * cleanup mbuf's vlan_tci and all related RX flags
 * (RTE_MBUF_F_RX_VLAN_PKT | RTE_MBUF_F_RX_VLAN_STRIPPED).
 * Doesn't touch contents of packet data.
 * To compile:
 * clang -O2 -emit-llvm -S t2.c
 * llc -march=bpf -filetype=obj t2.ll
 *
 * NOTE: if DPDK is not installed system-wide, add compiler flag with path
 * to DPDK rte_mbuf.h file, e.g. "clang -I/path/to/dpdk/headers -O2 ..."
 */

#include <stdint.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <rte_config.h>
#include <rte_mbuf_core.h>

struct Tag{
	uint8_t id;
	uint8_t length;
	uint16_t offset;
};

struct Result{
	uint8_t length;
	uint16_t offset;
};

inline struct Result IPv4_proto(const char* ptr, uint32_t len, uint32_t proto){
    struct Result result = { .length = 0, .offset = 0};
    if (proto != htons(0x0800)){ return result;} // not ip
    uint8_t version = (*(uint8_t*)(ptr) >> 4) & 0x0F; // ip version
    if (version != 4){ return result;} // not ipv4
    
    result.length = 8; // length of protocol
    result.offset = 72; // position of protocol
    return result;
}

// struct Result tcp_flags(const char* ptr, uint32_t len, uint32_t proto){
// 	struct Result result = { .length = 0, .offset = 0};
// 	if (proto != 6){ return result;} // not tcp
// 	result.length = 6; // length of flags
// 	result.offset = 106; // position of flags
// 	return result;
// }

uint64_t
entry(void *pkt)
{
	struct rte_mbuf *mb = pkt;

	const char* data = rte_pktmbuf_mtod(mb, const char *);
	uint16_t len = mb->data_len;

	// const struct ether_header *ether_header = (const struct ether_header*)data;

	// // if (ether_header->ether_type != htons(0x0800))
	// // 	return 0;

	// const char* l3 = (const char*)(data + 14);

	// struct Result res_l3 = IPv4_proto(data+14,len-14,*(uint16_t*)(data + 12));

	int tag_pos = 0;

	uint16_t proto = *(uint16_t*)(data + 12);

	if (proto != htons(0x0800)){ return 1;} // not ip
    uint8_t version = (*(uint8_t*)(data+14) >> 4) & 0x0F; // ip version
    if (version != 4){ return 1;} // not ipv4

	struct Tag tag1 = {
		.id = 1,
		.length = 8,
		.offset = 72 + 14,
	};

	mb->dynfield1[tag_pos] = *(uint32_t*)&tag1;
	tag_pos++;


	// uint8_t version = (*(const uint8_t*)(l3) >> 4) & 0x0F;

	// if(version == 4){
	// 	uint8_t proto = (uint8_t)l3[9];
	// 	uint8_t hl = (*(const uint8_t*)(l3)) & 0x0F;
	// 	if(res_l3.length != 0){
	// 		struct Tag tag1 = {
	// 			.id = 1,
	// 			.length = res_l3.length,
	// 			.offset = res_l3.offset + 14,
	// 		};
	
	// 		mb->dynfield1[tag_pos] = *(uint32_t*)&tag1;
	// }
        
    // }else if(version == 6){
    //     const u_int16_t* sport = (const u_int16_t*)(meta.data + this->eth_header_len + IPV6_HEADER_LEN);
    //     const u_int16_t* dport = sport + 1;
    //     IPv6Address srcip = {
    //         .high = swap_endianness(*(u_int64_t*)(meta.data + this->eth_header_len + 8)),
    //         .low = swap_endianness(*(u_int64_t*)(meta.data + this->eth_header_len + 16)),
    //     };
    //     IPv6Address dstip = {
    //         .high = swap_endianness(*(u_int64_t*)(meta.data + this->eth_header_len + 24)),
    //         .low = swap_endianness(*(u_int64_t*)(meta.data + this->eth_header_len + 32)),
    //     };
    //     FlowMetadata flow_meta = {
    //         .sourceAddress = std::string((char*)&srcip,sizeof(srcip)),
    //         .destinationAddress = std::string((char*)&dstip,sizeof(dstip)),
    //         .sourcePort = htons(*sport),
    //         .destinationPort = htons(*dport),
    //     };
    //     return flow_meta;
    // }
	
	// mb->vlan_tci = 0;
	// mb->ol_flags &= ~(RTE_MBUF_F_RX_VLAN | RTE_MBUF_F_RX_VLAN_STRIPPED);

	return 1;
}
