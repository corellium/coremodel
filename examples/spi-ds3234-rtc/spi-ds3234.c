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
typedef struct ds3234_state {
    time_t system_time;
    uint8_t index;
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
    uint8_t SRAM_Address;
    uint8_t *SRAM_Data;
} ds3234_state_t;

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

static void spi_ds3234_cs(void *priv, unsigned csel)
{
    if(!csel)
        return;
    // cast the void * into the "this" pointer
    ds3234_state_t *state = priv;
    printf("CS Asserted\n");

    // read the system time
    struct tm *gmt_time = localtime(&state->system_time);
    int old_second = gmt_time->tm_sec;
    int old_minute = gmt_time->tm_min;
    time(&state->system_time);
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
    // last CS, rather it only alerts if it should be on now
    fflush(stdout);
}

static uint8_t access_reg(ds3234_state_t *state, unsigned ADDR, unsigned WRITE, uint8_t data)
{
    struct tm *gmt_time = localtime(&state->system_time);
    switch (ADDR) {
        case 0: // BCD Seconds
            if (WRITE) {
                printf("Ignoring write of %02x to seconds register", data);
            } else{ 
                // reread clock every time reg 0 is read
                gmt_time = localtime(&state->system_time);
                data = format_7bit_bcd(gmt_time->tm_sec);
                printf("Seconds [00-59] %02x", data);
            }
            break;
        case 1: // BCD Minutes
            if(WRITE) {
                printf("Ignoring write of %02x to minutes register", data);
            } else{
                data = format_7bit_bcd(gmt_time->tm_min);
                printf("Minutes [00-59] %02x", data);
            }
            break;
        case 2: // BCD Hours
            if(WRITE) {
                if ((data & 0x40) > 0) {
                    printf("Setting AM/PM mode");
                    state->twelve_hour_flag = 1;
                } else {
                    printf("Setting 24 hour mode");
                    state->twelve_hour_flag = 0;
                }
            } else {
                if (state->twelve_hour_flag > 0) {
                    if (gmt_time->tm_hour > 11) {
                        data = format_5bit_bcd(gmt_time->tm_hour - 12) + 0b01100000;
                    } else {
                        data = format_5bit_bcd(gmt_time->tm_hour - 12) + 0b01000000;
                    }
                    printf("Hours flags + [00-11] %02x", data);
                } else {
                    data = format_6bit_bcd(gmt_time->tm_hour);
                    printf("Hours [00-23] %02x", data);
                }
            }
            break;
        case 3: // 1 based day of week
            if(WRITE) {
                printf("Ignoring write of %02x to day of week register", data);
            } else {
                data = gmt_time->tm_wday + 1;
                printf("Day of Week [1-7] %02x", data);
            }
            break;
        case 4: // 1 based day of month
            if(WRITE) {
                printf("Ignoring write of %02x to day of month register", data);
            } else {
                data = format_6bit_bcd(gmt_time->tm_mday);
                printf("Day of Month [1-31] %02x", data);
            }
            break;
        case 5: // centry + month
            if(WRITE) {
                printf("Ignoring write of %02x to month register", data);
            } else {
                data = ((gmt_time->tm_year / 100) << 7)+ format_5bit_bcd(gmt_time->tm_mon + 1);
                printf("Month [1-12] %02x + [Century] %02x = %02x",
                    data & 0b11111, data >> 7, data);
            }
            break;
        case 6: // year
            if(WRITE) {
                printf("Ignoring write of %02x to year register", data);
            } else {
                data = (((gmt_time->tm_year % 100) / 10) << 4)
                + gmt_time->tm_year % 10;
                printf("Year since 1900 [0-99] %02x", data);
            }
            break;
        case 7: // Alarm 1 Seconds
                if(WRITE) {
                state->Alarm1_Sec = data;
                printf("Setting ");
            } else {
                data = state->Alarm1_Sec;
            }
            printf("Alarm 1 Seconds [00-59] %02x ", data & 0x7f);
            print_enabled(data);
            break;
        case 8: // Alarm 1 Minutes
            if(WRITE) {
                state->Alarm1_Min = data;
                printf("Setting ");
            } else {
                data = state->Alarm1_Min;
            }
            printf("Alarm 1 Minutes [00-59] %02x ", data & 0x7f);
            print_enabled(data);
            break;
        case 9: // Alarm 1 Hours
            if(WRITE) {
                state->Alarm1_Hours = data;
                printf("Setting ");
            } else {
                data= state->Alarm1_Hours;
            }
            printf("Alarm 1 Hours [00-23] %02x ", data & 0x6f);
            print_enabled(data);
            break;
        case 0xA: // Alarm 1 Day Date
            if(WRITE) {
                state->Alarm1_DayDate = data;
                printf("Setting ");
            } else {
                data = state->Alarm1_DayDate;
            }
            if ((data & 0x40) > 0) {
                printf("Alarm 1 Day [1-7] %02x ", data & 0x3f);
            } else {
                printf("Alarm 1 Date [0-31] %02x ", data & 0x3f);
            }
            print_enabled(data);
            break;
        case 0xB: // Alarm 2 Minutes
            if(WRITE) {
                state->Alarm2_Min = data;
                printf("Setting ");
            } else {
                data = state->Alarm2_Min;
            }
            printf("Alarm 2 Minutes [00-59] %02x ", data & 0x7f);
            print_enabled(data);
            break;
        case 0xC: // Alarm 2 Hours
            if(WRITE) {
                state->Alarm2_Hours = data;
                printf("Setting ");
            } else {
                data = state->Alarm2_Hours;
            }
            printf("Alarm 2 Hours [00-23] %02x ", data & 0x6f);
            print_enabled(data);
            break;
        case 0xD: // Alarm 2 Day Date
            if(WRITE) {
                state->Alarm1_DayDate = data;
                printf("Setting ");
            } else {
                data = state->Alarm1_DayDate;
            }
            if ((data & 0x40) > 0) {
                printf("Alarm 2 Day [1-7] %02x ", data & 0x3f);
            } else {
                printf("Alarm 2 Date [0-31] %02x ", data & 0x3f);
            }
            print_enabled(data);
            break;
        case 0xE: // Control: EOSC BBSQW CONV RS2 RS1 INTCN A2IE A1IE not implemented
            if(WRITE) {
                state->control = data & 0x1f;
                printf("Setting ");
            } else {
                data = state->control;
            }
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
            if(WRITE) {
                // set bit 3
                state->status = (state->status & 0xf7) | (data & 0b1000);
                // bits 1 and 0, clear on 0 ignore 1
                if ((data & 0b10) == 0)
                    state->status &= 0xfd;
                if ((data & 0b1) == 0)
                    state->status &= 0xfe;
            }
            data = state->status;
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
            if(WRITE) {
                state->aging = data;
            } else {
                data = state->aging;
            }
            printf("Aging offset %02x", data);
            break;
        case 0x11: // Temperature upper byte not implemented
            // statically return 25C
            data = 0b00011001;
            printf("The Temperature is %02d C", (int8_t) data);
            break;
        case 0x12: // Temperature lower byte not implemented
            // statically return .25C
            data = 1 << 6;
            printf(".%02d", (data > 6) * 25);
            break;
        case 0x18: // SRAM Address Register
            if(WRITE) {
                state->SRAM_Address = data;
            } else {
                data = state->SRAM_Address;
            }
            printf("SRAM Address = %02d", data);
            break;
        case 0x19: // SRAM Data
            if(WRITE) {
                state->SRAM_Data[state->SRAM_Address] = data;
            } else {
                data = state->SRAM_Data[state->SRAM_Address];
            }
            printf("SRAM Data[%02d] = %02d", state->SRAM_Address, data);
            state->SRAM_Address = (state->SRAM_Address + 1) % 0xff; // & 0xff likely redundant
            break;
    }
    printf("\n");
    fflush(stdout);
    return data;
}

