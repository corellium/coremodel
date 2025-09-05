/*
 *
 * Copyright (C) 2025 Corellium LLC
 * All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <coremodel.h>

// added for RTC
typedef struct ds3231_state {
    time_t system_time;
    uint8_t index;
    uint8_t start;
    uint8_t twelve_hour_flag;
    uint8_t Alarm1_Sec;
    uint8_t Alarm1_Min;
    uint8_t Alarm1_Hours;
    uint8_t Alarm1_DayDate;
    uint8_t Alarm2_Min;
    uint8_t Alarm2_Hours;
    uint8_t Alarm2_DayDate;
    uint8_t control;
    uint8_t status;
    uint8_t aging;
} ds3231_state_t;

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

// Print Enabled or disabled based on the high bit
static void print_enabled(uint8_t value)
{
    if ((value & 0x80) > 0) {
        printf("Enabled");
    } else {
        printf("Disabled");
    }
}

static int i2c_ds3231_start(void *priv)
{
    // cast the void * into the "this" pointer
    ds3231_state_t *state = priv;
    printf("START\n");

    // read the system time
    struct tm *gmt_time = localtime(&state->system_time);
    int old_second = gmt_time->tm_sec;
    int old_minute = gmt_time->tm_min;
    time(&state->system_time);
    state->start = 1;
    gmt_time = localtime(&state->system_time);

    // calculating alarm 1
    if ((state->Alarm1_Sec & 0x80) > 0) { // A1M1 is high
        if (old_second != format_7bit_bcd(gmt_time->tm_sec))
            state->status |= 0b1;
    } else if ((state->Alarm1_Min & 0x80) > 0) { //A1M2 is high
        if ((state->Alarm1_Sec & 0x7f) == format_7bit_bcd(gmt_time->tm_sec))
            state->status |= 0b1;
    } else if ((state->Alarm1_Hours & 0x80) > 0) { //A1M3 is high
        if ((state->Alarm1_Min & 0x7f) == format_7bit_bcd(gmt_time->tm_min))
            state->status |= 0b1;
    } else if ((state->Alarm1_DayDate & 0x80) > 0) { //A1M4 is high
        if (state->twelve_hour_flag > 0) {
            if (gmt_time->tm_hour > 11) {
                if ((state->Alarm1_Hours & 0x7f) == format_5bit_bcd(gmt_time->tm_hour - 12) + 0b01100000)
                    state->status |= 0b1;
            } else {
                if ((state->Alarm1_Hours & 0x7f) == format_5bit_bcd(gmt_time->tm_hour - 12) + 0b01000000)
                    state->status |= 0b1;
            }
        } else {
            if ((state->Alarm1_Hours & 0x7f) == format_6bit_bcd(gmt_time->tm_hour))
                state->status |= 0b1;
        }
    } else if ((state->Alarm1_DayDate & 0x40) > 0) { // DY/!DT is high
        if ((state->Alarm1_DayDate & 0x3f) == gmt_time->tm_wday + 1)
            state->status |= 0b1;
    } else { // all flags are low
        if ((state->Alarm1_DayDate & 0x3f) == format_6bit_bcd(gmt_time->tm_mday))
            state->status |= 0b1;
    }
    // calculating alarm 2
    if ((state->Alarm2_Min & 0x80) > 0) { //A1M2 is high
        if (old_minute != format_7bit_bcd(gmt_time->tm_min))
            state->status |= 0b10;
    } else if ((state->Alarm2_Hours & 0x80) > 0) { //A1M3 is high
        if ((state->Alarm2_Min & 0x7f) == format_7bit_bcd(gmt_time->tm_min))
            state->status |= 0b10;
    } else if ((state->Alarm2_DayDate & 0x80) > 0) { //A1M4 is high
        if (state->twelve_hour_flag > 0) {
            if (gmt_time->tm_hour > 11) {
                if ((state->Alarm2_Hours & 0x7f) == format_5bit_bcd(gmt_time->tm_hour - 12) + 0b01100000)
                    state->status |= 0b10;
            } else {
                if ((state->Alarm2_Hours & 0x7f) == format_5bit_bcd(gmt_time->tm_hour - 12) + 0b01000000)
                    state->status |= 0b10;
            }
        } else {
            if ((state->Alarm2_Hours & 0x7f) == format_6bit_bcd(gmt_time->tm_hour))
                state->status |= 0b10;
        }
    } else if ((state->Alarm2_DayDate & 0x40) > 0) { // DY/!DT is high
        if ((state->Alarm2_DayDate & 0x3f) == gmt_time->tm_wday + 1)
            state->status |= 0b10;
    } else { // all flags are low
        if ((state->Alarm2_DayDate & 0x3f) == format_6bit_bcd(gmt_time->tm_mday))
            state->status |= 0b10;
    }

    // note this code does not check if an alarm should have gone off since the
    // last START, rather it only alerts if it should be on now
    fflush(stdout);
    return 1;
}

static int i2c_ds3231_write(void *priv, unsigned len, uint8_t *data)
{
    // only register select for reading implemented no other RTC functions implemented
    unsigned idx = 0;
    ds3231_state_t *state = priv;
    uint16_t offset;
    uint8_t temp = 0x80;
    if (state->start == 1) {
        printf("WRITE Addr [%d]: %02x\n", len, data[idx]);
        state->index = data[0];
        idx = 1;
        state->start = 0;
        if (len == 1)
            return len;
    }
    printf("WRITE [%d]:", len);
    for(; idx<len; idx++) {
        offset = (idx + state->index) % 0x13;
        switch (offset) {
            case 0: // BCD Seconds
                printf("Ignoring write of %02x to seconds register", data[idx]);
                break;
            case 1: // BCD Minutes
                printf("Ignoring write of %02x to minutes register", data[idx]);
                break;
            case 2: // BCD Hours
                if ((data[idx] & 0x40) > 0) {
                    printf("Setting AM/PM mode");
                    state->twelve_hour_flag = 1;
                } else {
                    printf("Setting 24 hour mode");
                    state->twelve_hour_flag = 0;
                }
                break;
            case 3: // 1 based day of week
                printf("Ignoring write of %02x to day of week register", data[idx]);
                break;
            case 4: // 1 based day of month
                printf("Ignoring write of %02x to day of month register", data[idx]);
                break;
            case 5: // centry + month
                printf("Ignoring write of %02x to month register", data[idx]);
                break;
            case 6: // year
                printf("Ignoring write of %02x to year register", data[idx]);
                break;
            case 7: // Alarm 1 Seconds
                state->Alarm1_Sec = data[idx];
                printf("Setting Alarm 1 Seconds [00-59] %02x ", data[idx] & 0x7f);
                print_enabled(data[idx]);
                break;
            case 8: // Alarm 1 Minutes
                state->Alarm1_Min = data[idx];
                printf("Setting Alarm 1 Minutes [00-59] %02x ", data[idx] & 0x7f);
                print_enabled(data[idx]);
                break;
            case 9: // Alarm 1 Hours
                state->Alarm1_Hours = data[idx];
                printf("Setting Alarm 1 Hours [00-23] %02x ", data[idx] & 0x6f);
                print_enabled(data[idx]);
                break;
            case 0xA: // Alarm 1 Day Date
                state->Alarm1_DayDate = data[idx];
                if ((data[idx] & 0x40) > 0) {
                    printf("Setting Alarm 1 Day [1-7] %02x ", data[idx] & 0x3f);
                } else {
                    printf("Setting Alarm 1 Date [0-31] %02x ", data[idx] & 0x3f);
                }
                print_enabled(data[idx]);
                break;
            case 0xB: // Alarm 2 Minutes
                state->Alarm2_Min = data[idx];
                printf("Setting Alarm 2 Minutes [00-59] %02x ", data[idx] & 0x7f);
                print_enabled(data[idx]);
                break;
            case 0xC: // Alarm 2 Hours
                state->Alarm2_Hours = data[idx];
                printf("Setting Alarm 2 Hours [00-23] %02x ", data[idx] & 0x6f);
                print_enabled(data[idx]);
                break;
            case 0xD: // Alarm 2 Day Date
                state->Alarm1_DayDate = data[idx];
                if ((data[idx] & 0x40) > 0) {
                    printf("Setting Alarm 2 Day [1-7] %02x ", data[idx] & 0x3f);
                } else {
                    printf("Setting Alarm 2 Date [0-31] %02x ", data[idx] & 0x3f);
                }
                print_enabled(data[idx]);
                break;
            case 0xE: // Control: EOSC BBSQW CONV RS2 RS1 INTCN A2IE A1IE not implemented
                state->control = data[idx] & 0x1f;
                printf("Setting Oscillator Enabled, ");
                printf("Square-Wave Disabled, ");
                printf("Temp Conversion Finished, ");
                switch ((state->control >> 3) & 0b11) {
                    case 0b00:
                        printf("Frequency 1Hz, ");
                        break;
                    case 0b01:
                        printf("Frequency 1.024kHz, ");
                        break;
                    case 0b10:
                        printf("Frequency 4.096kHz, ");
                        break;
                    case 0b11:
                        printf("Frequency 8.192kHz, ");
                        break;
                }
                if ((state->control & 0b100) > 0) {
                    printf("Interrupt Mode, ");
                } else {
                    printf("Oscillator Mode, ");
                }
                if ((state->control & 0b10) > 0) {
                    printf("Alarm 2 Enabled, ");
                } else {
                    printf("Alarm 2 Disabled, ");
                }
                if ((state->control & 0b1) > 0) {
                    printf("Alarm 1 Enabled");
                } else {
                    printf("Alarm 1 Disabled");
                }
                break;
            case 0xF: // Status: OSF 0 0 0 EN32kHz BSY A2F A1F
                if ((data[idx] & 0x80) > 0) // always active, has no effect
                    printf("Enabling Oscillator, ");
                if ((data[idx] & 0b1000) > 0) {
                    printf("Enabling 32kHz Output, ");
                    temp = 0b1000;
                } else {
                    printf("Disabling 32kHz Output, ");
                }
                if ((data[idx] & 0b10) > 0) {
                    temp += state->status & 0b10;
                } else {
                    printf("Clearing Alarm 2, ");
                }
                if ((data[idx] & 0b1) > 0) {
                    temp += state->status & 0b1;
                } else {
                    printf("Clearing Alarm 1");
                }
                state->status = temp;
                break;
            case 0x10: // Aging offset has no user effect
                state->aging = data[idx];
                printf("Aging offset %02x", data[idx]);
                break;
            case 0x11: // Temperature upper byte not implemented
            case 0x12: // Temperature lower byte not implemented
                printf("The Temperature is read only");
                break;
            default:
                printf(" %02x", data[idx]);
        }
    }
    printf("\n");
    fflush(stdout);
    state->index = (offset + 1) % 0x13;
    return len;

    for(idx = 0; idx<len; idx++) {
        switch (offset) {

        }
    }
    state->index = offset + 1;
    return len;
}

static int i2c_ds3231_read(void *priv, unsigned len, uint8_t *data)
{
    ds3231_state_t *state = priv;
    unsigned idx;
    uint16_t offset;
    struct tm *gmt_time = localtime(&state->system_time);
    printf("READ [%d]: ", len);
    for(idx = 0; idx<len; idx++) {
        offset = (idx + state->index) % 0x13;
        switch (offset) {
            case 0: // BCD Seconds
                // reread clock every time reg 0 is read
                gmt_time = localtime(&state->system_time);
                data[idx] = format_7bit_bcd(gmt_time->tm_sec);
                printf("Seconds [00-59] %02x", data[idx]);
                break;
            case 1: // BCD Minutes
                data[idx] = format_7bit_bcd(gmt_time->tm_min);
                printf("Minutes [00-59] %02x", data[idx]);
                break;
            case 2: // BCD Hours
                if (state->twelve_hour_flag > 0) {
                    if (gmt_time->tm_hour > 11) {
                        data[idx] = format_5bit_bcd(gmt_time->tm_hour - 12) + 0b01100000;
                    } else {
                        data[idx] = format_5bit_bcd(gmt_time->tm_hour - 12) + 0b01000000;
                    }
                    printf("Hours flags + [00-11] %02x", data[idx]);
                } else {
                    data[idx] = format_6bit_bcd(gmt_time->tm_hour);
                    printf("Hours [00-23] %02x", data[idx]);
                }
                break;
            case 3: // 1 based day of week
                data[idx] = gmt_time->tm_wday + 1;
                printf("Day of Week [1-7] %02x", data[idx]);
                break;
            case 4: // 1 based day of month
                data[idx] = format_6bit_bcd(gmt_time->tm_mday);
                printf("Day of Month [1-31] %02x", data[idx]);
                break;
            case 5: // centry + month
                data[idx] = ((gmt_time->tm_year / 100) << 7)+ format_5bit_bcd(gmt_time->tm_mon + 1);
                printf("Month [1-12] %02x + [Century] %02x = %02x",
                    data[idx] & 0b11111, data[idx] >> 7, data[idx]);
                break;
            case 6: // year
                data[idx] = (((gmt_time->tm_year % 100) / 10) << 4)
                + gmt_time->tm_year % 10;
                printf("Year since 1900 [0-99] %02x", data[idx]);
                break;
            case 7: // Alarm 1 Seconds
                data[idx] = state->Alarm1_Sec;
                printf("Alarm 1 Seconds [00-59] %02x ", data[idx] & 0x7f);
                print_enabled(data[idx]);
                break;
            case 8: // Alarm 1 Minutes
                data[idx] = state->Alarm1_Min;
                printf("Alarm 1 Minutes [00-59] %02x ", data[idx] & 0x7f);
                print_enabled(data[idx]);
                break;
            case 9: // Alarm 1 Hours
                data[idx] = state->Alarm1_Hours;
                printf("Alarm 1 Hours [00-23] %02x ", data[idx] & 0x6f);
                print_enabled(data[idx]);
                break;
            case 0xA: // Alarm 1 Day Date
                data[idx] = state->Alarm1_DayDate;
                if ((data[idx] & 0x40) > 0) {
                    printf("Alarm 1 Day [1-7] %02x ", data[idx] & 0x3f);
                } else {
                    printf("Alarm 1 Date [0-31] %02x ", data[idx] & 0x3f);
                }
                print_enabled(data[idx]);
                break;
            case 0xB: // Alarm 2 Minutes
                data[idx] = state->Alarm2_Min;
                printf("Alarm 2 Minutes [00-59] %02x ", data[idx] & 0x7f);
                print_enabled(data[idx]);
                break;
            case 0xC: // Alarm 2 Hours
                data[idx] = state->Alarm2_Hours;
                printf("Alarm 2 Hours [00-23] %02x ", data[idx] & 0x6f);
                print_enabled(data[idx]);
                break;
            case 0xD: // Alarm 2 Day Date
                data[idx] = state->Alarm1_DayDate;
                if ((data[idx] & 0x40) > 0) {
                    printf("Alarm 2 Day [1-7] %02x ", data[idx] & 0x3f);
                } else {
                    printf("Alarm 2 Date [0-31] %02x ", data[idx] & 0x3f);
                }
                print_enabled(data[idx]);
                break;
            case 0xE: // Control: EOSC BBSQW CONV RS2 RS1 INTCN A2IE A1IE not implemented
                data[idx] = state->control;
                printf("Oscillator Enabled, ");
                printf("Square-Wave Disabled, ");
                printf("Temp Conversion Finished, ");
                switch ((state->control >> 3) & 0b11) {
                    case 0b00:
                        printf("Frequency 1Hz, ");
                        break;
                    case 0b01:
                        printf("Frequency 1.024kHz, ");
                        break;
                    case 0b10:
                        printf("Frequency 4.096kHz, ");
                        break;
                    case 0b11:
                        printf("Frequency 8.192kHz, ");
                        break;
                }
                if ((state->control & 0b100) > 0) {
                    printf("Interrupt Mode, ");
                } else {
                    printf("Oscillator Mode, ");
                }
                if ((state->control & 0b10) > 0) {
                    printf("Alarm 2 Enabled, ");
                } else {
                    printf("Alarm 2 Disabled, ");
                }
                if ((state->control & 0b1) > 0) {
                    printf("Alarm 1 Enabled");
                } else {
                    printf("Alarm 1 Disabled");
                }
                break;
            case 0xF: // Status: OSF 0 0 0 EN32kHz BSY A2F A1F
                data[idx] = state->status;
                printf("Oscillator Running, "); // bit 7
                if ((state->status & 0b1000) > 0) { // bit 3
                    printf("32kHz Output Enabled, ");
                } else {
                    printf("32kHz Output Disabled, ");
                }
                printf("Temp Conversion Finished, "); // bit 2
                if ((state->status & 0b10) > 0) {
                    printf("Alarm 2 Active, ");
                } else {
                    printf("Alarm 2 Inactive, ");
                }
                if ((state->status & 0b1) > 0) {
                    printf("Alarm 1 Active");
                } else {
                    printf("Alarm 1 Inactive");
                }
                break;
            case 0x10: // Aging offset has no user effect
                data[idx] = state->aging;
                printf("Aging offset %02x", data[idx]);
                break;
            case 0x11: // Temperature upper byte not implemented
            // statically return 25C
                data[idx] = 0b00011001;
                printf("The Temperature is %02d C", (int8_t) data[idx]);
                break;
            case 0x12: // Temperature lower byte not implemented
            // statically return .25C
                data[idx] = 1 << 6;
                printf(".%02d", (data[idx] > 6) * 25);
                break;
        }
        printf("\n");
        fflush(stdout);
    }
    state->index = (offset + 1) % 0x13;
    return len;
}

static void i2c_ds3231_stop(void *priv)
{
    printf("STOP\n");
    fflush(stdout);
}

static const coremodel_i2c_func_t i2c_ds3231_func = {
    .start = i2c_ds3231_start,
    .write = i2c_ds3231_write,
    .read = i2c_ds3231_read,
    .stop = i2c_ds3231_stop };

int main(int argc, char *argv[])
{
    int res;
    void *handle;
    ds3231_state_t *state;

    if(argc != 3) {
        printf("usage: coremodel-i2c <address[:port]> <i2c>\n");
        return 1;
    }

    res = coremodel_connect(argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    state = malloc(sizeof(ds3231_state_t));
    // Initialize the state of the device to POR values
    time(&state->system_time);
    state->index = 0;
    state->start = 0;
    state->twelve_hour_flag = 0; // reset value is undefined in HW
    state->Alarm1_Sec = 0; // reset value is undefined in HW
    state->Alarm1_Min = 0; // reset value is undefined in HW
    state->Alarm1_Hours = 0; // reset value is undefined in HW
    state->Alarm1_DayDate = 0; // reset value is undefined in HW
    state->Alarm2_Min = 0; // reset value is undefined in HW
    state->Alarm2_Hours = 0; // reset value is undefined in HW
    state->Alarm2_DayDate = 0; // reset value is undefined in HW
    state->control = 0b00011100;
    state->status = 0b10001000;
    state->aging = 0;
    handle = coremodel_attach_i2c(argv[2], 0x42, &i2c_ds3231_func, state, 0);
    if(!handle) {
        fprintf(stderr, "error: failed to attach i2c.\n");
        coremodel_disconnect();
        return 1;
    }

    coremodel_mainloop(-1);

    coremodel_detach(handle);
    coremodel_disconnect();

    return 0;
}