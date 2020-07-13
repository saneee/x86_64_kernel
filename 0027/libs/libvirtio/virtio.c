#include <yaos/types.h>
#include <asm/apic.h>
#include <drivers/virtio.h>
#include <yaos/virtio_ring.h>
#include <asm/apic.h>
#include <yaoscall/page.h>      //yaos_heap_alloc_4k
#include <yaoscall/malloc.h>    //yaos_malloc
#include <errno.h>
#include <yaos/assert.h>
#include <string.h>
#include <yaos/msi.h>
#include "virtio_ring.h"
#define MAPVEC_NR 64
#define PAGE_SIZE_4K 0x1000
#define max_virtqueues_nr 64
#define VRING_AVAIL_F_NO_INTERRUPT 1
#undef pci_d
#define pci_d(...) printk(__VA_ARGS__)

extern void set_data_break(ulong addr);
bool showTcp = false;
/*
 * About indirect descriptors:
 *
 * For each possible thread, a single indirect descriptor table is allocated.
 * If using direct descriptors would lead to the situation that another thread
 * might not be able to add another descriptor to the ring, indirect descriptors
 * are used.
 *
 * Indirect descriptors are pre-allocated. Each alloc_contig() call involves a
 * kernel call which is critical for performance.
 *
 * The size of indirect descriptor tables is chosen based on MAPVEC_NR. A driver
 * using this library should never add more than
 *
 *    MAPVEC_NR + MAPVEC_NR / 2
 *
 * descriptors to a queue as this represent the maximum size of an indirect
 * descriptor table.
 */

struct indirect_desc_table {
    int in_use;
    struct vring_desc *descs;
    phys_bytes paddr;
    size_t len;
};
struct virtio_queue {

    void *vaddr;                /* virtual addr of ring */
    phys_bytes paddr;           /* physical addr of ring */
    u32_t page;                 /* physical guest page */

    u16_t num;                  /* number of descriptors */
    u16_t index;
    u32_t ring_size;            /* size of ring in bytes */
    unsigned last_used_idx;

    struct vring vring;

    u16_t free_num;             /* free descriptors */
    u16_t free_head;            /* next free descriptor */
    u16_t free_tail;            /* last free descriptor */
    u16_t last_used;            /* we checked in used */
    u16_t * avail_event;
    u16_t * used_event;
    void (*callback)(void *data, size_t len);
    void **data;                /* points to pointers */
};

bool msix_unmask_entry(pci_device_t pci, int);
void msix_unmask_all(pci_device_t pci);
bool msix_mask_entry(pci_device_t pci, int entry_id);
static void
set_indirect_descriptors(virtio_dev_t dev, struct virtio_queue *q,
                         struct addr_size *bufs, size_t num);
static void
set_direct_descriptors(struct virtio_queue *q, struct addr_size *bufs,
                       size_t num);
void virtio_setup_features(virtio_dev_t p);
void virtio_conf_read(virtio_dev_t pd, u32 offset, void *buf, int length)
{
    unsigned char *ptr = (unsigned char *)(buf);

    for (int i = 0; i < length; i++)
        ptr[i] = pci_bar_readb(pd->_bar1, offset + i);
}

void virtio_set_queue_callback(virtio_dev_t dev, int qidx, void (*callback)(void * data,size_t len))
{
    struct virtio_queue *q = &dev->queues[qidx];
    q->callback = callback;
}

void virtio_conf_write(virtio_dev_t pd, u32 offset, void *buf, int length)
{
    u8 *ptr = (u8 *) (buf);

    for (int i = 0; i < length; i++)
        pci_bar_writeb(pd->_bar1, offset + i, ptr[i]);
}
bool virtio_had_irq(virtio_dev_t dev)
{
    return virtio_conf_readb(dev,VIRTIO_PCI_ISR)&1;
}
static void clear_indirect_table(virtio_dev_t dev, struct vring_desc *vd)
{
    int i;
    struct indirect_desc_table *desc;

    ASSERT(vd->len > 0);
    ASSERT(vd->flags & VRING_DESC_F_INDIRECT);
    vd->flags = vd->flags & ~VRING_DESC_F_INDIRECT;
    vd->len = 0;;

    for (i = 0; i < dev->num_indirect; i++) {
        desc = &dev->indirect[i];

        if (desc->paddr == vd->addr) {
            ASSERT(desc->in_use);
            desc->in_use = 0;
            break;
        }
    }
    if (i >= dev->num_indirect)
        panic("Could not clear indirect descriptor table ");
}

