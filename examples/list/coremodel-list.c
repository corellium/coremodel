#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <coremodel.h>

static const char * const type_string[] = {
    [ COREMODEL_UART ] = "uart",
    [ COREMODEL_I2C ]  = "i2c",
    [ COREMODEL_SPI ]  = "spi",
    [ COREMODEL_GPIO ] = "gpio",
    [ COREMODEL_USBH ] = "usbh" };

int main(int argc, char *argv[])
{
    coremodel_device_list_t *list;
    unsigned idx;
    int res;

    if(argc != 2) {
        printf("usage: coremodel-list <address[:port]>\n");
        return 1;
    }

    res = coremodel_connect(argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    list = coremodel_list();

    coremodel_disconnect();

    if(!list) {
        fprintf(stderr, "error: failed to list devices.\n");
        return 1;
    }

    for(idx=0; list[idx].type!=COREMODEL_INVALID; idx++)
        printf("%2d  %-7s %-11s %d\n", idx, type_string[list[idx].type], list[idx].name, list[idx].num);

    coremodel_free_list(list);
    return 0;
}
