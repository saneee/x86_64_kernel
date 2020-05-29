#ifndef _DRIVER_VIRTIO_RING_H
#define _DRIVER_VIRTIO_RING_H
/* This marks a buffer as continuing via the next field. */
#define VRING_DESC_F_NEXT       1
/* This marks a buffer as write-only (otherwise read-only). */
#define VRING_DESC_F_WRITE      2
/* This means the buffer contains a list of buffer descriptors. */
#define VRING_DESC_F_INDIRECT   4

/* The Host uses this in used->flags to advise the Guest: don't kick me when
 * you add a buffer.  It's unreliable, so it's simply an optimization.  Guest
 * will still kick if it's out of buffers. */
#define VRING_USED_F_NO_NOTIFY  1
/* The Guest uses this in avail->flags to advise the Host: don't interrupt me
 * when you consume a buffer.  It's unreliable, so it's simply an
 * optimization.  */
#define VRING_AVAIL_F_NO_INTERRUPT      1
/* We support indirect buffer descriptors */
#define VIRTIO_RING_F_INDIRECT_DESC     28

/* The Guest publishes the used index for which it expects an interrupt
 * at the end of the avail ring. Host should ignore the avail->flags field. */
/* The Host publishes the avail index for which it expects a kick
 * at the end of the used ring. Guest should ignore the used->flags field. */
#define VIRTIO_RING_F_EVENT_IDX         29

/* Virtio ring descriptors: 16 bytes.  These can chain together via "next". */
struct vring_desc {
        /* Address (guest-physical). */
        u64 addr;
        /* Length. */
        u32 len;
        /* The flags as indicated above. */
        u16 flags;
        /* We chain unused descriptors via this, too */
        u16 next;
};
struct vring_avail {
        u16 flags;
        u16 idx;
        u16 ring[];
};

/* u32 is used here for ids for padding reasons. */
struct vring_used_elem {
        /* Index of start of used descriptor chain. */
        u32 id;
        /* Total length of the descriptor chain which was used (written to) */
        u32 len;
};

struct vring_used {
        u16 flags;
        u16 idx;
        struct vring_used_elem ring[];
};
struct vring {
        unsigned int num;

        struct vring_desc *desc;

        struct vring_avail *avail;

        struct vring_used *used;
};

#define vring_used_event(vr) ((vr)->avail->ring[(vr)->num])
#define vring_avail_event(vr) (*(u16 *)&(vr)->used->ring[(vr)->num])

static inline void vring_init(struct vring *vr, unsigned int num, void *p,
                              unsigned long align)
{
        vr->num = num;
        vr->desc = p;
        vr->avail = (void *)((char *)p + num*sizeof(struct vring_desc));
        vr->used = (void *)(((unsigned long)&vr->avail->ring[num] + sizeof(u16)
                + align-1) & ~(align - 1));
}
static inline unsigned vring_size(unsigned int num, unsigned long align)
{
        return ((sizeof(struct vring_desc) * num + sizeof(u16) * (3 + num)
                 + align - 1) & ~(align - 1))
                + sizeof(u16) * 3 + sizeof(struct vring_used_elem) * num;
}


#endif