static int spi_ds3234_xfr(void *priv, unsigned len, uint8_t *wrdata, uint8_t *rddata)
{
    unsigned idx;
    ds3234_state_t *state = priv;
    unsigned WRITE = wrdata[0] & 0x80;
    unsigned ADDR = wrdata[0] & 0x7f;

    // dump intput data
    printf("RX [%d]", len);
    for(idx=0; idx<len; idx++)
        printf(" %02x", wrdata[idx]);
    printf("\n");
    fflush(stdout);
        
    rddata[0] = 0; // per spec first read byte is always high impedance

    if(ADDR < 0x13) { // RTC Registers
        if(!WRITE && len == 1) // special case for read one byte
            rddata[0] = access_reg(state, ADDR, WRITE, wrdata[0]);
        for(idx = 1; idx<len; idx++) {
            rddata[idx] = access_reg(state, ((ADDR + idx - 1) % 0x13), WRITE, wrdata[idx]);
        }
    } else if (ADDR == 0x18) { // SRAM Address
        if(len == 1) {
            if(!WRITE) // Read out the address as a magic first byte
                rddata[0] = access_reg(state, ADDR, WRITE, wrdata[0]);
        } else { // read or write the address as the second byte
            rddata[1] = access_reg(state, ADDR, WRITE, wrdata[1]);
        }
        for(idx = 2; idx<len; idx++) {
            rddata[idx] = 0; // ignore additional bytes
        }
    } else if (ADDR == 0x19) { //SRAM Data
        if(!WRITE && len == 1) // special case for read one byte
            rddata[0] = access_reg(state, ADDR, WRITE, wrdata[0]);
        for(idx = 1; idx<len; idx++) {
            rddata[idx] = access_reg(state, ADDR, WRITE, wrdata[idx]);
        }
    } else { // undefined per spec, assumed high impedance, implemented as byte count 
        for(idx=0; idx<len; idx++) {
            rddata[idx] = '0' + (idx & 63);
        }
    }
    
    if(ADDR < 0x20) {
        if(WRITE)
            printf("Write\n");
        else
            printf("Read\n");
        fflush(stdout);
    }
        
    printf("TX [%d]", len);
    for(idx=0; idx<len; idx++) {
        printf(" %02x", rddata[idx]);
    }
    printf("\n");
    fflush(stdout);
    return len;
}

static const coremodel_spi_func_t spi_ds3234_func = {
    .cs = spi_ds3234_cs,
    .xfr = spi_ds3234_xfr };
 
int main(int argc, char *argv[])
{
    int res;
    void *handle;
    ds3234_state_t *state;

    if(argc != 4) {
        printf("usage: coremodel-spi <address[:port]> <spi> <cs>\n");
        return 1;
    }

    res = coremodel_connect(argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    state = malloc(sizeof(ds3234_state_t));
    // Initialize the state of the device to POR values
    time(&state->system_time);
    state->index = 0;
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
    state->SRAM_Address = 0;
    state->SRAM_Data = malloc(sizeof(uint8_t)*256);
    handle = coremodel_attach_spi(argv[2], atoi(argv[3]), &spi_ds3234_func, state, COREMODEL_SPI_BLOCK);
    if(!handle) {
        fprintf(stderr, "error: failed to attach SPI.\n");
        coremodel_disconnect();
        return 1;
    }

    coremodel_mainloop(-1);

    coremodel_detach(handle);
    coremodel_disconnect();

    return 0;
}