static int wants_kick(struct virtio_queue *q)
{
    ASSERT(q != NULL);
    //pci_d("wants_kick:q:%lx,aflags:%x,%d,avl:%d,used:%d,event:%lx,usedevent:%lx\n", q,q->vring.avail->flags,!(q->vring.used->flags & VRING_USED_F_NO_NOTIFY),q->vring.avail->idx,q->vring.used->idx,*q->avail_event,*q->used_event);
    if (q->vring.avail->idx  >q->vring.used->idx + 5)  showTcp  = true;
    else showTcp = false;
    return true;
    return !(q->vring.used->flags & VRING_USED_F_NO_NOTIFY);
}

static void kick_queue(virtio_dev_t dev, int qidx)
{
    ASSERT(0 <= qidx && qidx < dev->num_queues);

    if (wants_kick(&dev->queues[qidx]))
        virtio_conf_writew(dev, VIRTIO_PCI_QUEUE_NOTIFY, qidx);
     

    return;
}
void virtio_pci_device_init(virtio_dev_t pd)
{
    if (!virtio_parse_pci_config(pd)) {
        panic("can't virtio parse pci config");
    }
    set_pci_bus_master(to_pci_device_t(pd), true);
    pci_msix_enable(to_pci_device_t(pd));

}
void virtio_driver_init(virtio_dev_t pd)
{
    virtio_pci_device_init(pd);
    virtio_reset_host_side(pd);
    virtio_add_dev_status(pd, VIRTIO_CONFIG_S_ACKNOWLEDGE);
    virtio_add_dev_status(pd, VIRTIO_CONFIG_S_DRIVER);

}

bool virtio_parse_pci_config(virtio_dev_t p)
{
    pci_device_parse_config((to_pci_device_t(p)));
    p->_bar1 = pci_get_bar(&p->dev, 1);
    if (!p->_bar1) {
        pci_d("p->_bar1:%d zero\n", p->_bar1);
        return false;
    }                           // Check ABI version
    u8 rev = pci_get_revision_id(to_pci_device_t(p));

    if (rev != VIRTIO_PCI_ABI_VERSION) {
        panic("Wrong virtio revision=%x", rev);
        return false;
    }

    // Check device ID
    u16 dev_id = pci_get_device_id(to_pci_device_t(p));

    if ((dev_id < VIRTIO_PCI_ID_MIN) || (dev_id > VIRTIO_PCI_ID_MAX)) {
        panic("Wrong virtio dev id %x", dev_id);
        return false;
    }
    
    return true;

}

static u64 virtio_get_device_features(virtio_dev_t p)
{
    return virtio_conf_readl(p, VIRTIO_PCI_HOST_FEATURES);
}

static void virtio_set_guest_features(virtio_dev_t p, u64 features)
{
    virtio_conf_writel(p, VIRTIO_PCI_GUEST_FEATURES, features);
    p->_enabled_features = features;
}
u8 get_dev_status(virtio_dev_t p)
{
    return virtio_conf_readb(p,VIRTIO_PCI_STATUS);
}
void set_dev_status(virtio_dev_t p, u8 status)
{
    virtio_conf_writeb(p,VIRTIO_PCI_STATUS,status);
}
void add_dev_status(virtio_dev_t p, u8 status)
{
    set_dev_status(p, get_dev_status(p) | status);
}
void del_dev_status(virtio_dev_t p, u8 status)
{
    set_dev_status(p, get_dev_status(p) & ~status);
}

