#include <stdio.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>

#include <coremodel.h>

static const unsigned can_datalen[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };

static void *handle = NULL;
static int send_dummy = 1; // XXX make this configurable

static int test_can_tx(void *priv, uint64_t *ctrl, uint8_t *data)
{
    unsigned dlen = can_datalen[(ctrl[0] & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT];
    unsigned idx;
    // Extract identifier fields
    // uint32_t std_id = (ctrl[0] & CAN_CTRL_ID_MASK) >> CAN_CTRL_ID_SHIFT;   // 11-bit
    // uint32_t ext_id = (ctrl[0] & CAN_CTRL_EID_MASK) >> CAN_CTRL_EID_SHIFT; // 18-bit
    // Extract flag bits
    int ide = (ctrl[0] & CAN_CTRL_IDE) ? 1 : 0;  // Extended ID enable
    int rtr = (ctrl[0] & CAN_CTRL_RTR) ? 1 : 0;  // Standard RTR
    int ertr = (ctrl[0] & CAN_CTRL_ERTR) ? 1 : 0; // Extended RTR
    // Extract CAN FD flags
    int fdf = (ctrl[0] & CAN_CTRL_FDF) ? 1 : 0;  // FD Format
    int brs = (ctrl[0] & CAN_CTRL_BRS) ? 1 : 0;  // Bit Rate Switch
    int edl = (ctrl[0] & CAN_CTRL_EDL) ? 1 : 0;  // Extended Data Length
    // Determine actual CAN ID and RTR status
    uint32_t can_id;
    int is_rtr;
    if (ide) {
        // Extended frame (29-bit ID)
        can_id = (ctrl[0] >> CAN_CTRL_EID_SHIFT & 0x3ffff) | ((ctrl[0] >> CAN_CTRL_ID_SHIFT & 0x7ff) << 18);
        is_rtr = ertr;
    } else {
        // Standard frame (11-bit ID)
        can_id = (ctrl[0] >> CAN_CTRL_ID_SHIFT & 0x7ff);
        is_rtr = rtr;
    }
    // Print detailed frame information
    printf("\n======== CAN Frame Received ========\n");
    printf("Raw ctrl[0]: 0x%016"PRIx64"\n", ctrl[0]);
    printf("Raw ctrl[1]: 0x%016"PRIx64"\n", ctrl[1]);
    printf("\n");
    printf("Frame Type:  %s CAN%s\n", ide ? "Extended" : "Standard", fdf ? " FD" : "");
    printf("CAN ID:      0x%X (%u)\n", can_id, can_id);
    printf("Data Bytes Length:  %u\n", dlen);
    printf("RTR:         %s\n", is_rtr ? "Yes" : "No");
    printf("IDE:         %s\n", ide ? "Yes" : "No");
    if (fdf) {
        printf("\nCAN FD Info:\n");
        printf("  BRS: %d (Bit Rate Switch)\n", brs);
        printf("  EDL: %d (Extended Data Length)\n", edl);
    }
    // Print data payload
    if (dlen > 0 && data != NULL) {
        printf("\nData Payload (%u bytes):\n", dlen);
        printf("  Hex: ");
        for(idx=0; idx<dlen; idx++)
            printf("%02X ", data[idx]);
        printf("\n  Dec: ");
        for(idx=0; idx<dlen; idx++)
            printf("%3u ", data[idx]);
        printf("\n");
    } else if (is_rtr) {
        printf("\nNo data (RTR frame - remote request)\n");
    }
    printf("====================================\n\n");
    fflush(stdout);
    // Send response frame
   // Using ID 0x123 as a generic test ID
    uint64_t rxctrl[2] = {
        (8ull << CAN_CTRL_DLC_SHIFT) | (0x123ull << CAN_CTRL_ID_SHIFT),
        0
    };

    if (send_dummy) {
        // Dummy data payload (8 bytes)
        uint8_t dummy_data[8] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE };
        if(coremodel_can_rx(handle, rxctrl, dummy_data)) {
            fprintf(stderr, "Rx send data failed\n");
        }
    } else {
        if(coremodel_can_rx(handle, rxctrl, NULL)) {
            fprintf(stderr, "Rx send failed\n");
        }
    }

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
