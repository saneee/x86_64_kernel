#include <yaos/printk.h>
#include <api/net/udp.h>
#include <string.h>
#include <api/net/ip.h>
void udp_set_data(struct udp_packet *p,void *data, size_t len)
{
    memcpy(p->data, data, len);
    p->udp.len = htons(len + sizeof(p->udp));
    p->ip.ip_len = htons(len +sizeof(p->udp) + (p->ip.ip_hl<<2));
    ip_set_checksum(&p->ip);
    ip_set_udp_checksum(&p->ip);
}
void udp6_set_data(struct udp6_packet *p,void *data, size_t len)
{
    memcpy(p->data, data, len);
    p->udp.len = htons(len + sizeof(p->udp));
    p->ip6.payload_len = p->udp.len;
    ip_set_udp6_checksum(&p->ip6);
}