void virtio_setup_features(virtio_dev_t p)
{
    u64 dev_features = virtio_get_device_features(p);
    u64 drv_features = p->driver_features;

    u64 subset = dev_features & drv_features;

    //notify the host about the features in used according
    //to the virtio spec
    for (int i = 0; i < 64; i++)
        if (subset & (1 << i))
            pci_d("%s: found feature intersec of bit %d\n", __FUNCTION__, i);

    if (subset & (1 << VIRTIO_RING_F_INDIRECT_DESC))
        p->_cap_indirect_buf = true;

    if (subset & (1 << VIRTIO_RING_F_EVENT_IDX))
        p->_cap_event_idx = true;
    pci_d("virtio_set_guest_features subset:%x\n", subset);
    virtio_set_guest_features(p, subset);
}

void virtio_irq_register(virtio_dev_t p)
{
    int irq = pci_get_interrupt_line(to_pci_device_t(p));
    pci_msix_enable(to_pci_device_t(p));
    ioapic_enable(irq, 0);
    struct msi_msg msg; 
    msi_compose_msg(58,0,&msg);
    pci_d("msix_msg addr:%x%08x,data:%lx\n", msg.address_hi,msg.address_lo, msg.data);
    
    pci_msix_write_entry(to_pci_device_t(p), 0, &msg);
    msi_compose_msg(59, 0,&msg);

    pci_msix_write_entry(to_pci_device_t(p), 1, &msg);
    msix_unmask_entry(to_pci_device_t(p),0);
    msix_unmask_entry(to_pci_device_t(p),1);
    ioapic_enable(58, 0);
    ioapic_enable(59, 1);
}

void virtio_device_ready(virtio_dev_t p)
{

    /* Register IRQ line */
    //virtio_irq_register(p);

    /* Driver is ready to go! */
    add_dev_status(p, VIRTIO_CONFIG_S_DRIVER_OK);
}

int
virtio_to_queue(virtio_dev_t dev, int qidx, struct addr_size *bufs,
                size_t num, void *data)
{
    u16_t free_first;
    int left;
    struct virtio_queue *q = &dev->queues[qidx];
    struct vring *vring = &q->vring;

    ASSERT(0 <= qidx && qidx <= dev->num_queues);

    if (!data)
        panic("%s: NULL data received queue %d", __func__, qidx);
    if (q->free_num + 4 <q->num) {
        virtio_read_queue(dev,qidx);
    }
    free_first = q->free_head;

    left = (int)q->free_num - (int)num;
if(q->free_num<100){
    printk("q->free_num:%d,num:%d,qidx:%d\n",q->free_num,num,qidx);
}
    if (left<=0) {
        printk("free_num:%d, num:%d\n",q->free_num, num);
        return EAGAIN;
    }

    //pci_d("free_first:%016lx,left:%d,dev->threads:%d,n:%d\n",free_first,left,dev->threads,num);
    if (num >1 && left < dev->threads)
        set_indirect_descriptors(dev, q, bufs, num);
    else
        set_direct_descriptors(q, bufs, num);

    /* Next index for host is old free_head */
    vring->avail->ring[vring->avail->idx % q->num] = free_first;

    /* Provided by the caller to identify this slot */
    q->data[free_first] = data;

    /* Make sure the host sees the new descriptors */
    virtio_wmb(true);

    /* advance last idx */
    vring->avail->idx += 1;

    /* Make sure the host sees the avail->idx */
    virtio_rmb(true);

    /* kick it! */
    kick_queue(dev, qidx);
    return 0;
}

static void inline use_vring_desc(struct vring_desc *vd, struct addr_size *vp)
{
    vd->addr = vp->vp_addr;
    vd->len = vp->vp_size;
    vd->flags = VRING_DESC_F_NEXT;
    if (vp->vp_flags & VRING_DESC_F_WRITE) vd->flags |= VRING_DESC_F_WRITE;
    //pci_d("vd->addr:%lx,vd->len:%lx,vd->flags:%x\n",vd->addr,vd->len,vd->flags);
}

static void
set_indirect_descriptors(virtio_dev_t dev, struct virtio_queue *q,
                         struct addr_size *bufs, size_t num)
{
    /* Indirect descriptor tables are simply filled from left to right */
    int i;
    struct indirect_desc_table *desc=NULL;
    struct vring *vring = &q->vring;
    struct vring_desc *vd=NULL, *ivd=NULL;

