#ifndef _YAOS_TIME_H
#define _YAOS_TIME_H
struct rtcdate {
    uint second;
    uint minute;
    uint hour;
    uint day;
    uint month;
    uint year;
};
void set_timeout(unsigned long ms, void (*p) (unsigned long nowmsec));
time64_t mktime64(const unsigned int year0, const unsigned int mon0,
                  const unsigned int day, const unsigned int hour,
                  const unsigned int min, const unsigned int sec);
#endif
