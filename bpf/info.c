/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2018 Intel Corporation
 */

/*
 * eBPF program sample.
 * Accepts pointer to first segment packet data as an input parameter.
 * analog of tcpdump -s 1 -d 'dst 1.2.3.4 && udp && dst port 5000'
 * (000) ldh      [12]
 * (001) jeq      #0x800           jt 2    jf 12
 * (002) ld       [30]
 * (003) jeq      #0x1020304       jt 4    jf 12
 * (004) ldb      [23]
 * (005) jeq      #0x11            jt 6    jf 12
 * (006) ldh      [20]
 * (007) jset     #0x1fff          jt 12   jf 8
 * (008) ldxb     4*([14]&0xf)
 * (009) ldh      [x + 16]
 * (010) jeq      #0x1388          jt 11   jf 12
 * (011) ret      #1
 * (012) ret      #0
 *
 * To compile on x86:
 * clang -O2 -U __GNUC__ -target bpf -c t1.c
 *
 * To compile on ARM:
 * clang -O2 -I/usr/include/aarch64-linux-gnu/ -target bpf -c t1.c
 */

#include <stdint.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

struct HeaderInfo{
    u_int16_t l3_type;
    u_int16_t l3_offset;
    u_int8_t l4_type;
    u_int32_t l4_offset;
};


uint64_t
entry(void *pkt)
{
	struct ether_header *ether_header = (void *)pkt;

	if (ether_header->ether_type != htons(0x0800))
		return 0;

    struct HeaderInfo *info = (struct HeaderInfo*)(ether_header);

    info->l3_type = ether_header->ether_type;
    info->l3_offset = sizeof(struct ether_header);

	struct iphdr *iphdr = (void *)(ether_header + 1);

    info->l4_type = iphdr->protocol;
    info->l4_offset = iphdr->ihl * 4 + sizeof(struct ether_header);

	return 1;
}
