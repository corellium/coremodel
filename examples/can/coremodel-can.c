#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <coremodel.h>

static const unsigned can_datalen[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };

static void *handle = NULL;

static int test_can_tx(void *priv, uint64_t ctrl, uint8_t *data)
{
    unsigned dlen = can_datalen[(ctrl & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT], idx;

    if(dlen) {
        printf("[%010lx] ", ctrl);
        for(idx=0; idx<dlen; idx++)
            printf("%02x", data[idx]);
        printf("\n");
    } else
        printf("[%010lx]\n", ctrl);
    fflush(stdout);

    if(coremodel_can_rx(handle, 0x8ac7ffff00, NULL))
        fprintf(stderr, "Rx send failed\n");

    return CAN_ACK;
}

static void test_can_rxcomplete(void *priv, int nak)
{
    printf(" -> %d\n", nak);
}

static const coremodel_can_func_t test_can_func = {
    .tx = test_can_tx,
    .rxcomplete = test_can_rxcomplete };
 
int main(int argc, char *argv[])
{
    int res;

    if(argc != 3) {
        printf("usage: coremodel-can <address[:port]> <can>\n");
        return 1;
    }

    res = coremodel_connect(argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    handle = coremodel_attach_can(argv[2], &test_can_func, NULL);
    if(!handle) {
        fprintf(stderr, "error: failed to attach CAN.\n");
        coremodel_disconnect();
        return 1;
    }

    coremodel_mainloop(-1);

    coremodel_detach(handle);
    coremodel_disconnect();

    return 0;
}
