#ifndef _DRIVERS_NETDRIVER_H
#define _DRIVERS_NETDRIVER_H
struct netdriver_data;

/* Function call table for network drivers. */
struct netdriver {
        int (*ndr_init)(unsigned int instance, ether_addr_t *addr);
        void (*ndr_stop)(void);
        void (*ndr_mode)(unsigned int mode);
        ssize_t (*ndr_recv)(struct netdriver_data *data, size_t max);
        int (*ndr_send)(struct netdriver_data *data, size_t size);
        void (*ndr_stat)(eth_stat_t *stat);
        void (*ndr_intr)(unsigned int mask);
        void (*ndr_alarm)(clock_t stamp);
        void (*ndr_other)(const message *m_ptr, int ipc_status);
};

/* Functions defined by libnetdriver. */
void netdriver_task(const struct netdriver *ndp);


#endif
