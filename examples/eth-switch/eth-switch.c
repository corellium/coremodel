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
#include <inttypes.h>
#include <signal.h>
#include <time.h>

#include <coremodel.h>
#include <eth-switch.h>

#define SWITCH_DEBUG_DATA   1
#define SWITCH_DEBUG_LUT    2
#define SWITCH_DEBUG        (0)

/* MACs will be purged if not sene for at least this long */
#define SWITCH_MAC_PURGE_SECONDS    60

struct client {
    void *cm;
    char *name;
    struct client *next;
    char **tuple;
    void *handle;

    struct mac_entry {
        struct mac_entry *next;
        uint64_t mac;
        time_t ts;
    } *me;
    int nme;
};

/* Global wiswitch ub state */
struct switch_state {
    unsigned run;
    unsigned reload;
    struct client *clients;
    unsigned nclients;
} g_state;

extern const char *__progname;
static int switch_usage(void)
{
    pr_info("usage %s [OPTIONS]\n"
            "\t-c remote/port/interface\tConnect exported 'iface' of host 'remote:port' to switch\n"
            "\t-h\t\t\t\tThis help text\n"
    , __progname);
    return -1;
}

static uint64_t macswap(uint64_t mac)
{
    return __builtin_bswap64(mac) >> 16;
}

static uint64_t buf2mac(uint8_t *data)
{
    uint64_t mac;

    memcpy(&mac, data, 6);
    return (mac & ~0xffff000000000000ull);
}

static char *coremodel_mktarget(char **tuple)
{
    static char name[256];
    char *ip = tuple[TUPLE_IP_IDX];
    char *port = tuple[TUPLE_PORT_IDX];

    if(!ip)
        ip = "localhost";
    if(!port)
        port = str(COREMODEL_DFLT_PORT);

    snprintf(name, sizeof(name), "%s:%s", ip, port);
    return name;
}

static char **switch_parse_tuple(const char *tuple, int len, char *sep)
{
    char **v, *s, *t, *u;
    unsigned sz, q, i, j;
    int seps[len];

    sz = strlen(tuple);
    s = t = strdup(tuple);
    if(!s)
        return NULL;

    v = calloc(sizeof(char *), len + 1);
    if(!v){
        free(s);
        return NULL;
    }

    /* Count and cache the number of seps */
    i = q = 0;
    while(*s && ((s - t) <= sz)){
        q ^= (*s == '\"');
        if(!q && (*s == *sep)){
            seps[i++] = (s - t);
            *(t + (s - t)) = '\0';
        }
        s++;
    }

    if(i+1 != len){
        pr_error("Malformed argument\n");
        goto err;
    }

    /* Save each substring, removing parenthesis as needed */
    i = len - i - 1;
    for(s=t,j=0; j<len; j++, i++){
        v[i] = strdup(s);
        if('\"' == *s){
            if( !(u = strchr(s + 1, '\"')) ){
                pr_error("Unbalanced parenthesis in arguments\n");
                goto err;
            }
            memcpy(v[i], s + 1, u - s - 1);
            v[i][u - s - 1] = '\0';
        }
        s = t + seps[j] + 1;
    }

    v[len] = (void *)-1ull;
    free(t);

    return v;

err:
    if(v)
        free(v);
    if(t)
        free(t);
    return NULL;
}

static void client_free(struct client *cli)
{
    struct client **pcli;
    struct mac_entry *me;

    for(pcli=&g_state.clients; *pcli;){
        if(*pcli == cli)
            *pcli = cli->next;
        else
            pcli = &((*pcli)->next);
    }

    me = cli->me;
    while(me){
        cli->me = me->next;
        free(me);
        cli->nme--;
        me = cli->me;
    }

    if(cli->handle)
        coremodel_detach(cli->handle);
    if(cli->cm)
        coremodel_disconnect(cli->cm);
    if(cli->name)
        free(cli->name);
    if(cli->tuple)
        free(cli->tuple);

    free(cli);
    g_state.nclients--;
}

/* A naive linear search of discovered clients */
static struct client *switch_cli_locate(uint64_t mac)
{
    struct client *cli;
    struct mac_entry *me, *pme = NULL;

    cli = g_state.clients;
    while(cli){
        me = cli->me;
        while(me){
            if(me->mac == mac){
                me->ts = time(NULL);
                break;
            }else{
                /* Purge old MACs */
                if((time(NULL) - me->ts) > SWITCH_MAC_PURGE_SECONDS){
                    if(pme)
                        pme->next = me->next;
                    free(me);
                    me = pme;

                    if( !--cli->nme ){
                        me = pme = cli->me = NULL;
                        break;
                    }
                }
            }
            pme = me;
            me = me->next;
        }

        if(me)
            break;
        cli = cli->next;
    }

#if SWITCH_DEBUG & SWITCH_DEBUG_LUT
    if(me)
        pr_debug("Found MAC.%012lx on %s\n", macswap(me->mac), cli->name);
#endif

