#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <hub.h>
#include <rbtree.h>

#include <json.h>

/* Convenience function for casting a client structure */
#define ICLIENT(_cli)   ((struct client_int *)(_cli))
#define ECLIENT(_cli)   ((struct client_ext *)(_cli))

#define CLIENT_TYPE_INTERNAL    0
#define CLIENT_TYPE_EXTERNAL    1

/* Types if VIF walks */
#define VIF_WALK_NODE_RESET         0
#define VIF_WALK_LIST_GRAPH         1
#define VIF_WALK_CONSTRUCT_NETLIST  2

/* External client */
struct client_ext {
    int fd;
    uint8_t type;
};

/* Internal (cohort) client) */
struct client_int {
    char *name;
    void *cm;
};

/* Global hub state */
struct hub_state {
    rbtree_t rb;
    int lfd;
    void *cred;
    struct client {
        struct client *next;
        int type;
        char *cred;
        void *inner;
    } *clients;
    unsigned nclients;
    struct netlist *nets;
} g_state;

extern const char *__progname;
static int hub_usage(void)
{
    pr_info("usage %s [OPTIONS]\n"
            "\t-c name:if:idx,name:if:idx\tMake a connection between two exposed VM interfaces\n"
            "\t-f path\t\t\t\tConfiguration file path\n"
            "\t-h\t\t\t\tThis help text\n"
            "\t-i name:ip:port\t\t\tNetwork location of a VM to bridge together\n"
    , __progname);
    return -1;
}

static void hub_free_tuple(char **tuple)
{
    unsigned i = 0;

    while(tuple[i] != (void *)-1ull)
        free(tuple[i++]);
    free(tuple);
}

static char **hub_parse_tuple(const char *tuple, int len, char *sep)
{
    char **v, *s, *t;
    unsigned i = 0;

    s = t = strdup(tuple);
    if(!s)
        return NULL;

    v = calloc(sizeof(char *), len + 1);
    if(!v){
        free(s);
        return NULL;
    }

    while(*s)
        i += (*s++ == *sep);

    i = len - i - 1;
    if((s = strtok(t, sep)))
        do{
            v[i++] = strdup(s);
        }while((s = strtok(NULL, sep)));

    v[len] = (void *)-1ull;
    free(t);
    return v;
}

static char *client_mkname(void)
{
    static int id = 0;
    char name[32];

    snprintf(name, sizeof(name), "vm%d", id++);
    return strdup(name);
}

static char *vif_mkname(struct client_int *cli, char *ifname, int idx)
{
    char name[128];

    snprintf(name, sizeof(name), "%s:%s:%d", cli->name, ifname, idx);
    return strdup(name);
}

static int client_iface_init(struct client_int *cli)
{
    coremodel_device_list_t *list;
    struct vif_node *vif;
    unsigned i, j;
    int ret = -1;

    if(!cli->cm)
        return -1;

    list = coremodel_list(cli->cm);
    if(!list)
        return -1;

    for(i=0; list[i].type!=COREMODEL_INVALID; i++){
        for(j=0; j<list[i].num; j++){
            vif = calloc(1, sizeof(*vif));
//            pr_info("VIF: %p\n", vif);
            if(!vif)
                goto cleanup;

            vif->type = list[i].type;
            vif->cli = cli;
            vif->name = vif_mkname(cli, list[i].name, j);
            if(!vif->name)
                goto cleanup;

            vif->tuple = hub_parse_tuple(vif->name, TUPLE_NELM, ":");
            if(!vif->tuple)
                goto cleanup;

            rbtree_insert(&g_state.rb, vif);
        }
    }

    vif = NULL;
    ret = 0;
cleanup:
    if(vif){
        if(vif->name)
            free(vif->name);
        if(vif->tuple)
            hub_free_tuple(vif->tuple);
        free(vif);
    }

    if(list)
        coremodel_free_list(list);
    return ret;
}

static void client_efree(struct client_ext *cli)
{
    free(cli);
}

static void client_ifree(struct client_int *cli)
{
    if(cli->cm)
        coremodel_disconnect(cli->cm);
    if(cli->name)
        free(cli->name);

    free(cli);
}

