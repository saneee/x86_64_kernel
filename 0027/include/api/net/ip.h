#ifndef _API_NET_IP_H
#define _API_NET_IP_H
#include <net/inet/ip.h>
#include <net/inet6/ip.h>
extern void ip_set_checksum(struct ip* iphdrp);
extern void ip_set_udp_checksum(struct ip* iphdrp);
extern void ip_set_udp6_checksum(struct ipv6hdr *p);
#endif
