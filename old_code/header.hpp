#ifndef HEADER_H_
#define HEADER_H_
#include <pcap.h>

/*从字符串直接转换为header时，为大端序*/

struct data_header{
    u_int32_t ts_h;
    u_int32_t ts_l;
    u_int32_t caplen;
    u_int32_t len;
};

struct ip_header{
#ifdef WORKS_BIGENDIAN
    u_int8_t ip_version:4,    /*version:4*/
       ip_header_length:4; /*IP协议首部长度Header Length*/
#else
    u_int8_t ip_header_length:4,
       ip_version:4;
#endif
    u_int8_t ip_tos;         /*服务类型Differentiated Services  Field*/
    u_int16_t ip_length;  /*总长度Total Length*/
    u_int16_t ip_id;         /*标识identification*/
    u_int16_t ip_off;        /*片偏移*/
    u_int8_t ip_ttl;            /*生存时间Time To Live*/
    u_int8_t ip_protocol;        /*协议类型（TCP或者UDP协议）*/
    u_int16_t ip_checksum;  /*首部检验和*/
    struct in_addr  ip_source_address; /*源IP*/
    struct in_addr  ip_destination_address; /*目的IP*/
 };

struct tcp_header
 { 
  u_int16_t tcp_source_port;		  //源端口号
  
  u_int16_t tcp_destination_port;	//目的端口号
  
  u_int32_t tcp_acknowledgement;	//序号
  
  u_int32_t tcp_ack;	//确认号字段
 #ifdef WORDS_BIGENDIAN
  u_int8_t tcp_offset:4 ,
     tcp_reserved:4;
 #else
  u_int8_t tcp_reserved:4,
     tcp_offset:4;
 #endif
  u_int8_t tcp_flags;
  u_int16_t tcp_windows;	//窗口字段
  u_int16_t tcp_checksum;	//检验和
  u_int16_t tcp_urgent_pointer;	//紧急指针字段
};

struct udp_header
 { 
  u_int16_t udp_source_port;		  //源端口号
  
  u_int16_t udp_destination_port;	//目的端口号
  
  u_int16_t udp_length;

  u_int16_t udp_checksum;
};
#endif