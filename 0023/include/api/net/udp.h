#ifndef _API_NET_UDP_H
#define _API_NET_UDP_H
#include <yaos/types.h>
#include <net/ether.h>
#include <net/inet/ip.h>
#include <net/inet6/ip.h>
struct udphdr {
        __be16  source;
        __be16  dest;
        __be16  len;
        __sum16 check;
};

struct udp_packet {
    eth_hdr_t eth;
    struct ip ip;
    struct udphdr udp;    
    uchar data[];
} __packed;
struct udp6_packet {
    eth_hdr_t eth;
    struct ipv6hdr ip6;
    struct udphdr udp;
    uchar data[];

} __packed;
extern void udp_set_data(struct udp_packet *p,void *data, size_t len);
extern void udp6_set_data(struct udp6_packet *p,void *data, size_t len);
#endif
