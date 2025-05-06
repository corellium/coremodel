#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <fcntl.h>
#include <unistd.h>

#include <hub.h>
#include <nshostport.h>

static const char * const coremodel_type_string[] = {
    [ COREMODEL_UART ] = "uart",
    [ COREMODEL_I2C ]  = "i2c",
    [ COREMODEL_SPI ]  = "spi",
    [ COREMODEL_GPIO ] = "gpio",
    [ COREMODEL_USBH ] = "usbh",
    [ COREMODEL_CAN ] = "can"
};

char *coremodel_type(unsigned type)
{
    if(type >= ARRAY_SIZE(coremodel_type_string))
        return NULL;
    return (char *)coremodel_type_string[type];
}

char *coremodel_mktarget(char **tuple)
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

int coremodel_connect_nswrap(void **cm, char *target)
{
    struct nshostport nshp = {0};
    int cmret = -1, ret = -1, onsfd = -1, nsfd = -1;
    char remote[256];

    if(nshostport_parse(target, &nshp))
        return -1;

    if(nshp.ns){
        onsfd = open("/proc/self/ns/net", O_RDONLY);
        if(onsfd < 0)
            goto cleanup;

        nsfd = open(nshp.ns, O_RDONLY);
        if(nsfd < 0)
            goto cleanup;

        if(setns(nsfd, CLONE_NEWNET))
            goto cleanup;
    }

    snprintf(remote, sizeof(remote), "%s:%d", nshp.host, nshp.port);
    cmret = coremodel_connect(cm, remote);

    if(nsfd >= 0){
        if(setns(onsfd, CLONE_NEWNET))
            goto cleanup;
    }

    ret = 0;
cleanup:
    if(nsfd >= 0)
        close(nsfd);
    if(onsfd >= 0)
        close(onsfd);
    nshostport_clean(&nshp);
    return !!(ret | cmret);
}