    /* Find the first unused indirect descriptor table */
    for (i = 0; i < dev->num_indirect; i++) {
        desc = &dev->indirect[i];

        /* If an unused indirect descriptor table was found,
         * mark it as being used and exit the loop.
         */
        if (!desc->in_use) {
            desc->in_use = 1;
            break;
        }
    }
    ASSERT(desc);
    /* Sanity check */
    if (i >= dev->num_indirect)
        panic("No indirect descriptor tables left");

    /* For indirect descriptor tables, only a single descriptor from
     * the main ring is used.
     */
    vd = &vring->desc[q->free_head];
    vd->flags = VRING_DESC_F_INDIRECT;
    vd->addr = desc->paddr;
    vd->len = num * sizeof(desc->descs[0]);
    /* Initialize the descriptors in the indirect descriptor table */
    for (i = 0; i < (int)num; i++) {
        ivd = &desc->descs[i];

        use_vring_desc(ivd, &bufs[i]);
        ivd->next = i + 1;
    }
    ASSERT(ivd);
    /* Unset the next bit of the last descriptor */
    ivd->flags = ivd->flags & ~VRING_DESC_F_NEXT;

    /* Update queue, only a single descriptor was used */
    q->free_num -= 1;
    q->free_head = vd->next;
}

static void
set_direct_descriptors(struct virtio_queue *q, struct addr_size *bufs,
                       size_t num)
{
    u16_t i;
    size_t count;
    struct vring *vring = &q->vring;
    struct vring_desc *vd=NULL;
    ASSERT(num);
    for (i = q->free_head, count = 0; count < num; count++) {

        /* The next free descriptor */
        vd = &vring->desc[i];

        /* The descriptor is linked in the free list, so
         * it always has the next bit set.
         */
        if (!(vd->flags & VRING_DESC_F_NEXT)) {
            pci_d("i:%d,vd->flags:%x,used:%d,avail:%d\n",i,vd->flags,vring->used->idx,vring->avail->idx);
        }
        ASSERT(vd->flags & VRING_DESC_F_NEXT);

        use_vring_desc(vd, &bufs[count]);
        i = vd->next;
    }
    ASSERT(vd);
    /* Unset the next bit of the last descriptor */
    vd->flags = vd->flags & ~VRING_DESC_F_NEXT;

    /* Update queue */
    q->free_num -= num;
    q->free_head = i;
}
void virtio_get_buf_finalize(virtio_dev_t dev, int qidx) {
    struct virtio_queue *q;
    struct vring *vring;
    q = &dev->queues[qidx];
    vring = &q->vring;
    pci_d("avail flag:%x,used flag:%x,%x，last_used:%x ",vring->avail->flags,vring->used->flags,*q->used_event,q->last_used);
    pci_d("vring.used:%d,avail:%d,q->last_used:%d\n",vring->used->idx,vring->avail->idx,q->last_used);
    barrier();
    //*q->used_event = vring->used->idx;//q->last_used;
    barrier();
    pci_d("new :%x,%d\n",*q->used_event,q->last_used_idx);
    
}
void virtio_dump_queue(virtio_dev_t dev, int qidx)
{
    struct virtio_queue *q;
    struct vring *vring;
    q = &dev->queues[qidx];
    vring = &q->vring;
    printk("avail flag:%x,used flag:%x,used_event:%x，last_used:%x \n",vring->avail->flags,vring->used->flags,*q->used_event,q->last_used);
    printk("vring.used:%d,avail:%d,q->last_used:%d\n",vring->used->idx,vring->avail->idx,q->last_used);
    for (int i=vring->used->idx-2;i<vring->avail->idx;i++) {
       struct vring_desc *vd = &vring->desc[i % q->num]; 
       printk("i:%d,flag:%d\n",i,vd->flags);
       dump_mem((char*)P2V(vd->addr), vd->len);
    } 
}
static int virtio_from_queue(virtio_dev_t dev, int qidx, void **data, size_t * len)
{
    struct virtio_queue *q;
    struct vring *vring;
    struct vring_used_elem *uel;
    struct vring_desc *vd;
    int count = 0;
    u16_t idx;
    u16_t used_idx;

    ASSERT(0 <= qidx && qidx < dev->num_queues);

    q = &dev->queues[qidx];
    vring = &q->vring;
    /* Make sure we see changes done by the host */
    virtio_rmb(true);

    /* The index from the host */
    used_idx = vring->used->idx % q->num;

    /* We already saw this one, nothing to do here */
    if (q->last_used == used_idx)
        return -1;
    //virtio_queue_enable_intr(q);

    /* Get the vring_used element */
    uel = &q->vring.used->ring[q->last_used];

    /* Update the last used element */
    q->last_used = (q->last_used + 1) % q->num;

    /* index of the used element */
    idx = uel->id % q->num;

    ASSERT(q->data[idx] != NULL);

    /* Get the descriptor */
    vd = &vring->desc[idx];

    /* Unconditionally set the tail->next to the first used one */
    //pci_d("idx:%d,tail:%d,flag:%x\n", idx, q->free_tail, vring->desc[q->free_tail].flags);
    ASSERT(vring->desc[q->free_tail].flags & VRING_DESC_F_NEXT);
    vring->desc[q->free_tail].next = idx;

    /* Find the last index, eventually there has to be one
     * without a the next flag.
     *
     * FIXME: Protect from endless loop
     */
    while (vd->flags & VRING_DESC_F_NEXT) {

        if (vd->flags & VRING_DESC_F_INDIRECT)
            clear_indirect_table(dev, vd);

        idx = vd->next;
        vd = &vring->desc[idx];
        count++;
    }

    /* Didn't count the last one */
    count++;

    if (vd->flags & VRING_DESC_F_INDIRECT)
        clear_indirect_table(dev, vd);

    /* idx points to the tail now, update the queue */
    q->free_tail = idx;
    ASSERT(!(vd->flags & VRING_DESC_F_NEXT));

    /* We can always connect the tail with the head */
    vring->desc[q->free_tail].next = q->free_head;
    vring->desc[q->free_tail].flags = VRING_DESC_F_NEXT;

    q->free_num += count;
if(q->free_num<100)printk("q->free_num:%d,count:%d,qidx:%d\n",q->free_num, count, qidx);
    ASSERT(q->free_num <= q->num);

    *data = q->data[uel->id];
    q->data[uel->id] = NULL;
    if (len != NULL)
        *len = uel->len;
    q->last_used_idx++;
    virtio_queue_enable_intr(q);
    if (!(vring->avail->flags & VRING_AVAIL_F_NO_INTERRUPT))
        virtio_store_mb(true,q->used_event,q->last_used_idx);

    return 0;
}
int virtio_read_queue(virtio_dev_t dev, int qidx)
{
    void *data;
    size_t len;
    struct virtio_queue *q;
    ASSERT(0 <= qidx && qidx < dev->num_queues);

    q = &dev->queues[qidx];

    while(virtio_from_queue(dev, qidx, &data, &len)==0) {
        (*q->callback)(data, len);    
    }
    return 0;
}

