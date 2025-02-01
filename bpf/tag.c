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
	u_int8_t id:4,
       agg:4;
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

	int tag_pos = 0;

	struct Tag tag1 = {
		.id = 1,
		.agg = 2,
		.length = 0,
		.offset = len,
	};

	mb->dynfield1[tag_pos] = *(uint32_t*)&tag1;
	tag_pos++;	

	uint16_t eth_proto = *(uint16_t*)(data + 12);

	if (eth_proto != htons(0x0800)){ return 1;} // not ip
    uint8_t version = (*(uint8_t*)(data+14) >> 4) & 0x0F; // ip version

	const char* l3 = (const char*)(data + 14);

	uint8_t ip_proto = 0;
	uint8_t hl = 0;

	if(version == 4){
		ip_proto = (uint8_t)l3[9];
		hl = (*(const uint8_t*)(l3)) & 0x0F;
	}else if(version == 6){
		ip_proto = (uint8_t)l3[6];
		hl = 10;
	}

	if (ip_proto != 6){ return 1;} // not tcp

	struct Tag tag2 = {
		.id = 2,
		.agg = 6,
		.length = 8,
		.offset = (14 + hl * 4) * 8 + 13 * 8,
	};
	
	mb->dynfield1[tag_pos] = *(uint32_t*)&tag2;

	tag_pos++;

	// struct Tag tag3 = {
	// 	.id = 3,
	// 	.agg = 5,
	// 	.length = 16,
	// 	.offset = (14 + hl * 4) * 8 + 14 * 8,
	// };

	// mb->dynfield1[tag_pos] = *(uint32_t*)&tag3;

	// tag_pos++;

	return 1;
}
