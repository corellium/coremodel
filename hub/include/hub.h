#ifndef _HUB_H
#define _HUB_H

#include <nshostport.h>
#include <coremodel.h>
#include <stdio.h>
#include <stddef.h>
#include <rbtree.h>

#ifndef HUB_LOGLEVEL
#define HUB_LOGLEVEL    HUB_LOGLEVEL_INFO
#endif

#define HUB_LOGLEVEL_ERROR      0
#define HUB_LOGLEVEL_INFO       1
#define HUB_LOGLEVEL_DEBUG      2

#ifndef HUB_LOGLEVEL
    #define HUB_LOGLEVEL HUB_LOGLEVEL_INFO
#endif

#ifdef HUB_LOGCOLOR
    #define HUB_PRINT_COLOR     "\033[93m"
    #define HUB_ERROR_COLOR     "\033[31m"
    #define HUB_COLOR_END       "\033[0m"
#else
    #define HUB_PRINT_COLOR     ""
    #define HUB_ERROR_COLOR     ""
    #define HUB_COLOR_END       ""
#endif

#if HUB_LOGLEVEL >= HUB_LOGLEVEL_DEBUG
    #define pr_debug(_fmt, ...)\
        fprintf(stderr, HUB_PRINT_COLOR _fmt HUB_COLOR_END , ## __VA_ARGS__)
#else
    #define pr_debug(...)   do{}while(0)
#endif

#if HUB_LOGLEVEL >= HUB_LOGLEVEL_INFO
    #define pr_info(_fmt, ...)\
        fprintf(stderr, HUB_PRINT_COLOR _fmt HUB_COLOR_END , ## __VA_ARGS__)
#else
    #define pr_info(...)    do{}while(0)
#endif

#define pr_error(_fmt, ...)\
    fprintf(stderr, HUB_ERROR_COLOR _fmt HUB_COLOR_END , ## __VA_ARGS__)

/* Generic to-string macro */
#define xstr(s) str(s)
#define str(s) #s

#define container_of(_ptr, _type, _member)({\
    ((_type *)(((void *) _ptr) - offsetof(_type, _member)));\
})

#define ARRAY_SIZE(_arr)    (sizeof(_arr)/sizeof(_arr[0]))

/* Tuple offset definitions */
#define TUPLE_NAME_IDX  0
#define TUPLE_IP_IDX    1
#define TUPLE_PORT_IDX  2
#define TUPLE_NELM      3

/* Exported interfaces */
extern const coremodel_gpio_func_t vif_gpio;
extern const coremodel_can_func_t vif_can;

/*
 * Conver the CoreModel client credentials to a CoreModel name to open
 * @param tuple Components of the name as returned from hub_parse_tuple()
 * @return NULL on failure
 */
char *coremodel_mktarget(char **tuple);

/*
 * Open a CoreModel connection to a remote target, possibly entering a netns
 * @param cm Pointer to where to store the CoreModel hadnle
 * @return -1 on failure
 */
int coremodel_connect_nswrap(void **cm, char *target);

/*
 * Convert CoreModel integer types to string name
 * @param type One of COREMODEL_[UART|GPIO|...] etc
 * @return Corresponding CoreModel string name, NULL otherwise
 */
char *coremodel_type(unsigned type);

/* A single net */
struct netlist {
    struct netlist *head;
    struct netlist *next;
};

/*
 * A node representing a single exported CoreModel interface of a
 * VM instance
 */
struct vif_node {
    rbnode_t node;
    uint32_t flags;         /* Misc flags */
    void *cli;              /* Pointer to client that exports this vif interface */
    int type;               /* GPIO, CAN, etc */
    char *name;             /* Name of node format - vmname:interface:instance */
    char **tuple;           /* Tuple of name ^ */
    void *handle;           /* CoreModel interface handle */
    unsigned ref;           /* CoreModel interface client count */
    struct netlist net;     /* Linkage inside the netlist */
    struct edge {           /* Connected edges in the graph */
        struct edge *next;
        struct vif_node *node;
    } *edge;
};

#endif // _HUB_H