    return cli;
}

static void switch_cli_add_mac(struct client *cli, uint64_t mac)
{
    struct mac_entry *me;

#if SWITCH_DEBUG & SWITCH_DEBUG_LUT
    pr_debug("+MAC.%012lx on %s\n", macswap(mac), cli->name);
#endif

    if( !(me = calloc(1, sizeof(*me))) )
        return;

    me->mac = mac;
    me->ts = time(NULL);
    me->next = cli->me;
    cli->me = me;
    cli->nme++;
}

static int switch_eth_tx(void *priv, unsigned len, uint8_t *data)
{
    struct client *dcli, *scli = priv;
    uint64_t smac, dmac;

    /* No MAC addresses? */
    if(len < 12)
        return len;

    smac = buf2mac(&data[6]);
    dmac = buf2mac(&data[0]);

    /* If we don't find the source mac on the source CLI then add it */
    if(scli != switch_cli_locate(smac))
        switch_cli_add_mac(scli, smac);

    /* If we don't find the destination mac on any CLI then broadcast it */
    if( (0xffffffffffff == dmac) || !(dcli = switch_cli_locate(dmac)) ){
        dcli = g_state.clients;
        while(dcli){
            if(dcli != scli){
#if SWITCH_DEBUG & SWITCH_DEBUG_DATA
                pr_info("ETH.%04x (%s) %012lx -> (%s) %012lx\n", len, scli->name, macswap(smac), dcli->name, macswap(dmac));
                hexdump(data, len);
#endif
                coremodel_eth_rx(dcli->handle, len, data);
            }
            dcli = dcli->next;
        }
    }else{
#if SWITCH_DEBUG & SWITCH_DEBUG_DATA
                pr_info("ETH.%04x (%s) %012lx -> (%s) %012lx\n", len, scli->name, macswap(smac), dcli->name, macswap(dmac));
                hexdump(data, len);
#endif
        coremodel_eth_rx(dcli->handle, len, data);
    }

    return len;
}

static const coremodel_eth_func_t switch_eth_func = {
    .tx = switch_eth_tx
};

static int switch_client_add(const char *cred)
{
    struct client *cli;

    if( !(cli = calloc(1, sizeof(*cli))) )
        return -1;

    cli->name = strdup(cred);

    if( !(cli->tuple = switch_parse_tuple(cred, TUPLE_NELM, "/")) )
        goto cleanup;

    if(coremodel_connect(&cli->cm, coremodel_mktarget(cli->tuple)))
        goto cleanup;

    cli->handle = coremodel_attach_eth(cli->cm, cli->tuple[TUPLE_NAME_IDX], &switch_eth_func, cli);
    if(!cli->handle)
        goto cleanup;

    cli->next = g_state.clients;
    g_state.clients = cli;

    g_state.nclients++;
    return 0;

cleanup:
    if(cli)
        client_free(cli);
    return -1;
}

static void switch_cleanup_connections(void)
{
    while(g_state.clients)
        client_free(g_state.clients);
    g_state.clients = NULL;
}

static void switch_cleanup(void)
{
    switch_cleanup_connections();
}

static void switch_sigint_handler(int sig, siginfo_t *info, void *ucontext)
{
    g_state.run = 0;
}

static int switch_install_sigaction(void)
{
    struct sigaction sa_int = {
        .sa_sigaction = switch_sigint_handler,
        .sa_flags = SA_SIGINFO | SA_SIGINFO,
    };

    return sigaction(SIGINT, &sa_int, NULL);
}

static int switch_parse_args(int argc, char *argv[])
{
    int opt;

    if(argc <= 1)
        return -1;

    while((opt = getopt(argc, argv, "c:f:h")) >= 0){
        switch(opt){
        case 'c':
            if(switch_client_add(optarg)){
                pr_error("Failed to add client \'%s\'\n", optarg);
                return 1;
            }
            break;
        case '?':
        case 'h':
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int ret = -1;
    struct client *t, *cli;

    if(switch_parse_args(argc, argv) < 0){
        switch_usage();
        goto cleanup;
    }

    if(!g_state.nclients)
        goto cleanup;

    g_state.run = 1;
    while(g_state.run){
        cli = g_state.clients;
        if(!cli)
            break;

        while(cli){
            if(coremodel_mainloop(cli->cm, 10000)){
                t = cli->next;
                client_free(t);
                cli = t;
                continue;
            }
            cli = cli->next;
        }
    }

    ret = 0;
cleanup:
    switch_cleanup();
    return ret;
}
