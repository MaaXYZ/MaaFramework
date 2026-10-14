#pragma once

// 生产调用由统一核心线程串行执行；独立探针在其主线程执行 raw API。
#define NO_SYS 1
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define LWIP_IPV4 0
#define LWIP_IPV6 1
#define LWIP_IPV6_AUTOCONFIG 0
#define LWIP_IPV6_SEND_ROUTER_SOLICIT 0
#define LWIP_IPV6_DUP_DETECT_ATTEMPTS 0
#define LWIP_IPV6_MLD 0
#define LWIP_IPV6_FRAG 0
#define LWIP_IPV6_REASS 1
// 64 位主机的重组辅助结构大于 IPv6 分片头，需要单独保存原头。
#define IPV6_FRAG_COPYHEADER 1
#define LWIP_IPV6_NUM_ADDRESSES 1
#define LWIP_TCP 1
#define LWIP_TCP_PCB_NUM_EXT_ARGS 1
#define LWIP_UDP 1
#define LWIP_RAW 0
#define LWIP_DNS 0
#define LWIP_DHCP 0
#define LWIP_AUTOIP 0
#define LWIP_ARP 0
#define MEM_LIBC_MALLOC 1
#define MEMP_MEM_MALLOC 1
#define MEM_ALIGNMENT 8
#define TCP_MSS 1440
#define LWIP_WND_SCALE 1
#define TCP_RCV_SCALE 4
#define TCP_WND (512 * 1024)
#define TCP_SND_BUF 65535
#define TCP_SNDLOWAT 16384
#define TCP_SND_QUEUELEN 1024
#define TCP_QUEUE_OOSEQ 1
#define TCP_OOSEQ_MAX_BYTES (256 * 1024)
#define TCP_OOSEQ_MAX_PBUFS 256
#define LWIP_STATS 0
