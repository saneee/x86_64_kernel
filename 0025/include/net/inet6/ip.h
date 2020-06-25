#ifndef _NET_INET6_IP_H
#define _NET_INET6_IP_H
struct in6_addr {
        union {
                uint8_t         __u6_addr8[16];
                uint16_t        __u6_addr16[8];
                uint32_t        __u6_addr32[4];
        } __u6_addr;                    /* 128-bit IP6 address */
};

#define s6_addr   __u6_addr.__u6_addr8
struct ipv6hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
        __u8                    priority:4,
                                version:4;
#elif defined(__BIG_ENDIAN_BITFIELD)
        __u8                    version:4,
                                priority:4;
#else
#error  "Please fix <asm/byteorder.h>"
#endif
        __u8                    flow_lbl[3];

        __be16                  payload_len;
        __u8                    nexthdr;
        __u8                    hop_limit;

        struct  in6_addr        saddr;
        struct  in6_addr        daddr;
} __packed;
#define IPV6VERSION 6
static inline void ip6_init_hdr(struct ipv6hdr *p,void *dip,void *sip, u_char protocal)
{
    p->version = IPV6VERSION;
    p->priority = 0;
    p->flow_lbl[0] = 0;
    p->flow_lbl[1] = 0;
    p->flow_lbl[2] = 0;

    p->payload_len = 0;
    p->hop_limit = 0xff;
    p->nexthdr = protocal;
    p->saddr = *((struct in6_addr *)sip);
    p->daddr = *((struct in6_addr *)dip);
}

#endif
