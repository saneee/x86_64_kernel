#define NO_SYS 1
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_MPU_COMPATIBLE 0
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define TCP_DEBUG LWIP_DBG_ON
#define ETHARP_DEBUG     LWIP_DBG_ON
#define NETIF_DEBUG      LWIP_DBG_ON
#define LWIP_DBG_TYPES_ON   LWIP_DBG_ON
#define LWIP_DBG_MIN_LEVEL  0
#define IP_DEBUG         LWIP_DBG_ON
#define LWIP_DEBUG 1
#define TCP_INPUT_DEBUG  LWIP_DBG_ON
#define TCP_OUTPUT_DEBUG LWIP_DBG_ON
#define LWIP_IPV6                       1
#define IPV6_FRAG_COPYHEADER            1
#define LWIP_IPV6_DUP_DETECT_ATTEMPTS   0

/* Enable some protocols to test them */
#define LWIP_DHCP                       1
#define LWIP_AUTOIP                     1

#define LWIP_IGMP                       1
#define LWIP_DNS                        1

#define LWIP_ALTCP                      1

/* Turn off checksum verification of fuzzed data */
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0
#define CHECKSUM_CHECK_ICMP             0
#define CHECKSUM_CHECK_ICMP6            0