static void *client_inew(const char *cred)
{
    struct client_int *cli;
    char **tuple;

    cli = calloc(1, sizeof(struct client_int));
    if(!cli)
        goto cleanup;

    tuple = hub_parse_tuple(cred, TUPLE_NELM, ":");
    if(!tuple)
        goto cleanup;

    cli->name = !tuple[TUPLE_NAME_IDX] ? client_mkname() : strdup(tuple[TUPLE_NAME_IDX]);
    if(coremodel_connect_nswrap(&cli->cm, coremodel_mktarget(tuple)))
        goto cleanup;

    if(client_iface_init(cli))
        goto cleanup;

    hub_free_tuple(tuple);
    return cli;

cleanup:
    if(cli)
        client_ifree(cli);
    if(tuple)
        hub_free_tuple(tuple);
    return NULL;
}

static void *client_enew(const char *cred)
{
    struct client_int *cli;

    cli = calloc(1, sizeof(struct client_ext));
    if(!cli){
        free(cli);
        return NULL;
    }

    return cli;
}

static void client_free(struct client *cli)
{
    struct client **pcli;

    for(pcli=&g_state.clients; *pcli;){
        if(*pcli == cli){
            *pcli = cli->next;
        }else
            pcli = &((*pcli)->next);
    }

    switch(cli->type){
    case CLIENT_TYPE_INTERNAL:
        client_ifree(cli->inner);
        break;
    case CLIENT_TYPE_EXTERNAL:
        client_efree(cli->inner);
        break;
    };
    cli->inner = NULL;

    if(cli->cred)
        free(cli->cred);

    free(cli);
    g_state.nclients--;
}

static int hub_client_add(const char *cred, int type)
{
    struct client *cli;

    cli = calloc(1, sizeof(*cli));
    if(!cli)
        return -1;

    cli->type = type;
    cli->cred = strdup(cred);
    if(!cli->cred){
        free(cli);
        return -1;
    }

    switch(type){
    case CLIENT_TYPE_INTERNAL:
        cli->inner = client_inew(cred);
        break;
    case CLIENT_TYPE_EXTERNAL:
        cli->inner = client_enew(cred);
        break;
    };

    if(!cli->inner){
        free(cli->cred);
        free(cli);
        return -1;
    }

    if(g_state.clients)
        cli->next = g_state.clients;
    g_state.clients = cli;
    g_state.nclients++;

    return 0;
}

static int hub_set_listen(char *cred)
{
    char **v;

    g_state.cred = strdup(cred);
    if(!g_state.cred)
        return -1;

    v = hub_parse_tuple(cred, TUPLE_NELM, ":");
    if(!v)
        return -1;

    //XXX: set listen port, iface, etc

    hub_free_tuple(v);
    return 0;
}

static int vif_attach(struct vif_node *vif)
{
    int id;

    if(!vif->handle)
        switch(vif->type){
        case COREMODEL_GPIO:
            id = strtoull(vif->tuple[2], NULL, 0);
            vif->handle = coremodel_attach_gpio(ICLIENT(vif->cli)->cm, vif->tuple[1], id, &vif_gpio, vif);
            break;
        case COREMODEL_CAN:
            vif->handle = coremodel_attach_can(ICLIENT(vif->cli)->cm, vif->tuple[1], &vif_can, vif);
            break;
        case COREMODEL_UART:
        case COREMODEL_I2C:
        case COREMODEL_SPI:
        case COREMODEL_USBH:
            pr_error("Unsupported attach: %s\n", coremodel_type(vif->type));
            break;
        }

//    pr_debug("REF %s++ (%d -> %d)\n", vif->name, vif->ref, vif->ref + !!vif->handle);
    vif->ref += !!vif->handle;
    return !vif->handle;
}

static int vif_detach(struct vif_node *vif)
{
    if(vif->handle){
//    pr_debug("REF %s-- (%d -> %d)\n", vif->name, vif->ref, vif->ref - 1);
        if(--vif->ref){
            coremodel_detach(vif->handle);
            vif->handle = NULL;
        }
    }

    return 0;
}

static struct edge *edge_new(struct vif_node *node)
{
    struct edge *edge;

    edge = calloc(1, sizeof(*edge));
    if(!edge)
        return NULL;

    edge->node = node;
    return edge;
}

