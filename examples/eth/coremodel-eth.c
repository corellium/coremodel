#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <coremodel.h>

#include <stdio.h>
#include <ctype.h>

void *handle;

#include <inttypes.h>
void hexdump(void *addr, void *buf, size_t len)
{
    int i;
    char ascii[17] = {0};
    uint8_t *data = (uint8_t *)buf;

    printf("0x%08" PRIx64 "    ", (uint64_t)addr);
    for(i=0; i<len; i++){
        if( (i != 0) && (0 == (i % 16)) )
            printf("%s\n\r0x%08" PRIx64 "    ", ascii, (uint64_t)addr + i);

        if(data[i] >= 0x20 && data[i] <= 0x7e)
            ascii[i % 16] = data[i];
        else
            ascii[i % 16] = '.';

        printf("%02x ", data[i]);
    }

    /* Special case when len != 16 byte multiple */
    if(0 != (i % 16))
        for(int j=0; j < 16 - (i % 16); j++){
            printf("   ");
            ascii[ (i + j) % 16 ] = ' ';
        }

    printf("%s", ascii);
    printf("\n\r");
}

#include <stdlib.h>
static int test_eth_tx(void *priv, unsigned len, uint8_t *data)
{
    fprintf(stderr, "ETH TX! %x\n", len);
    hexdump(data, data, len);

    uint8_t d[32];
    memset(d, 0xff, 32);
    coremodel_eth_rx(handle, 32, d);
    return len;
}

static const coremodel_eth_func_t test_eth_func = {
    .tx = test_eth_tx
};

int main(int argc, char *argv[])
{
    int res;
    void *cm;

    if(argc != 3) {
        printf("usage: coremodel-eth<address[:port]> <eth>\n");
        return 1;
    }

    res = coremodel_connect(&cm, argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    handle = coremodel_attach_eth(cm, argv[2], &test_eth_func, NULL);
    if(!handle) {
        fprintf(stderr, "error: failed to attach ETH.\n");
        coremodel_disconnect(cm);
        return 1;
    }

    coremodel_mainloop(cm, -1);

    coremodel_detach(handle);
    coremodel_disconnect(cm);

    return 0;
}
