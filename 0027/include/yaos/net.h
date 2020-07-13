#ifndef __YAOS_NET_H_
#define __YAOS_NET_H_
typedef void (*netsoftirq_callback) (void *driver, void *param);
extern int
regist_net_driver_softirq(void *driver, netsoftirq_callback rx, netsoftirq_callback tx, void *param);
extern void run_net_tx();
extern void run_net_rx();
#endif