static int hub_vif_connect(char *a, char *b)
{
    struct vif_node t, *vifa, *vifb;
    struct edge *ea, *eb;
    int ret = -1;

    if(!a || !b)
        return -1;

    t.name = a;
    vifa = rbtree_find(&g_state.rb, &t);

    t.name = b;
    vifb = rbtree_find(&g_state.rb, &t);

    if(!vifa || !vifb)
        return -1;

    /* Only connect like-types */
    if(vifa->type != vifb->type){
        pr_error("Refusing to connect \'%s\' and \'%s\'\n",
            coremodel_type(vifa->type),
            coremodel_type(vifb->type));
        return -1;
    }

    ea = edge_new(vifb);
    eb = edge_new(vifa);
    if(!ea || !eb)
        goto cleanup;

    if(vif_attach(vifa))
        goto cleanup;
    if(vif_attach(vifb)){
        vif_detach(vifa);
        goto cleanup;
    }

    /* Not a diagraph; add to both nodes */
    eb->node = vifb;
    eb->next = vifa->edge;
    vifa->edge = eb;

    ea->node = vifa;
    ea->next = vifb->edge;
    vifb->edge = ea;

    ret = 0;
cleanup:
    if(ret){
        if(ea)  free(ea);
        if(eb)  free(eb);
    }
    return ret;
}

static int hub_connection_add(const char *cred)
{
    char **v;
    int ret = -1;

    if(!g_state.nclients){
        pr_error("Connection requested with no defined instance clients\n");
        return -1;
    }

    v = hub_parse_tuple(cred, 2, ",");
    if(v){
        ret = hub_vif_connect(v[0], v[1]);
        hub_free_tuple(v);
    }

    return ret;
}

static void *map_file(char *path, size_t *size)
{
    struct stat stat;
    void *map = NULL;
    int fd;

    fd = open(path, O_RDONLY);
    if(fd < 0){
        pr_error("Failed to open '%s': %s\n", path, strerror(errno));
        goto exit;
    }

    if(0 != fstat(fd, &stat)){
        pr_error("Failed to stat '%s': %s\n", path, strerror(errno));
        goto exit;
    }

    map = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(MAP_FAILED == map){
        pr_error("Failed to mmap '%s': %s\n", path, strerror(errno));
        map = NULL;
    }

    *size = stat.st_size;
exit:
    if(fd > 0)  close(fd);
    return map;
}

static int hub_parse_config_file(char *path)
{
    json_object *jroot, *j;
    void *map;
    const char *s;
    size_t size;
    int i, ret = -1;

    map = map_file(path, &size);
    if(!map)
        return -1;

    jroot = json_tokener_parse((char *)map);
    if(!jroot){
        pr_error("Invalid JSON config file provided\n");
        goto done;
    }

    /* Walk all specified instances */
    j = json_object_object_get(jroot, "instances");
    if(!j)
        goto done;

    for(i=0; i<json_object_array_length(j); i++){
        s = json_object_get_string(json_object_array_get_idx(j, i));
        if(hub_client_add(s, CLIENT_TYPE_INTERNAL))
            goto done;
    }

    /* Walk all specified connections */
    j = json_object_object_get(jroot, "connections");
    if(!j)
        goto done;

    for(i=0; i<json_object_array_length(j); i++){
        s = json_object_get_string(json_object_array_get_idx(j, i));
        if(hub_connection_add(s))
            goto done;
    }

    ret = 0;
done:
    if(map)
        munmap(map, size);
    return ret;
}

static int hub_parse_args(int argc, char *argv[])
{
    int opt;

    if(argc <= 1)
        return -1;

    while((opt = getopt(argc, argv, "c:f:hi:l:")) >= 0){
        switch(opt){
        case 'c':
            if(hub_connection_add(optarg))
                return -1;
            break;
        case 'f':
            if(hub_parse_config_file(optarg))
                return -1;
            break;
        case 'i':
            if(hub_client_add(optarg, CLIENT_TYPE_INTERNAL))
                return -1;
            break;
        case 'l':
            if(hub_set_listen(optarg))
                return -1;
            break;
        case '?':
        case 'h':
            return -1;
        }
    }

    return 0;
}

static void hub_vif_edge_free(void *param, void *node)
{
    struct vif_node *vif = node;
    struct edge *t, *edge = vif->edge;

    while(edge){
        vif_detach(edge->node);
        t = edge;
        edge = edge->next;
        free(t);
    }
}

static void hub_vif_free(void *param, void *node)
{
    struct vif_node *vif = node;

//    if(vif->ref)
//        pr_error("Freeing VIF (%p) with ref.%d\n", vif, vif->ref);

    if(vif->tuple)
        hub_free_tuple(vif->tuple);

    if(vif->name)
        free(vif->name);

    free(vif);
}

static void hub_cleanup(void)
{
    struct netlist *net, *t;

    /*
     * First, free all edges in the graph to lower references to
     * zero for all CoreModel connections. Then free the VIF structures
     * themselves
     */
   rbtree_walk(&g_state.rb, hub_vif_edge_free, NULL);
   rbtree_free(&g_state.rb, hub_vif_free, NULL);

    while(g_state.clients)
        client_free(g_state.clients);

    if(g_state.cred)
        free(g_state.cred);

    net = g_state.nets;
    while(net){
        t = net;
        net = net->next;
        free(t);
    }
}

