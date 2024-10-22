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
#include <stddef.h>
#include <rte_config.h>
#include <rte_mbuf_core.h>

struct Result{
	uint8_t length;
	uint32_t offset;
};

struct Result tcp_flags(char* ptr, uint32_t len, uint32_t proto){
	struct Result result = { .length = 0, .offset = 0};
	if (proto != 6){ return result;} // not tcp
	result.length = 8; // length of flags
	result.offset = 13; // position of flags
	return result;
}

uint64_t
entry(void *pkt)
{
	struct rte_mbuf *mb;

	mb = pkt;
	mb->dynfield1[0] = 0xff;

	
	// mb->vlan_tci = 0;
	// mb->ol_flags &= ~(RTE_MBUF_F_RX_VLAN | RTE_MBUF_F_RX_VLAN_STRIPPED);

	return 1;
}
