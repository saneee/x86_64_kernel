#ifndef _YAOS_TIME_H
#define _YAOS_TIME_H
#include <asm/time.h>
struct rtcdate {
    uint second;
    uint minute;
    uint hour;
    uint day;
    uint month;
    uint year;
};
extern int set_timeout(unsigned long ms, void (*p) (unsigned long nowmsec));
extern int set_timeout_nsec(unsigned long ns,void (*p) (unsigned long nownsec));

time64_t mktime64(const unsigned int year0, const unsigned int mon0,
                  const unsigned int day, const unsigned int hour,
                  const unsigned int min, const unsigned int sec);
#ifdef ARCH_HRT_UPTIME
#define hrt_uptime ARCH_HRT_UPTIME
#endif
#ifdef ARCH_HRT_SET_TIMEOUT
#define hrt_set_timeout ARCH_HRT_SET_TIMEOUT
#endif
#endif
