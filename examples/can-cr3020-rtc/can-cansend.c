#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <coremodel.h>

// translate CAN data length specification into bytes
static const unsigned can_datalen[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };

// static void *handle = NULL;
typedef struct cansend_state {
    void *handle;
    uint8_t padding;
} cansend_state_t;

static int can_cansend_receive(void *priv, uint64_t *ctrl, uint8_t *data)
{
    cansend_state_t *state=priv;
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

    if(coremodel_can_rx(state->handle, rxctrl, NULL))
        fprintf(stderr, "Rx send failed\n");

    return CAN_NAK;
}

static void can_cansend_rxcomplete(void *priv, int nak)
{
    printf(" -> %d\n", nak);
}

/* .tx is defined as the function called by the bus on any transmit
 * .rxcomplete is defined as the function called by the bus in response 
 *  to a transmit from this node
 */
static const coremodel_can_func_t can_cansend_func = {
    .tx = can_cansend_receive,
    .rxcomplete = can_cansend_rxcomplete };
 
/* 
 * To transmit from this node to the bus call the bus receive function:
 * int coremodel_can_rx(void *can, uint64_t ctrl, uint8_t *data);
 */
 
int main(int argc, char *argv[])
{
    int res;
    cansend_state_t *state;

    if(argc != 4 || strlen(argv[3]) < 4 || argv[3][3] != '#' || strlen(argv[3]) > 20) {
        printf("usage: coremodel-can <address[:port]> <can> <data>\n");
        printf("data is 3 hex character address followed by the # character\n");
        printf("payload is up to 16 hex characters (8 bytes)\n");
        return 1;
    }

    res = coremodel_connect(argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    state = malloc(sizeof(cansend_state_t));

    state->handle = coremodel_attach_can(argv[2], &can_cansend_func, state);
    if(!state->handle) {
        fprintf(stderr, "error: failed to attach CAN.\n");
        coremodel_disconnect();
        return 1;
    }

    char *input_str = argv[3];
    uint8_t input_len = strlen(input_str);
    uint64_t rxctrl[2] = { 0, 0 };  // return control message
    uint64_t addr = 0;
    uint8_t *txdata;                // return data
    uint8_t data_length = input_len - 4; // remove address length 
    uint8_t dln = 0;
    uint8_t character = '0';
    if(data_length % 2)
        dln = (data_length - 1)/2; // drop an extra char
    else
        dln = data_length/2; // convert to bytes

    // process the address argument
    for(int i = 0; i < 3; i++) {
        if(input_str[i] <= '9' && input_str[i] >= '0') {
            addr = addr << 4;
            addr += input_str[i] - '0';
        }
    }
    rxctrl[0] |= (addr << CAN_CTRL_ID_SHIFT); // set ID
    rxctrl[0] |= (dln << CAN_CTRL_DLC_SHIFT); // set size
    if(dln > 0)
        txdata = malloc(sizeof(uint8_t) * dln);

    // process the data argument
    for(int i = 0; i < dln; i++) {
        character = input_str[(2*i) + 4];  // first nibble
        if(character <= '9' && character >= '0') {
            txdata[i] = (character - '0') << 4;
        } else if(character <= 'F' && character >= 'A') {
            txdata[i] = (character - 'A' + 0xa) << 4;
        } else if(character <= 'f' && character >= 'a') {
            txdata[i] = (character - 'a' + 0xa) << 4;
        }
        character = input_str[(2*i) + 4 + 1]; // second nibble
        if(character <= '9' && character >= '0') {
            txdata[i] += (character - '0');
        } else if(character <= 'F' && character >= 'A') {
            txdata[i] += (character - 'A' + 0xa);
        } else if(character <= 'f' && character >= 'a') {
            txdata[i] += (character - 'a' + 0xa);
        }
    }
    // send the frame
    if(coremodel_can_rx(state->handle, rxctrl, txdata))
        fprintf(stderr, "Rx send failed\n");
    if(dln > 0)
        free(txdata); // WARNING when porting to CHARM this may need to move to _rxcomplete

    coremodel_mainloop(1);

    coremodel_detach(state->handle);
    coremodel_disconnect();

    return 0;
}
