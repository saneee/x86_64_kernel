#include <yaos/printk.h>
#include <api/net/ip.h>
#include <api/net/udp.h>
static u16 checksum(u32 init, uchar *addr, size_t count)
{
    /* Compute Internet Checksum for "count" bytes * beginning at location "addr". */
    u32 sum = init;
    while( count > 1 )
    {
        /* This is the inner loop */
        sum += ntohs(* (u16*) addr);
        addr += 2;
        count -= 2;
    }
    /* Add left-over byte, if any */
    if( count > 0 )
        sum += ntohs(( *(uchar *) addr ));
    /* Fold 32-bit sum to 16 bits */
    while (sum>>16)
        sum = (sum & 0xffff) + (sum >> 16);
    return (u16)~sum;
}

void ip_set_checksum(struct ip* iphdrp)
{
    iphdrp->ip_sum = 0;
    iphdrp->ip_sum = htons(checksum(0, (uchar *)iphdrp, iphdrp->ip_hl << 2));
}


void ip_set_udp_checksum(struct ip* iphdrp)
{
    struct udphdr *udphdrp = (struct udphdr*)((uchar*)iphdrp + (iphdrp->ip_hl<<2));
    udphdrp->check = 0;

    size_t udplen = ntohs(iphdrp->ip_len) - (iphdrp->ip_hl<<2);
    u32 cksum = 0;
    cksum += ntohs(* (u16*) (&iphdrp->ip_src));
    cksum += ntohs(* ((u16*) (&iphdrp->ip_src)+1));
    cksum += ntohs(* (u16*) (&iphdrp->ip_dst));
    cksum += ntohs(* ((u16*) (&iphdrp->ip_dst)+1));
    cksum += iphdrp->ip_p & 0x00ff;
    cksum += udplen;
    udphdrp->check = htons(checksum(cksum, (uchar*)udphdrp, udplen));

}
void ip_set_udp6_checksum(struct ipv6hdr* iphdrp)
{
    struct udphdr *udphdrp = (struct udphdr*)((uchar*)iphdrp + sizeof(struct ipv6hdr));
    udphdrp->check = 0;

    size_t udplen = ntohs(iphdrp->payload_len);
    u32 cksum = 0;
    u16 * p16 = (u16 *)&iphdrp->saddr;
    for (int i=0;i<2*sizeof(iphdrp->saddr)/sizeof(u16);i++){
        cksum+=ntohs(*p16++);
    }
    cksum += iphdrp->nexthdr & 0x00ff;
    cksum += udplen;
    udphdrp->check = htons(checksum(cksum, (uchar*)udphdrp, udplen));

}

