#ifndef _NETINET_IP_H_
#define _NETINET_IP_H_
#include <net/in.h>
#include <api/endian.h>
#define IPVERSION       4
struct ip {
#if __BYTE_ORDER == __LITTLE_ENDIAN
        u_char  ip_hl:4,                /* header length */
                ip_v:4;                 /* version */
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
        u_char  ip_v:4,                 /* version */
                ip_hl:4;                /* header length */
#endif
        u_char  ip_tos;                 /* type of service */
        u_short ip_len;                 /* total length */
        u_short ip_id;                  /* identification */
        u_short ip_off;                 /* fragment offset field */
#define IP_RF 0x8000                    /* reserved fragment flag */
#define IP_DF 0x4000                    /* dont fragment flag */
#define IP_MF 0x2000                    /* more fragments flag */
#define IP_OFFMASK 0x1fff               /* mask for fragmenting bits */
        u_char  ip_ttl;                 /* time to live */
        u_char  ip_p;                   /* protocol */
        u_short ip_sum;                 /* checksum */
        struct  in_addr ip_src,ip_dst;  /* source and dest address */
} __attribute__ ((aligned (4), packed));

#define IP_MAXPACKET    65535           /* maximum packet size */
static inline void ip_init_hdr(struct ip *p,void *dip,void *sip, u_char protocal)
{
    p->ip_v = IPVERSION;
    p->ip_hl = sizeof(*p)/4;
    p->ip_tos = 0;
    p->ip_id = 0;
    p->ip_off = htons(IP_DF);
    p->ip_ttl = 0xff;
    p->ip_p = protocal;
    p->ip_sum = 0;
    p->ip_src = *((struct in_addr *)sip);
    p->ip_dst = *((struct in_addr *)dip);
}
#endif
