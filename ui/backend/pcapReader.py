import dpkt
import socket
from operator import itemgetter
import hashlib

FILE_NAME = "../../data/output/result.pcap"


def get_protocol(ip):
    if isinstance(ip.data, dpkt.tcp.TCP):
        return "TCP"
    elif isinstance(ip.data, dpkt.udp.UDP):
        return "UDP"
    elif isinstance(ip.data, dpkt.icmp.ICMP):
        return "ICMP"
    else:
        return "Unknown"

def read_pcap(file_path):
    packets = []

    with open(file_path, 'rb') as f:
        pcap = dpkt.pcap.Reader(f)
        for ts, buf in pcap:
            # eth = dpkt.ethernet.Ethernet(buf)
            packets.append((ts, buf, len(buf)))

    return packets

# 按时间戳排序数据包
def sort_packets_by_timestamp(packets):
    sorted_packets = sorted(packets, key=itemgetter(0))
    return sorted_packets

def hash_ip_port(src,dst,sport,dport):
    input_str = f"{src}:{dst}:{sport}:{dport}"
    # 使用 SHA-256 哈希算法计算哈希值
    hash_object = hashlib.sha256(input_str.encode())
    # 获取哈希值的十六进制表示
    hash_hex = hash_object.hexdigest()
    # 将十六进制表示转换为整数
    hash_int = int(hash_hex[0:8], 16)
    return hash_int

def get_packets():
    return sort_packets_by_timestamp(read_pcap(FILE_NAME))

def get_meta_list(packets):
    meta_list = []
    num = 0
    for timestamp, buf, len in packets:
        try:
            eth = dpkt.ethernet.Ethernet(buf)
            ip = eth.data
            tcp = ip.data

            prot = get_protocol(ip)
            src = socket.inet_ntoa(ip.src)
            dst = socket.inet_ntoa(ip.dst)
            sport = tcp.sport
            dport = tcp.dport
            hash_value = hash_ip_port(src,dst,sport,dport)
            num += 1

            dic = {}
            dic["number"] = num
            dic["timestamp"] = timestamp
            dic["len"] = len
            dic["srcip"] = src
            dic["dstip"] = dst
            dic["srcport"] = sport
            dic["dstport"] = dport
            dic["protocol"] = prot
            dic["hash"] = hash_value
            meta_list.append(dic)
        except Exception:
            meta_list.append({})
    return meta_list

def get_packet_meta(buf):
    meta = {}
    eth = dpkt.ethernet.Ethernet(buf)
    src_mac = ':'.join(f'{byte:02x}' for byte in eth.src)
    dst_mac = ':'.join(f'{byte:02x}' for byte in eth.dst)
    eth_type = eth.type

    eth_meta = {"srcmac":src_mac,"dstmac":dst_mac,"type":eth_type}
    meta["l2"]=eth_meta

    if not isinstance(eth.data, dpkt.ip.IP):
        meta["data"]=len(eth.data)
        return meta

    ip = eth.data

    srcip = socket.inet_ntoa(ip.src)
    dstip = socket.inet_ntoa(ip.dst)
    version = ip.v
    ip_meta = {"srcip":srcip,"dstip":dstip,"version":version}
    meta["l3"]=ip_meta

    if isinstance(ip.data, dpkt.tcp.TCP):
        tcp = ip.data
        seq_num = tcp.seq
        flags = tcp.flags
        sport = tcp.sport
        dport = tcp.dport
        tcp_meta = {"protocol":"TCP","srcport":sport,"dstport":dport,"seq":seq_num,"flags":flags}
        meta["l4"] = tcp_meta
        meta["data"] = len(tcp.data)
        return meta
    
    if isinstance(ip.data, dpkt.udp.UDP):
        udp = ip.data
        sport = udp.sport
        dport = udp.dport
        udp_meta = {"protocol":"UDP","srcport":sport,"dstport":dport}
        meta["l4"] = udp_meta
        meta["data"] = len(udp.data)
        return meta
    
    if isinstance(ip.data, dpkt.icmp.ICMP):
        icmp = ip.data
        icmp_type = icmp.type
        icmp_code = icmp.code
        icmp_meta = {"protocol":"ICMP","type":icmp_type,"code":icmp_code}
        meta["l4"] = icmp_meta
        meta["data"] = len(icmp.data)
        return meta    

    meta["data"] = len(ip.data)
    return meta

def write_pcap(filename,packets):
    with open(filename, 'wb') as f:
        pcap_writer = dpkt.pcap.Writer(f)
        for ts, buf, _ in packets:
            pcap_writer.writepkt(buf, ts)

if __name__ == "__main__":
    packets = get_packets()
    meta_list = get_meta_list(packets)
    for dic in meta_list:
        if len(dic) == 0:
            print("error packet")
            continue
        print("[+]{} time: {}, src: {}:{} -> dst: {}:{}, len: {}, protocol: {}, hash: {}".format(
            dic["number"], dic["timestamp"], dic["srcip"], dic["srcport"], dic["dstip"], dic["dstport"],
            dic["len"], dic["protocol"], dic["hash"]
        ))
        _, packet, _ = packets[dic["number"]-1]
        data = ':'.join('{:02x}'.format(byte) for byte in packet)
    write_pcap("../../data/output/res.pcap",packets)
