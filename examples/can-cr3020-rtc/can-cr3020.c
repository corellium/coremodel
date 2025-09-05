/*
 *
 * Copyright (C) 2025 Corellium LLC
 * All rights reserved.
 *
 */

 #include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <coremodel.h>

// translate CAN data length specification into bytes
static const unsigned can_datalen[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };

// local storage for the RTC
typedef struct cr3020_state {
    void *handle;
    uint8_t initialized;
    uint8_t node_id;
    uint8_t alarm_day;
    uint8_t alarm_hour;
    uint8_t alarm_minute;
    uint8_t alarm_enabled;
} cr3020_state_t;

// Convert a binary value into 7 bit BCD
static int format_7bit_bcd(int binary_value)
{
    return ((((binary_value / 10) << 4) + binary_value % 10) & 0x7f);
}
// Convert a binary value into 5 bit BCD
static int format_6bit_bcd(int binary_value)
{
    return (format_7bit_bcd(binary_value) & 0x3f);
}
// Convert a binary value into 4 bit BCD
static int format_5bit_bcd(int binary_value)
{
    return (format_7bit_bcd(binary_value) & 0x1f);
}

static int can_cr3020_receive(void *priv, uint64_t *ctrl, uint8_t *data)
{
    cr3020_state_t *state = priv;

    /* begin basic packet dump functionality */

    // find the length of data portion of the CAN frame
    unsigned dlen = can_datalen[(ctrl[0] & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT], idx;
    uint32_t ID = (ctrl[0] & CAN_CTRL_ID_MASK) >> CAN_CTRL_ID_SHIFT; // 11 bit ID
    uint32_t IDE = (ctrl[0] & CAN_CTRL_IDE) >> 34;  // Extended Frame Format Flag
    uint32_t RTR = (ctrl[0] & CAN_CTRL_RTR) >> 35;  // Remote Transmission Request Flag
    uint32_t EID = (ctrl[0] & CAN_CTRL_EID_MASK) >> CAN_CTRL_EID_SHIFT; // 18 bit EID
    uint32_t CEID = (ID << 18) | EID; // combined ID and EID as shown by candump
    if(IDE) { // if extended ID
        RTR = (ctrl[0] & CAN_CTRL_ERTR) >> 15;  // Extended Remote Transmission Request Flag
        printf("ID %08"PRIx32" RTR %"PRIx32, CEID, RTR);
    } else {
        printf("ID %03"PRIx32" RTR %"PRIx32, ID, RTR);
    }
    // note fields for CAN XL not addressed

    // print raw CTRL and data
    if(dlen) {
        printf(" [%016"PRIx64" %016"PRIx64"] %u, ", ctrl[0], ctrl[1], dlen);
        for(idx=0; idx<dlen; idx++) {
            printf("%02x", data[idx]);
            if((idx % 2) && (idx<(dlen-1))) // print _ every 4 characters
                printf("_");
        }
        printf("\n");
    } else
        printf(" [%016"PRIx64" %016"PRIx64"]\n", ctrl[0], ctrl[1]);
    fflush(stdout);

    /* end packet dump, begin RTC functionality */
    uint64_t rxctrl[2] = { 0, 0 };  // return control message
    uint8_t *txdata;                // return data

    if(IDE) // Note the RTC only handles SFF frames
        return CAN_ACK;
    
    // programming manual section 6.3.1
    if((ID == 0x7fful) && (dlen == 0)) { // broadcast initialization
        state->initialized = 1;
        printf("Initialization received.\n");
        rxctrl[0] |= (0x7feul << CAN_CTRL_ID_SHIFT); // set return ID
        rxctrl[0] |= (0x2ul << CAN_CTRL_DLC_SHIFT); // set return size
        txdata = malloc(sizeof(uint8_t) * 2);
        txdata[0] = state->node_id; // send the node id 
        txdata[1] = 0x2;     // set the baud rate to 500 kBaud ignored
        // send the reply
        if(coremodel_can_rx(state->handle, rxctrl, txdata))
            fprintf(stderr, "Rx send failed\n");
        free(txdata); // WARNING when porting to CHARM this may need to move to _rxcomplete
        return CAN_ACK; // only handle a single transaction
    }

    // device doesn't listen until initialized
    if(state->initialized == 0)
        return CAN_ACK;

    // programming manual section 6.3.2
    if(ID == 0x300 + state->node_id) {
        if(dlen != 6) { // RTC set
            printf("Incorrect argument count %02x for RTC Set command.\n", dlen);
        } else {
            printf("RTC Set.\n");
            printf("Ignoring request to set Day %02x Month %02x Year %02x Hour %02x Minute %02x DOW %02x\n",
                data[0], data[1], data[2], data[3], data[4], data[5]);
            rxctrl[0] |= ((0x380ul + state->node_id) << CAN_CTRL_ID_SHIFT); // set return ID
            rxctrl[0] |= (0x6ul << CAN_CTRL_DLC_SHIFT); // set return size
            txdata = malloc(sizeof(uint8_t) * 6);

            time_t system_time;
            time(&system_time);
            struct tm *gmt_time = localtime(&system_time);
            txdata[0] = format_6bit_bcd(gmt_time->tm_mday);
            printf("Day of Month [1-31] %02x\n", txdata[0]);
            txdata[1] = format_5bit_bcd(gmt_time->tm_mon + 1);
            printf("Month [1-12] %02x\n", txdata[1]);
            txdata[2] = (((gmt_time->tm_year % 100) / 10) << 4) + gmt_time->tm_year % 10;
            printf("Year since 1900 [0-99] %02x\n", txdata[2]);
            txdata[3] = format_6bit_bcd(gmt_time->tm_hour);
            printf("Hours [00-23] %02x\n", txdata[3]);
            txdata[4] = format_7bit_bcd(gmt_time->tm_min);
            printf("Minutes [00-59] %02x\n", txdata[4]);
            txdata[5] = gmt_time->tm_wday + 1;
            printf("Day of Week [1-7] %02x\n", txdata[5]);
            
            // send the reply
            if(coremodel_can_rx(state->handle, rxctrl, txdata))
                fprintf(stderr, "Rx send failed\n");
            free(txdata); // WARNING when porting to CHARM this may need to move to _rxcomplete
            return CAN_ACK; // only handle a single transaction
        }
    }

    // programming manual section 6.3.3
    if((ID == 0x200 + state->node_id) && (dlen == 0)) { // RTC request
        printf("RTC Request.\n");
        rxctrl[0] |= ((0x180ul + state->node_id) << CAN_CTRL_ID_SHIFT); // set return ID
        rxctrl[0] |= (0x8ul << CAN_CTRL_DLC_SHIFT); // set return size
        txdata = malloc(sizeof(uint8_t) * 8);

        time_t system_time;
        time(&system_time);
        struct tm *gmt_time = localtime(&system_time);
        txdata[0] = format_6bit_bcd(gmt_time->tm_mday);
        printf("Day of Month [1-31] %02x\n", txdata[0]);
        txdata[1] = format_5bit_bcd(gmt_time->tm_mon + 1);
        printf("Month [1-12] %02x\n", txdata[1]);
        txdata[2] = (((gmt_time->tm_year % 100) / 10) << 4) + gmt_time->tm_year % 10;
        printf("Year since 1900 [0-99] %02x\n", txdata[2]);
        txdata[3] = format_6bit_bcd(gmt_time->tm_hour);
        printf("Hours [00-23] %02x\n", txdata[3]);
        txdata[4] = format_7bit_bcd(gmt_time->tm_min);
        printf("Minutes [00-59] %02x\n", txdata[4]);
        txdata[5] = format_7bit_bcd(gmt_time->tm_sec);
        printf("Seconds [00-59] %02x\n", txdata[5]);
        txdata[6] = gmt_time->tm_wday + 1;
        printf("Day of Week [1-7] %02x\n", txdata[6]);
        txdata[7] = 0x0; // Battery state sufficient
        printf("Battery State Sufficient\n");
        
        // send the reply
        if(coremodel_can_rx(state->handle, rxctrl, txdata))
            fprintf(stderr, "Rx send failed\n");
        free(txdata); // WARNING when porting to CHARM this may need to move to _rxcomplete
        return CAN_ACK; // only handle a single transaction
    }

    // programming manual section 6.3.4
    if(ID == 0x500 + state->node_id) {
        if (dlen != 7) { // Alarm Set
            printf("Incorrect argument count %02x for Alarm Set command.\n", dlen);
        } else {
            printf("Setting Alarm.\n");
            rxctrl[0] |= (0x7ul << CAN_CTRL_DLC_SHIFT); // set return size
            txdata = malloc(sizeof(uint8_t) * 7);

            time_t system_time;
            time(&system_time);
            struct tm *gmt_time = localtime(&system_time);

            if (data[0] < 7)
                state->alarm_day = (((gmt_time->tm_wday - 1) + data[0]) % 7) + 1;
            txdata[0] = data[0];
            printf("Days until alarm [0-6] %02x\n", txdata[0]);

            if (data[1] < 24)
                state->alarm_hour = (gmt_time->tm_hour  + data[1]) % 24;
            txdata[1] = format_6bit_bcd(state->alarm_hour);
            printf("Hours until alarm [00-23] %02x\n", txdata[1]);

            if (data[2] < 60)
                state->alarm_minute = (gmt_time->tm_min  + data[2]) % 24;
            txdata[1] = format_7bit_bcd(gmt_time->tm_min);
            printf("Minutes until alarm [00-59] %02x\n", txdata[2]);

            if (data[3] == data[4] && data[3] < 0x40) {
                state->node_id = data[3];
                printf("Setting NodeID %02x\n", data[3]);
            }
            txdata[3] = state->node_id;
            txdata[4] = state->node_id;
            txdata[5] = 0x2;     // set the baud rate to 500 kBaud ignored
            printf("Setting Baud Rate to 500 kBd\n");
            if (data[6] == 00 || data[6] == 0xff) {
                state->alarm_enabled = data[6];
            }
            txdata[6] = state->alarm_enabled;
            printf("Setting Enabled to %02x\n", data[6]);
            
            // delay until after potential node_id update 
            rxctrl[0] |= ((0x480ul + state->node_id) << CAN_CTRL_ID_SHIFT); // set return ID

            // send the reply
            if(coremodel_can_rx(state->handle, rxctrl, txdata))
                fprintf(stderr, "Rx send failed\n");
            free(txdata); // WARNING when porting to CHARM this may need to move to _rxcomplete
            return CAN_ACK; // only handle a single transaction
        }
    }

    // programming manual section 6.3.5
    if((ID == 0x400 + state->node_id) && (dlen == 0)) { // RTC request
        printf("Request Time to Alarm.\n");
        rxctrl[0] |= ((0x180ul + state->node_id) << CAN_CTRL_ID_SHIFT); // set return ID
        rxctrl[0] |= (0x7ul << CAN_CTRL_DLC_SHIFT); // set return size
        txdata = malloc(sizeof(uint8_t) * 7);

        time_t system_time;
        time(&system_time);
        struct tm *gmt_time = localtime(&system_time);

        uint8_t temp = state->alarm_day;
        if (temp < gmt_time->tm_wday)
            temp += 7;
        txdata[0] = temp - gmt_time->tm_wday;
        printf("Days until alarm [0-6] %02x\n", txdata[0]);

        temp = state->alarm_hour;
        if (temp < gmt_time->tm_hour)
            temp += 24;
        txdata[1] = format_6bit_bcd(temp - gmt_time->tm_hour);
        printf("Hours until alarm [00-23] %02x\n", txdata[1]);

        temp = state->alarm_minute;
        if (temp < gmt_time->tm_mday)
            temp += 60;
        txdata[1] = format_7bit_bcd(temp - gmt_time->tm_min);
        printf("Minutes until alarm [00-59] %02x\n", txdata[2]);

        txdata[3] = state->node_id;
        txdata[4] = state->node_id;
        printf("NodeID %02x\n", txdata[3]);
        txdata[5] = 0x2;     // set the baud rate to 500 kBaud ignored
        printf("Baud Rate to 500 kBd\n");
        data[6] = state->alarm_enabled;
        printf("Enabled is %02x\n", data[6]);
        
        // send the reply
        if(coremodel_can_rx(state->handle, rxctrl, txdata))
            fprintf(stderr, "Rx send failed\n");
        free(txdata); // WARNING when porting to CHARM this may need to move to _rxcomplete
        return CAN_ACK; // only handle a single transaction
    }

    return CAN_ACK;
}

static void can_cr3020_rxcomplete(void *priv, int nak)
{
    printf(" -> %d\n", nak);
}

/* .tx is defined as the function called by the bus on any transmit
 * .rxcomplete is defined as the function called by the bus in response 
 *  to a transmit from this node
 */
static const coremodel_can_func_t test_can_func = {
    .tx = can_cr3020_receive,
    .rxcomplete = can_cr3020_rxcomplete };

/* 
 * To transmit from this node to the bus call the bus receive function:
 * int coremodel_can_rx(void *can, uint64_t ctrl, uint8_t *data);
 */
 
int main(int argc, char *argv[])
{
    int res;
    cr3020_state_t *state;

    if(argc != 3) {
        printf("usage: coremodel-can <address[:port]> <can>\n");
        return 1;
    }

    res = coremodel_connect(argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    state = malloc(sizeof(cr3020_state_t));

    state->handle = coremodel_attach_can(argv[2], &test_can_func, state);
    state->initialized = 0;
    state->node_id = 0x13; // per CANopen max ID == 0x3F
    state->alarm_day = 0;
    state->alarm_hour = 0;
    state->alarm_minute = 0;
    state->alarm_enabled = 0;

    if(!state->handle) {
        fprintf(stderr, "error: failed to attach CAN.\n");
        coremodel_disconnect();
        return 1;
    }

    coremodel_mainloop(-1);

    coremodel_detach(state->handle);
    coremodel_disconnect();

    return 0;
}
