#define NO_SYS 1
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_MPU_COMPATIBLE 0
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define MEMP_NUM_TCP_PCB 10000
#define MEMP_NUM_PBUF	1024
#define PBUF_POOL_SIZE 1280
#define MEM_SIZE 	0x100000
#define ETHARP_SUPPORT_STATIC_ENTRIES 1
#if 0 
#define TCP_DEBUG LWIP_DBG_ON
#define ETHARP_DEBUG     LWIP_DBG_ON
#define NETIF_DEBUG      LWIP_DBG_ON
#define LWIP_DBG_TYPES_ON   LWIP_DBG_ON
#define LWIP_DBG_MIN_LEVEL  0
#define IP_DEBUG         LWIP_DBG_ON
#define LWIP_DEBUG 	 LWIP_DBG_ON
#define TCP_INPUT_DEBUG  LWIP_DBG_ON
#define TCP_OUTPUT_DEBUG LWIP_DBG_ON
#define HTTPD_DEBUG 	 LWIP_DBG_ON
#define MEMP_DEBUG       LWIP_DBG_ON
#endif
#define MEMP_DEBUG       LWIP_DBG_OFF
#define LWIP_DEBUG       LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG LWIP_DBG_OFF
#define ETHARP_DEBUG     LWIP_DBG_OFF
#define HTTPD_DEBUG      LWIP_DBG_OFF

#define MEMP_MEM_MALLOC			0
#define MEMP_SANITY_CHECK	0
#define MEMP_OVERFLOW_CHECK     0	
#define LWIP_IPV6                       0
#define IPV6_FRAG_COPYHEADER            0
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