static void init_phys_queue(struct virtio_queue *q)
{
    memset(q->vaddr, 0, q->ring_size);
    memset(q->data, 0, sizeof(q->data[0]) * q->num);

    /* physical page in guest */
    q->page = q->paddr / PAGE_SIZE_4K;

    /* Set pointers in q->vring according to size */
    vring_init(&q->vring, q->num, q->vaddr, PAGE_SIZE_4K);

    /* Everything's free at this point */
    for (int i = 0; i < q->num; i++) {
        q->vring.desc[i].flags = VRING_DESC_F_NEXT;
        q->vring.desc[i].next = (i + 1) & (q->num - 1);
    }
    q->free_num = q->num;
    q->free_head = 0;
    q->free_tail = q->num - 1;
    q->last_used = 0;
    q->avail_event = (u16_t *)vring_avail_event(&q->vring);
    q->used_event  = (u16_t *)vring_used_event(&q->vring);
    q->last_used_idx=0;
    pci_d("%lx,num:%d used_event:%016lx, avail_event:%016lx,avail:%016lx,user:%016lx\n",q,q->vring.num,q->used_event,q->avail_event,q->vring.avail->ring,q->vring.used->ring);
    return;
}
static void setup_queue(virtio_dev_t dev, struct virtio_queue *q)
{
    if (pci_is_msix(to_pci_device_t(dev))) {
        virtio_conf_writew(dev, VIRTIO_MSI_QUEUE_VECTOR, q->index);
        if (virtio_conf_readw(dev, VIRTIO_MSI_QUEUE_VECTOR) != q->index) {
            panic("Setting MSIx entry for queue %d failed.",q->index);
        }
    }
    virtio_conf_writel(dev, VIRTIO_PCI_QUEUE_PFN, (u32)(q->paddr >> VIRTIO_PCI_QUEUE_ADDR_SHIFT));
}
static int alloc_phys_queue(struct virtio_queue *q)
{
    ASSERT(q != NULL);

    /* How much memory do we need? */
    q->ring_size = vring_size(q->num, PAGE_SIZE_4K);
    q->vaddr =
        yaos_heap_alloc_4k((q->ring_size + PAGE_SIZE_4K - 1) & ~(PAGE_SIZE_4K -
	                         1));
    if (q->vaddr == NULL)
        return ENOMEM;
    q->paddr = (phys_bytes) yaos_v2p(q->vaddr);
    pci_d("q->ring_size:0x%x,q->vaddr:%lx,q->paddr:%lx,num:%d\n", q->ring_size,
          q->vaddr, q->paddr, q->num);

    q->data = yaos_heap_alloc_4k(PAGE_SIZE_4K);
    ASSERT(sizeof(q->data[0]) * q->num < PAGE_SIZE_4K);

    if (q->data == NULL) {
        panic("No memory:%s,line:%d\n", __func__, __LINE__);

        return ENOMEM;
    }

    return OK;
}