int vif_compare(void *param, void *a, void *b)
{
    struct vif_node *vifa = a;
    struct vif_node *vifb = b;

    return strcmp(vifa->name, vifb->name);
}

static void vif_visit(struct vif_node *vif)
{
    struct edge *edge;

    if(!vif->edge || vif->flags)
        return;

    /* The head of the netlist is the most recently added net, so append */
    vif->net.head = g_state.nets;
    vif->net.next = g_state.nets->head;
    g_state.nets->head = &vif->net;

    vif->flags = 1;
    edge = vif->edge;
    while(edge){
        vif_visit(edge->node);
        edge = edge->next;
    }
}

static void vif_walk(void *param, void *node)
{
    struct vif_node *vif = node;
    struct edge *e;
    struct netlist *net;

    switch((uintptr_t)param){
    case VIF_WALK_NODE_RESET:
        vif->flags = 0;
        break;
    case VIF_WALK_LIST_GRAPH:
        if(!vif->edge)
            return;

        pr_info("[%s]\n", vif->name);
        e = vif->edge;
        while(e){
            pr_info("\t|--> %s\n", e->node->name);
            e = e->next;
        }
        break;
    case VIF_WALK_CONSTRUCT_NETLIST:
        if(!vif->flags){
            if(vif->edge){
                net = calloc(1, sizeof(*net));
                if(net){
                    net->next = g_state.nets;
                    g_state.nets = net;
                }
            }
            vif_visit(vif);
        }

        break;
    }
}

static void vif_graph_reset(void)
{
    rbtree_walk(&g_state.rb, vif_walk, (void *)VIF_WALK_NODE_RESET);
}

static void debug_list_instances(void)
{
    struct client *cli;

    pr_debug("\n--- Internal Clients ---\n");
    cli = g_state.clients;
    while(cli){
        if(cli->type == CLIENT_TYPE_INTERNAL)
            pr_debug("%s\n", cli->cred);
        cli = cli->next;
    }
    pr_debug("------------------------\n");
}

static void debug_list_cgraph(void)
{
    pr_debug("\n--- Connectivity Graph ---\n");
    rbtree_walk(&g_state.rb, vif_walk, (void *)VIF_WALK_LIST_GRAPH);
    pr_debug("---------------------------\n");
}

static void debug_list_nets(void)
{
    struct netlist *net, *t;
    struct vif_node *vif;
    unsigned i = 0, nnets;

    net = g_state.nets;
    if(!net)
        return;

    pr_debug("\n--- Netlists ---\n");
    while(net){
        pr_debug("%d", i++);
        t = net->head;
        nnets = 0;
        while(t){
            if( nnets && !(nnets % 3) )
                pr_debug("\n\t");
            else
                pr_debug("\t");
            nnets++;

            vif = container_of(t, struct vif_node, net);
            pr_debug("%s\t", vif->name);
            t = t->next;
        }
        net = net->next;
        pr_debug("\n");
        if(net)
            pr_debug("\n");
    }
    pr_debug("----------------\n");
}

int hub_construct_netlist(void)
{
    vif_graph_reset();
    rbtree_walk(&g_state.rb, vif_walk, (void *)VIF_WALK_CONSTRUCT_NETLIST);
    return 0;
}

static char *hub_save(void)
{
    return NULL;
}

static int hub_load(char *state)
{
    return -1;
}

int main(int argc, char *argv[])
{
    int ret = -1;
    struct client *t, *cli;

    rbtree_setup(&g_state.rb, offsetof(struct vif_node, node), vif_compare, NULL);
    if(hub_parse_args(argc, argv)){
        hub_usage();
        goto cleanup;
    }

    if(!g_state.nclients)
        goto cleanup;

    if(hub_construct_netlist())
        goto cleanup;

    debug_list_instances();
    //debug_list_cgraph();
    debug_list_nets();

    while(1){
        cli = g_state.clients;
        if(!cli)
            break;

        while(cli){
            if(CLIENT_TYPE_INTERNAL == cli->type){
                if(coremodel_mainloop(ICLIENT(cli->inner)->cm, 10000)){
                    t = cli->next;
                    client_free(t);
                    cli = t;
                    continue;
                }
            }
            cli = cli->next;
        }
    }

    ret = 0;
cleanup:
    hub_cleanup();
    return ret;
}
