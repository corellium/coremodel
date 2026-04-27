#ifndef _ETH_SWITCH_H
#define _ETH_SWITCH_H

#include <stdio.h>
#include <stddef.h>

#ifndef ETH_SWITCH_LOGLEVEL
#define ETH_SWITCH_LOGLEVEL    ETH_SWITCH_LOGLEVEL_INFO
#endif

#define ETH_SWITCH_LOGLEVEL_ERROR      0
#define ETH_SWITCH_LOGLEVEL_INFO       1
#define ETH_SWITCH_LOGLEVEL_DEBUG      2

#ifndef ETH_SWITCH_LOGLEVEL
    #define ETH_SWITCH_LOGLEVEL ETH_SWITCH_LOGLEVEL_INFO
#endif

#ifdef ETH_SWITCH_LOGCOLOR
    #define ETH_SWITCH_PRINT_COLOR     "\033[93m"
    #define ETH_SWITCH_ERROR_COLOR     "\033[31m"
    #define ETH_SWITCH_DEBUG_COLOR     "\033[35m"
    #define ETH_SWITCH_COLOR_END       "\033[0m"
#else
    #define ETH_SWITCH_PRINT_COLOR     ""
    #define ETH_SWITCH_ERROR_COLOR     ""
    #define ETH_SWITCH_DEBUG_COLOR     ""
    #define ETH_SWITCH_COLOR_END       ""
#endif

#if ETH_SWITCH_LOGLEVEL >= ETH_SWITCH_LOGLEVEL_DEBUG
    #define pr_debug(_fmt, ...)\
        fprintf(stderr, ETH_SWITCH_DEBUG_COLOR _fmt ETH_SWITCH_COLOR_END , ## __VA_ARGS__)
#else
    #define pr_debug(...)   do{}while(0)
#endif

#if ETH_SWITCH_LOGLEVEL >= ETH_SWITCH_LOGLEVEL_INFO
    #define pr_info(_fmt, ...)\
        fprintf(stderr, ETH_SWITCH_PRINT_COLOR _fmt ETH_SWITCH_COLOR_END , ## __VA_ARGS__)
#else
    #define pr_info(...)    do{}while(0)
#endif

#define pr_error(_fmt, ...)\
    fprintf(stderr, ETH_SWITCH_ERROR_COLOR _fmt ETH_SWITCH_COLOR_END , ## __VA_ARGS__)

/* Generic to-string macro */
#define xstr(s) str(s)
#define str(s) #s

/* Tuple offset definitions */
#define TUPLE_IP_IDX    0
#define TUPLE_PORT_IDX  1
#define TUPLE_NAME_IDX  2
#define TUPLE_NELM      3

void hexdump(void *buf, size_t len);

#endif // _ETH_SWITCH_H
