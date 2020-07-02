#ifndef _YAOS_LOCK_H
#define _YAOS_LOCK_H
#include <yaos/irq.h>
#include <yaos/spinlock.h>
static inline ulong enter_softirq_lock()
{
    return local_irq_save();
}
static inline ulong leave_softirq_lock(ulong flag)
{
    local_irq_restore(flag);
    return 0;
}
static inline bool enter_spin_lock(__thread spinlock_t *p)
{
    spin_lock(p);
    return true;
}
static inline bool leave_spin_lock(__thread spinlock_t *p)
{
    spin_unlock(p);
    return false;
}
static inline ulong enter_softirq_spin_lock(__thread spinlock_t *p)
{
    ulong flag = local_irq_save();
    spin_lock(p);
    return flag;
}
static inline ulong leave_softirq_spin_lock(__thread spinlock_t *p,ulong flag)
{
    spin_unlock(p);
    local_irq_restore(flag);
    return 0;
}

#define WITH_SOFTIRQ_LOCK() \
     for(ulong _flag=enter_softirq_lock();_flag;_flag=leave_softirq_lock(_flag))

#define WITH_SPIN_LOCK(spin) \
     for (bool _locked = enter_spin_lock(&(spin));_locked;\
     _locked = leave_spin_lock(&(spin)))

#define WITH_SOFTIRQ_SPIN_LOCK(spin) \
     for(ulong _flag=enter_softirq_spin_lock(&(spin));\
          _flag;_flag=leave_softirq_spin_lock(&(spin),_flag))

#endif
