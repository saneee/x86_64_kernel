#ifndef _YAOS_EXPORT_H
#define _YAOS_EXPORT_H 1
#define EXPORT_SYMBOL(sym)                            \
        extern typeof(sym) sym;


#endif
