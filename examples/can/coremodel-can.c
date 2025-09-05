#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>

#include <coremodel.h>

static const unsigned can_datalen[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };

static void *handle = NULL;

static int test_can_tx(void *priv, uint64_t *ctrl, uint8_t *data)
{
    unsigned dlen = can_datalen[(ctrl[0] & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT], idx;
    uint64_t rxctrl[2] = {
        CAN_CTRL_ERTR | (0x3FFFFull << CAN_CTRL_EID_SHIFT) | (0x456ull << CAN_CTRL_ID_SHIFT),
        0
    };

    if(dlen) {
        printf("[%016"PRIx64" %016"PRIx64"] %u, ", ctrl[0], ctrl[1], dlen);
        for(idx=0; idx<dlen; idx++)
            printf("%02x", data[idx]);
        printf("\n");
    } else
        printf("[%016"PRIx64" %016"PRIx64"]\n", ctrl[0], ctrl[1]);
    fflush(stdout);

    if(coremodel_can_rx(handle, rxctrl, NULL))
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
