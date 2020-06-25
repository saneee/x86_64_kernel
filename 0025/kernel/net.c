#include <yaos/types.h>
#include <yaos/init.h>
#include <yaos/percpu.h>
#include <errno.h>
#include <yaoscall/malloc.h>
#include <yaos/sched.h>
#include <yaos/irq.h>
#include <yaos/assert.h>
#include <yaos/tasklet.h>
#include <yaos/time.h>
#include <yaos/net.h>
#include <yaos/clockevent.h>
#define NR_NETDRIVER_BUF 8

struct net_callback;
struct net_callback {
    struct net_callback *pnext;
    void *driver;
    void *param;
    netsoftirq_callback tx_callback;
    netsoftirq_callback rx_callback;
};
struct netdriver_manager {
    struct net_callback *pfree;
    struct net_callback *phead;
    struct net_callback netdrivers[NR_NETDRIVER_BUF];
    __thread bool canrun_tx;
    __thread bool canrun_rx;


};
static DEFINE_PER_CPU(struct netdriver_manager,netmanager);

int 
regist_net_driver_softirq(void *driver, netsoftirq_callback rx, netsoftirq_callback tx, void *param)
{
    __thread struct netdriver_manager *pm = this_cpu_ptr(&netmanager);
    struct net_callback *p = pm->pfree;
    if (!p) {
        p = yaos_malloc(sizeof(*p));
        if (!p)
            return ENOMEM;
    }
    else {
        pm->pfree = p->pnext;
    }
    p->driver = driver;
    p->tx_callback = tx;
    p->rx_callback = rx;
    p->param = param;
    p->pnext = pm->phead;
    pm->phead = p;
     
    return 0;
}

void run_net_rx(void)
{
    struct netdriver_manager *pm = this_cpu_ptr(&netmanager);
    pm->canrun_rx = true;
    raise_softirq(NET_RX_SOFTIRQ);
}
void run_net_tx(void)
{
    struct netdriver_manager *pm = this_cpu_ptr(&netmanager);
    pm->canrun_tx = true;
    raise_softirq(NET_TX_SOFTIRQ);
}
void run_net_tx_softirq(struct softirq_action *paction)
{
    struct netdriver_manager *pm = this_cpu_ptr(&netmanager);
    struct net_callback *p;
    if (!(p=pm->phead)) {
        return;
    }
    int loop=0;
    while (pm->canrun_tx && ++loop<10) {
        pm->canrun_tx = 0;
        while (p){
            p->tx_callback(p->driver, p->param);
            p = p->pnext;
        }
    }

}
void run_net_rx_softirq(struct softirq_action *paction)
{
    struct netdriver_manager *pm = this_cpu_ptr(&netmanager);
    struct net_callback *p;
    if (!(p=pm->phead)) {
        return;
    }
    int loop=0;
    while (pm->canrun_rx && ++loop<10) {
        pm->canrun_tx = 0;
        while (p){
            p->rx_callback(p->driver, p->param);
            p = p->pnext;
        }
    }

}


__init int static init_net(bool isbp)
{
    struct netdriver_manager *p = this_cpu_ptr(&netmanager);

    for (int i = 0; i < NR_NETDRIVER_BUF - 1; i++) {
        p->netdrivers[i].pnext = &p->netdrivers[i + 1];
    }
    p->netdrivers[NR_NETDRIVER_BUF - 1].pnext = NULL;

    p->pfree = &p->netdrivers[0];
    p->phead = NULL;
    if (isbp) {
        open_softirq(NET_TX_SOFTIRQ, run_net_tx_softirq);
        open_softirq(NET_RX_SOFTIRQ, run_net_rx_softirq);
    }
    return 0;
}

early_initcall(init_net);

