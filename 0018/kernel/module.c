#include <yaos/types.h>
#include <yaos/printk.h>
#include <yaos/module.h>
#include <yaos/sysparam.h>
#include <yaos/yaoscall.h>
#include <yaos/percpu.h>
#include <yaos/init.h>
#if 1
#define DEBUG_PRINT printk
#else
#define DEBUG_PRINT inline_printk
#endif

#define MODULE_INIT 0
#define MODULE_LOADED 1

struct module {
    int status;                 /* status */
    int id;                     /* unique id number */
    const char *name;           /* module name */
    modeventhand_t handler;     /* event handler */
    void *arg;                  /* argument for handler */
    modspecific_t data;         /* module specific data */
};
static struct module all[MAX_MODULE_NR];
static DEFINE_PER_CPU(int,percpu_status[MAX_MODULE_NR]);
static int total = 0;

__used static int dummy_module(module_t m, ulong t, void *arg)
{
    return MOD_ERR_OK;
}

DECLARE_MODULE(dummy1, 0, dummy_module);
DECLARE_MODULE(dummy2, 0, dummy_module);
static inline ulong myabs(ulong a, ulong b)
{
    return a > b ? a - b : b - a;
}

__init static int init_module()
{
    extern moduledata_t _module_data_start[];	//kernel64.ld
    extern moduledata_t _module_data_end[];
    ulong step =
        myabs((ulong) & _module_data_dummy1, (ulong) & _module_data_dummy2);

    moduledata_t *p = _module_data_start;
    int i = 0;
    bool found = false;
    int *pstatus = (int *)((ulong)(&percpu_status[0])+(ulong)__per_cpu_load);
    while (p < _module_data_end) {
        int ret;

        if (!p->name || !p->evhand) {
            DEBUG_PRINT("zero module:%lx,%s,%lx,%lx\n", p, p->name, p->evhand,
                        p->priv);

            p = (moduledata_t *) ((ulong) p + step);
            continue;
        }
        all[i].status = 0;
        all[i].id = i;
        all[i].name = p->name;
        all[i].handler = p->evhand;
        all[i].arg = p->priv;
        all[i].data.ulongval = 0;
        pstatus[i] = MODULE_INIT;
        DEBUG_PRINT("adding module:step:%d,%lx,%s,%lx\n", step, p, p->name,
                    p->evhand);
        ret = (all[i].handler) (&all[i], MOD_BPLOAD, all[i].arg);
        if (MOD_ERR_OK == ret) {
            all[i].status = MODULE_LOADED;
        }
        else if (ret == MOD_ERR_NEXTLOOP) {

            found = true;
        }
        i++;
        p = (moduledata_t *) ((ulong) p + step);
    }
    total = i;
    DEBUG_PRINT("total modules:%d\n", total);
    int times = 10;

    while (found && --times) {
        found = false;
        for (i = 0; i < total; i++) {
            if (all[i].status == MODULE_INIT) {
                int ret = (all[i].handler) (&all[i], MOD_BPLOAD, all[i].arg);

                if (MOD_ERR_OK == ret) {
                    all[i].status = MODULE_LOADED;
                }
                else if (ret == MOD_ERR_NEXTLOOP) {

                    found = true;
                }

            }
        }
    }
    if (found) {
        DEBUG_PRINT("some module want to be loaded more times\n");
    }
    return 0;
}

__init static int init_module_ap()
{
    bool found = false;

    DEBUG_PRINT("init_module_ap start\n");
    int *pstatus = PERCPU_PTR(&percpu_status[0]);

    for (int i = 0; i < total; i++) {
        if (all[i].status == MODULE_LOADED && pstatus[i] == MODULE_INIT) {
            int ret = (all[i].handler) (&all[i], MOD_APLOAD, all[i].arg);

            if (MOD_ERR_OK == ret) {
                pstatus[i] = MODULE_LOADED;

            }
            else if (ret == MOD_ERR_NEXTLOOP) {

                found = true;
            }

        }
    }
    int times = 10;

    while (found && --times) {
        found = false;
        for (int i = 0; i < total; i++) {
            if (all[i].status == MODULE_LOADED && pstatus[i] == MODULE_INIT) {
                int ret = (all[i].handler) (&all[i], MOD_APLOAD, all[i].arg);

                if (MOD_ERR_OK == ret) {
                    pstatus[i] = MODULE_LOADED;

                }
                else if (ret == MOD_ERR_NEXTLOOP) {

                    found = true;
                }

            }
        }
    }
    if (found) {
        DEBUG_PRINT("some module want to be loaded more times\n");
    }
    return 0;
}

static int init_module_call(bool isbp)
{
    if (isbp) {
        init_module();
    }
    else {
        init_module_ap();
    }
    return 0;
}

pure_initcall(init_module_call);