static void free_phys_queue(struct virtio_queue *q)
{
    ASSERT(q != NULL);
    ASSERT(q->vaddr != NULL);

    //free_contig(q->vaddr, q->ring_size);
    q->vaddr = NULL;
    q->paddr = 0;
    q->num = 0;
    //free_contig(q->data, sizeof(q->data[0]));
    q->data = NULL;
}

void virtio_free_queues(virtio_dev_t dev)
{
    int i;

    ASSERT(dev != NULL);
    ASSERT(dev->queues != NULL);
    ASSERT(dev->num_queues > 0);

    for (i = 0; i < dev->num_queues; i++)
        free_phys_queue(&dev->queues[i]);

    dev->num_queues = 0;
    dev->queues = NULL;
}

static int init_phys_queues(virtio_dev_t dev)
{
    /* Initialize all queues */
    int i, j, r;
    struct virtio_queue *q;

    for (i = 0; i < dev->num_queues; i++) {
        q = &dev->queues[i];
        /* select the queue */
        virtio_conf_writew(dev, VIRTIO_PCI_QUEUE_SEL, i);
        q->num = virtio_conf_readw(dev, VIRTIO_PCI_QUEUE_NUM);
        q->index = i;

        if (q->num & (q->num - 1)) {
            pci_d(" Queue %d num=%d not ^2", i, q->num);
            r = EINVAL;
            goto free_phys_queues;
        }
        if ((r = alloc_phys_queue(q)) != OK)
            goto free_phys_queues;

        init_phys_queue(q);
        setup_queue(dev, q);
    }

    return OK;

/* Error path */
  free_phys_queues:
    for (j = 0; j < i; j++)
        free_phys_queue(&dev->queues[i]);

    return r;
}

