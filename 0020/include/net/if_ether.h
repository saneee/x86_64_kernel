#ifndef _NET_IF_ETHER_H
#define _NET_IF_ETHER_H
#define ETHER_ADDR_LEN  6       /* length of an Ethernet address */
#define ETHER_TYPE_LEN  2       /* length of the Ethernet type field */
#define ETHER_CRC_LEN   4       /* length of the Ethernet CRC */
#define ETHER_HDR_LEN   ((ETHER_ADDR_LEN * 2) + ETHER_TYPE_LEN)
#define ETHER_MIN_LEN   64      /* minimum frame length, including CRC */
#define ETHER_MAX_LEN   1518    /* maximum frame length, including CRC */
#define ETHER_MAX_LEN_JUMBO 9018 /* maximum jumbo frame len, including CRC */

/*
 * Some Ethernet extensions.
 */
#define ETHER_VLAN_ENCAP_LEN 4  /* length of 802.1Q VLAN encapsulation */
#define ETHER_PPPOE_ENCAP_LEN 8 /* length of PPPoE encapsulation */

/*
 * Ethernet address - 6 octets
 * this is only used by the ethers(3) functions.
 */
struct ether_addr {
        uint8_t ether_addr_octet[ETHER_ADDR_LEN];
} __packed;

#define ea_addr ether_addr_octet
typedef struct ether_addr ether_addr_t;
/*
 * Structure of a 10Mb/s Ethernet header.
 */
struct ether_header {
        uint8_t  ether_dhost[ETHER_ADDR_LEN];
        uint8_t  ether_shost[ETHER_ADDR_LEN];
        uint16_t ether_type;
} __packed;
#endif
