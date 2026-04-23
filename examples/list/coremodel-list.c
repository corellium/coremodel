#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <coremodel.h>

static const char * const type_string[] = {
    [ COREMODEL_UART ] = "uart",
    [ COREMODEL_I2C ]  = "i2c",
    [ COREMODEL_SPI ]  = "spi",
    [ COREMODEL_GPIO ] = "gpio",
    [ COREMODEL_USBH ] = "usbh",
    [ COREMODEL_CAN ] = "can",
    [ COREMODEL_EVENT] = "event" };

int main(int argc, char *argv[])
{
    coremodel_device_list_t *list, *sublist;
    void *cm;
    unsigned idx, subidx;
    int res;

    if(argc != 2) {
        printf("usage: coremodel-list <address[:port]>\n");
        return 1;
    }

    res = coremodel_connect(&cm, argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    list = coremodel_list(cm);
    if(!list) {
        coremodel_disconnect(cm);
        fprintf(stderr, "error: failed to list devices.\n");
        return 1;
    }

    for(idx=0; list[idx].type!=COREMODEL_INVALID; idx++) {
        printf("%2d  %-7s %-11s %d\n", idx, type_string[list[idx].type], list[idx].name, list[idx].num);
        if(list[idx].type == COREMODEL_SPI || list[idx].type == COREMODEL_GPIO || list[idx].type == COREMODEL_EVENT) {
            sublist = coremodel_list_subdevices(cm, list[idx].type, list[idx].name);
            if(sublist) {
                for(subidx=0; sublist[subidx].type!=COREMODEL_INVALID; subidx++)
                    if(sublist[subidx].name[0])
                        printf("            %-11s %d\n", sublist[subidx].name, sublist[subidx].num);
                coremodel_free_list(sublist);
            }
        }
    }

    coremodel_free_list(list);
    coremodel_disconnect(cm);
    return 0;
}