int virtio_alloc_queues(virtio_dev_t dev, int num_queues)
{
    int r = OK;

    /* Assume there's no device with more than 256 queues */
    if (num_queues < 0 || num_queues > 256)
        return EINVAL;

    dev->num_queues = num_queues;
    /* allocate queue memory */
    dev->queues = yaos_malloc(num_queues * sizeof(dev->queues[0]));

    if (dev->queues == NULL)
        return ENOMEM;

    memset(dev->queues, 0, num_queues * sizeof(dev->queues[0]));

    if ((r = init_phys_queues(dev) != OK)) {
        printf(" Could not initialize queues (%d)\n", r);
        yaos_mfree(dev->queues);
        dev->queues = NULL;
    }

    return r;
}

static int init_indirect_desc_table(struct indirect_desc_table *desc)
{
    desc->in_use = 0;
    desc->len = (MAPVEC_NR + MAPVEC_NR / 2) * sizeof(struct vring_desc);
    ASSERT(desc->len < PAGE_SIZE_4K);
    desc->descs = yaos_heap_alloc_4k(PAGE_SIZE_4K);
    if (desc->descs == NULL)
        return ENOMEM;

    desc->paddr = (phys_bytes) yaos_v2p(desc->descs);

    memset(desc->descs, 0, desc->len);

    return OK;
}

int init_indirect_desc_tables(virtio_dev_t dev)
{
    int i, r;
    struct indirect_desc_table *desc;

    dev->indirect = yaos_malloc(dev->num_indirect * sizeof(dev->indirect[0]));

    if (dev->indirect == NULL) {
        panic("No memory:%s,line:%d\n", __func__, __LINE__);

        return ENOMEM;
    }

    memset(dev->indirect, 0, dev->num_indirect * sizeof(dev->indirect[0]));

    for (i = 0; i < dev->num_indirect; i++) {
        desc = &dev->indirect[i];
        if ((r = init_indirect_desc_table(desc)) != OK) {
            panic("No memory:%s,line:%d\n", __func__, __LINE__);

            return r;
        }
    }

    return OK;

}
void virtio_queue_disable_intr(struct virtio_queue *q)
{
   struct vring *vring = &q->vring;
   virtio_wmb(true);
   vring->avail->flags= VRING_AVAIL_F_NO_INTERRUPT;

}
struct virtio_queue * virtio_get_queue(virtio_dev_t dev, int qidx) {
    return &dev->queues[qidx];
}
void virtio_queue_enable_intr(struct virtio_queue *q)
{
   struct vring *vring = &q->vring;
   virtio_wmb(true);
   vring->avail->flags= 0;

}

void virtio_probe_queues(virtio_dev_t p)
{
   p->num_queues = 0;
   u16 qsize = 0;
   do {
       if (p->num_queues >= max_virtqueues_nr) return;

       virtio_conf_writew(p, VIRTIO_PCI_QUEUE_SEL, p->num_queues);
       qsize = virtio_conf_readw(p, VIRTIO_PCI_QUEUE_NUM);
       if (0 == qsize) break;
       
       p->num_queues++;       
     
       
   } while(1);
}
bool virtio_setup_device(virtio_dev_t p, uint16_t did, int threads)
{
    pci_set_device_id(to_pci_device_t(p), did);
    pci_set_vendor_id(to_pci_device_t(p), VIRTIO_VENDOR_ID);
    if (!find_pci(to_pci_device_t(p))) {
        pci_d("can't found virtio device:vid：%x，did:%x\n",
               VIRTIO_VENDOR_ID, did);
        return false;
    }
   
    //if (!virtio_parse_pci_config(p)) {
    //    pci_d("can't virtio parse pci config");
    //    return false;
    //}
    //virtio_conf_writeb(p, VIRTIO_PCI_STATUS, 0);
    //virtio_conf_writeb(p, VIRTIO_PCI_STATUS, VIRTIO_CONFIG_S_ACKNOWLEDGE);
    //virtio_conf_writeb(p, VIRTIO_PCI_STATUS, VIRTIO_CONFIG_S_DRIVER);
    //virtio_setup_features(p);
    virtio_driver_init(p);
    virtio_setup_features(p);
    virtio_probe_queues(p);
    virtio_alloc_queues(p, p->num_queues);
    p->num_indirect = threads;
    init_indirect_desc_tables(p);
    return true;
}
