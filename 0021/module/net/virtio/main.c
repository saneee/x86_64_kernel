#include <yaos/types.h>
#include <yaos/module.h>
#include <yaos/printk.h>
DECLARE_MODULE(virtio_net_drivers, 0, main);
void init_virtio_net();


__used static int main(module_t m, ulong t, void *arg)
{
    modeventtype_t env = (modeventtype_t) t;
    printk("virtiomain env:%d,%d\n",env,MOD_BPDEVICE);
    if (env == MOD_BPLOAD) {
        printk("virtio_net_drivers, %lx,%lx,%lx\n", m, t, arg);
    } else if(env == MOD_BPDEVICE) {
        printk("init_virtio_net\n");
        init_virtio_net();
    }
    return MOD_ERR_OK;          //MOD_ERR_NEXTLOOP;
}

