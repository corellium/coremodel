/*
 * CoreModel ADXL36X Example
 *
 * Copyright (c) 2026 Corellium Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <string.h>

#include <coremodel.h>
#include "coremodel-adxl36x.h"

#define FIFO_SIZE    2048
#define MAX_TIMERS   8
#define PIN_NUM      2

#define ADI_ADXL36x_DEBUG 1

typedef struct adi_adxl36x_state{

    struct timer_evt{
       void *state;
       struct epoll_event *eevt;
       struct itimerspec itime;
       int tfd;
       void (*comp_cb)(void *priv, unsigned idx);
    }tmevt[MAX_TIMERS]; //timer 0 base counter

    struct epoll_event *eevto;

    int epfd;

    pthread_mutex_t lock;
    pthread_t timer_thread;
    pthread_t cm_thread;
    pthread_cond_t cm_cond;
    unsigned cycle_time;

    void *cm;
    void *handle;

    struct coremodel_gpio_pin {
        struct adi_adxl36x_state *state;
        void *pin_handle; //coremodel_if
        int mvolt;
    } pin[PIN_NUM];

    uint64_t timer_count;
    atomic_bool run;
    uint32_t err;

    uint8_t devid_ad, devid_mst, part_id, rev_id, serial_number_2, serial_number_1, serial_number_0;
    uint8_t status[3], fifo_entries_l, fifo_entries_h, xdata_h, xdata_l, ydata_h, ydata_l, zdata_h, zdata_l;
    uint8_t temp_h, temp_l, ex_adc_h, ex_adc_l, soft_reset, thresh_act_h, thresh_act_l, time_act;
    uint8_t thresh_inact_h, thresh_inact_l, time_inact_h, time_inact_l, act_inact_ctl, fifo_control, fifo_samples;
    uint8_t intmapn_lower[2], filter_ctl, power_ctl, self_test, tap_thresh, tap_dur, tap_latent, tap_window;
    uint8_t x_offset, y_offset, z_offset, x_sens, y_sens, z_sens, timer_ctl, intmapn_upper[2], adc_ctl;
    uint8_t temp_ctl, temp_adc_over_thrsh_h, temp_adc_over_thrsh_l, temp_adc_under_thrsh_h, temp_adc_under_thrsh_l;
    uint8_t temp_adc_timer, axis_mask, pedometer_step_cnt_h, pedometer_step_cnt_l;
    uint8_t pedometer_ctl, pedometer_thres_h, pedometer_thres_l, pedometer_sens_h, pedometer_sens_l;
    uint8_t addr, count, stc, tap_sample, tap_state;
    uint32_t activity_mode, data_range, tme_init, act_threshold, inact_threshold, temp_threshold_h, temp_threshold_l, time_inact;
    uint32_t act_ref[3], inact_ref[3], time_temp_inact, time_temp_act, sample_count, sample_fifo_wpt, sample_fifo_rpt, read_count, sample_size, fifo_count;
    uint16_t sample_fifo[FIFO_SIZE], fifo_trig;
    uint64_t tme_cnt, tap_time;
    int64_t accel[3], temp;
    float odr;

    unsigned irq_line[2];

} adi_adxl36x_state_t;

const static int self_test_value[3][2] = { { 1440, 4320 }, { 1440*2, 4320*2 }, { 1440*4, 4320*4 } }; //360 << 2, 1080 << 2
const static int range_scale[3] = { 1, 2, 4 };

static void gpio_notify(void *priv, int mvolt)
{

    struct coremodel_gpio_pin *pin = priv;
    adi_adxl36x_state_t *state = pin->state;
    unsigned pin_idx = pin - state->pin;


    printf("GPIO[%d] = %d mV\n", pin_idx, mvolt);
    fflush(stdout);

    pin->mvolt = mvolt;
}

static const coremodel_gpio_func_t gpio_func = {
    .notify = gpio_notify };


void timer_evt_init(void *priv, struct epoll_event *eevt, struct epoll_event *events);
void timer_evt_free(void *priv);
void timer_evt_reset(void *priv);

void timer_evt_start(void *priv);
void timer_evt_stop(void *priv);

void timer_evt_set_frequency(adi_adxl36x_state_t *state, float hz);
void timer_evt_set_oneshot(struct timer_evt *tmevt, unsigned nsec);

static void adi_adxl36x_irq_update(adi_adxl36x_state_t *state)
{
    unsigned irql;

    for(int idx = 0; idx < 2; idx++){
    irql  = !!((state->status[0] & state->intmapn_lower[idx] & 0x7f) | (state->status[1] & state->intmapn_upper[idx] & 0xbf) | (state->status[0] & state->intmapn_upper[idx] & 0x80));

        if(irql != state->irq_line[idx]) {
            if(state->pin[idx].pin_handle){
                coremodel_gpio_set(state->pin[idx].pin_handle, 1, state->pin[idx].mvolt ? 0 : 3300);
            }
            state->irq_line[idx] = irql;
        }
    }
}

#if ADI_ADXL36x_DEBUG
static void adi_adxl36x_mmio_debug(unsigned index, unsigned addr, uint32_t size, uint64_t val, char *tag)
{
    char *name = adi_adxl36x_reg_name(addr);
    if(name) {
        printf("[adi-adxl36x:%d] %s%d %04x(%s) %08lx: ", index, tag, size, addr, name, val);
        adi_adxl36x_print_fields(addr, val, printf);
        printf("\n");
    } else
        printf("[adi-adxl36x:%d] %s%d %04x %08lx\n", index, tag, size, addr, val);
}
#else
#define adi_adxl36x_mmio_debug(...) do {} while(0)
#endif

static void adi_adxl36x_fifo_sample(void *priv){
    adi_adxl36x_state_t *state = priv;
    uint32_t fifo_samples = ((state->fifo_control & REG_FIFO_CONTROL_FIFO_SAMPLES) << 6) | state->fifo_samples;
    uint32_t fifo_rsize =  state->sample_fifo_wpt - state->sample_fifo_rpt;
    uint32_t fifo_wsize = FIFO_SIZE - fifo_rsize;
    uint32_t control = state->fifo_control & REG_FIFO_CONTROL_FIFO_MODE_MASK;
    uint16_t temp_buf[4] = {0};


    switch(control){
    case 1:
        //oldest save mode
        if(!fifo_wsize){
            state->status[0] |= REG_STATUS_FIFO_OVER_RUN;
            return;
        }
        break;
    case 2:
        //stream mode
        break;
    case 3:
        //trigger mode
        if((state->sample_count == fifo_samples) || !state->fifo_trig || !fifo_wsize){
            return;
        }
        break;
    }

    temp_buf[0] = state->accel[0];
    temp_buf[1] = state->accel[0];
    temp_buf[2] = state->accel[0];
    temp_buf[3] = state->temp;

    switch((state->adc_ctl & REG_ADC_CTL_FIFO_8_12BIT_MASK) >> REG_ADC_CTL_FIFO_8_12BIT_SHIFT){
    case 1:
        temp_buf[0] >>= 6;
        temp_buf[1] >>= 6;
        temp_buf[2] >>= 6;
        temp_buf[3] >>= 6;

        break;
    case 2:
        temp_buf[0] >>= 4;
        temp_buf[1] >>= 4;
        temp_buf[2] >>= 4;
        temp_buf[3] >>= 4;
        break;
    case 0:
    case 3:
        temp_buf[1] |= 1 << 14;
        temp_buf[2] |= 2 << 14;
        temp_buf[3] |= 3 << 14;
        break;
    }

    switch(state->fifo_control & REG_FIFO_CONTROL_CHANNEL_SELECT_MASK){
    case REG_FIFO_CONTROL_CHANNEL_SELECT_XYZ:
        if(fifo_wsize < 3){
            return;
        }
        state->sample_size = 3;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[0];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[1];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[2];
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count += 3;
        break;
    case REG_FIFO_CONTROL_CHANNEL_SELECT_X:
        if(fifo_wsize < 1){
            return;
        }
        state->sample_size = 1;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[0];
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count++;
        break;
    case REG_FIFO_CONTROL_CHANNEL_SELECT_Y:
        if(fifo_wsize < 1){
            return;
        }
        state->sample_size = 1;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[1];
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count++;
        break;
    case REG_FIFO_CONTROL_CHANNEL_SELECT_Z:
        if(fifo_wsize < 1){
            return;
        }
        state->sample_size = 1;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[2];
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count++;
        break;
    case REG_FIFO_CONTROL_CHANNEL_SELECT_XYZT:
        if(fifo_wsize < 4){
            return;
        }
        state->sample_size = 4;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[0];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[1];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[2];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[3];
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count += 4;
        break;
    case REG_FIFO_CONTROL_CHANNEL_SELECT_XT:
        if(fifo_wsize < 2){
            return;
        }
        state->sample_size = 2;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[0];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[3];
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count += 2;
        break;
    case REG_FIFO_CONTROL_CHANNEL_SELECT_YT:
        if(fifo_wsize < 2){
            return;
        }
        state->sample_size = 2;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[1];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[3];
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count += 2;
        break;
    case REG_FIFO_CONTROL_CHANNEL_SELECT_ZT:
        if(fifo_wsize < 2){
            return;
        }
        state->sample_size = 2;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[2];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[3];
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count += 2;
        break;
    case REG_FIFO_CONTROL_CHANNEL_SELECT_XYZA:
        if(fifo_wsize < 4){
            return;
        }
        state->sample_size = 4;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[0];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[1];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[2];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = 0;
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count += 4;
        break;
    case REG_FIFO_CONTROL_CHANNEL_SELECT_XA:
        if(fifo_wsize < 2){
            return;
        }
        state->sample_size = 2;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[0];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = 0;
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count += 2;
        break;
    case REG_FIFO_CONTROL_CHANNEL_SELECT_YA:
        if(fifo_wsize < 2){
            return;
        }
        state->sample_size = 2;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[1];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = 0;
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count += 2;
        break;
    case REG_FIFO_CONTROL_CHANNEL_SELECT_ZA:
        if(fifo_wsize < 2){
            return;
        }
        state->sample_size = 2;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = temp_buf[2];
        state->sample_fifo_wpt++;
        state->sample_fifo[state->sample_fifo_wpt%FIFO_SIZE] = 0;
        state->sample_fifo_wpt++;
        state->sample_count++;
        state->fifo_count += 2;
        break;
    }

    if(state->sample_count >= fifo_samples){
        state->status[0] |= REG_STATUS_FIFO_WATER_MARK;
    }

}

static void adi_adxl36x_accel_convert(void *priv)
{
    adi_adxl36x_state_t *state = priv;

    //get accel data in m/s^2

    for(int i = 0; i < 3; i++){
        state->accel[i] = 2509;       //TODO dummy data Q56.8 9.8 m/s^2
        state->accel[i] *= 256000000; //Q56.8  256000000  m/s^2 to mm/s^2 * 1000 Q56.8 -> Q48.16
        state->accel[i] += 32768;     //Q48.16 32768 rounding 0.5
        state->accel[i] /= 2510502;   //Q56.8  2510502 9806.65 mm/s^2  Q48.16 -> Q56.8

        state->accel[i] /= range_scale[(state->filter_ctl & REG_FILTER_CTL_RANGE_MASK) >> REG_FILTER_CTL_RANGE_SHIFT]; //apply scalar factor
        state->accel[i] >>= 4;

    }
    state->xdata_h = (state->accel[0] & 0xff00) >> 8;
    state->xdata_l = (state->accel[0] & 0x00fc);
    state->ydata_h = (state->accel[1] & 0xff00) >> 8;
    state->ydata_l = (state->accel[1] & 0x00fc);
    state->zdata_h = (state->accel[2] & 0xff00) >> 8;
    state->zdata_l = (state->accel[2] & 0x00fc);

    for(int i = 0; i < 3; i++){
        state->accel[i] >>= 2;
    }
}

static void adi_adxl36x_temp_convert(void *priv)
{
    adi_adxl36x_state_t *state = priv;

    state->temp = 6400;        //TODO dummy data Q56.8 25c
    state->temp -= 6400;       //Q56.8 -25
    state->temp *= 13824;      //Q48.16 *54
    state->temp >>= 8;         //Q56.8
    state->temp += 42240;      //Q56.8 +165
    state->temp >>= 6;

    state->temp_h = (state->temp & 0xff00) >> 8;
    state->temp_l = (state->temp & 0x00fc);
    state->temp >>= 2;
}

static void adi_adxl36x_stop(void *priv)
{
    adi_adxl36x_state_t *state = priv;

    #if ADI_ADXL36x_DEBUG
    printf("adxl36x STOP \n");
    #endif

    state->count = 0;
    state->addr = 0;
}

static int adi_adxl36x_start(void *priv)
{
    adi_adxl36x_state_t *state = priv;

    pthread_mutex_lock(&state->lock);

    #if ADI_ADXL36x_DEBUG
    printf("adxl36x START \n");
    #endif

    if(state->count == 0){
        if((state->power_ctl & REG_POWER_CTL_MEASURE_MASK) == 2){ //measure mode
            adi_adxl36x_accel_convert(state);
        }
        if(state->temp_ctl & REG_TEMP_CTL_TEMP_EN){
            adi_adxl36x_temp_convert(state);
        }
    }

    pthread_mutex_unlock(&state->lock);
    return 1;
}

static void adi_adxl36x_reset(void *priv)
{
    adi_adxl36x_state_t *state = priv;

    state->devid_ad = 0xAD;
    state->devid_mst = 0x1D;
    state->part_id = 0xF7;
    state->rev_id = 0x3; //366 0x5 367 0x3
    state->serial_number_2 = 0;
    state->serial_number_1 = 0;
    state->serial_number_0 = 0;
    state->status[0] = REG_STATUS_AWAKE;
    state->fifo_entries_l = 0;
    state->fifo_entries_h = 0;
    state->xdata_h = 0;
    state->xdata_l = 0;
    state->ydata_h = 0;
    state->ydata_l = 0;
    state->zdata_h = 0;
    state->zdata_l = 0;
    state->temp_h = 0;
    state->temp_l = 0;
    state->ex_adc_h = 0;
    state->ex_adc_l = 0;
    state->soft_reset = 0x52;
    state->thresh_act_h = 0;
    state->thresh_act_l = 0;
    state->time_act = 0;
    state->thresh_inact_h = 0;
    state->thresh_inact_l = 0;
    state->time_inact_h = 0;
    state->time_inact_l = 0;
    state->act_inact_ctl = 0;
    state->fifo_control = 0;
    state->fifo_samples = 0x80;
    state->intmapn_lower[0] = 0;
    state->intmapn_lower[1] = 0;
    state->filter_ctl = REG_FILTER_CTL_I2C_HS | (3 << REG_FILTER_CTL_ODR_SHIFT);
    state->power_ctl = 0;
    state->self_test = 0;
    state->tap_thresh = 0;
    state->tap_dur = 0;
    state->tap_latent = 0;
    state->tap_window = 0;
    state->x_offset = 0;
    state->y_offset = 0;
    state->z_offset = 0;
    state->x_sens = 0;
    state->y_sens = 0;
    state->z_sens = 0;
    state->timer_ctl = 0;
    state->intmapn_upper[0] = 0;
    state->intmapn_upper[1] = 0;
    state->adc_ctl = 0;
    state->temp_ctl = 0;
    state->temp_adc_over_thrsh_h = 0;
    state->temp_adc_over_thrsh_l = 0;
    state->temp_adc_under_thrsh_h = 0;
    state->temp_adc_under_thrsh_l = 0;
    state->temp_adc_timer = 0;
    state->axis_mask = 0;
    state->status[1] = 0;
    state->status[2] = 0;
    state->pedometer_step_cnt_h = 0;
    state->pedometer_step_cnt_l = 0;
    state->stc = 0;
    state->tap_state = 0;
    state->tap_sample = 0;
    state->tap_time = 0;

    for(int i = 0; i < 3; i++){
        state->accel[i] = 256;
        state->act_ref[i] = 0;
        state->inact_ref[i] = 0;
    }

    state->sample_count = 0;
    state->sample_fifo_wpt = 0;
    state->sample_fifo_rpt = 0;
    state->fifo_trig = 0;

    memset(state->sample_fifo, 0, sizeof(state->sample_fifo));

    state->tme_init = 0;
    state->odr = 100;
    state->tme_cnt = 0;

    timer_evt_reset(state);

    state->timer_count = 0;

}

static int adi_adxl36x_write(void *priv, unsigned len, uint8_t *data)
{
    adi_adxl36x_state_t *state = priv;
    unsigned i;

    pthread_mutex_lock(&state->lock);

    timer_evt_stop(state);

    state->tme_cnt = state->timer_count; // only can do inside lock as it is shared with the timer thread

    for(i=0; i<len; i++){

        if(state->count == 0){
            state->addr = 0x7f & data[i];
#if ADI_ADXL36x_DEBUG
            printf("adxl36x write address %04x length %d \n", state->addr, len);
#endif
            state->count++;
            continue;
        }

        adi_adxl36x_mmio_debug(0, state->addr, len, data[i], "W");

        switch(state->addr) {
        case REG_SOFT_RESET:
            state->soft_reset = data[i] & REG_SOFT_RESET_SOFT_RESET;
            break;
        case REG_THRESH_ACT_H:
            state->thresh_act_h = data[i] & REG_THRESH_ACT_H_DATA_MASK;
            break;
        case REG_THRESH_ACT_L:
            state->thresh_act_l = data[i] & REG_THRESH_ACT_L_DATA_MASK;
            state->act_threshold = (state->thresh_act_l >> 2) | (state->thresh_act_h << 6);
            break;
        case REG_TIME_ACT:
            state->time_act = data[i];
            if(((state->act_inact_ctl & REG_ACT_INACT_CTL_ACT_EN_MASK) == 1) || ((state->act_inact_ctl & REG_ACT_INACT_CTL_ACT_EN_MASK) == 3)){
                timer_evt_set_oneshot(&state->tmevt[1], (unsigned)(( (float)state->time_act / state->odr) * 1000000000));

            }
            break;
        case REG_THRESH_INACT_H:
            state->thresh_inact_h = data[i] & REG_THRESH_INACT_H_DATA_MASK;
            break;
        case REG_THRESH_INACT_L:
            state->thresh_inact_l = data[i] & REG_THRESH_INACT_L_DATA_MASK;
            state->inact_threshold = (state->thresh_inact_l >> 2) | (state->thresh_inact_h << 6);
            break;
        case REG_TIME_INACT_H:
            state->time_inact_h = data[i];
            break;
        case REG_TIME_INACT_L:
            state->time_inact_l = data[i];
            state->time_inact = (state->time_inact_l >> 2) | (state->time_inact_h << 6);
            if((((state->act_inact_ctl & REG_ACT_INACT_CTL_INTACT_EN_MASK) >> REG_ACT_INACT_CTL_INTACT_EN_SHIFT) == 1) ||
                    (((state->act_inact_ctl & REG_ACT_INACT_CTL_INTACT_EN_MASK) >> REG_ACT_INACT_CTL_INTACT_EN_SHIFT) == 3)){
                timer_evt_set_oneshot(&state->tmevt[2], (unsigned)(( (float)state->time_inact / state->odr) * 1000000000));
            }
            break;
        case REG_ACT_INACT_CTL:
            state->act_inact_ctl = data[i] & (REG_ACT_INACT_CTL_REG_READBACK_MASK | REG_ACT_INACT_CTL_LINKLOOP_MASK | REG_ACT_INACT_CTL_INTACT_EN_MASK |
                REG_ACT_INACT_CTL_ACT_EN_MASK);
            state->activity_mode = (state->act_inact_ctl & REG_ACT_INACT_CTL_LINKLOOP_MASK) >> REG_ACT_INACT_CTL_LINKLOOP_SHIFT;
            break;
        case REG_FIFO_CONTROL:
            state->fifo_control = data[i] & (REG_FIFO_CONTROL_CHANNEL_SELECT_MASK | REG_FIFO_CONTROL_FIFO_SAMPLES | REG_FIFO_CONTROL_FIFO_MODE_MASK);
            break;
        case REG_FIFO_SAMPLES:
            state->fifo_samples = data[i];
            break;
        case REG_INTMAP1_LOWER:
            state->intmapn_lower[0] = data[i] & (REG_INTMAP1_LOWER_INT_LOW_INT | REG_INTMAP1_LOWER_AWAKE_INT | REG_INTMAP1_LOWER_INACT_INT |
                REG_INTMAP1_LOWER_ACT_INT | REG_INTMAP1_LOWER_FIFO_OVERRUN_INT | REG_INTMAP1_LOWER_FIFO_WATER_MARK_INT | REG_INTMAP1_LOWER_FIFO_RDY_INT |
                REG_INTMAP1_LOWER_DATA_RDY_INT);
            if(state->intmapn_lower[0] & REG_INTMAP1_LOWER_INT_LOW_INT){
                if(state->pin[0].pin_handle){
                    coremodel_gpio_set(state->pin[0].pin_handle, 1, 3300);
                }
            }
            break;
        case REG_INTMAP2_LOWER:
            state->intmapn_lower[1] = data[i] & (REG_INTMAP2_LOWER_INT_LOW_INT | REG_INTMAP2_LOWER_AWAKE_INT | REG_INTMAP2_LOWER_INACT_INT |
                REG_INTMAP2_LOWER_ACT_INT | REG_INTMAP2_LOWER_FIFO_OVERRUN_INT | REG_INTMAP2_LOWER_FIFO_WATER_MARK_INT | REG_INTMAP2_LOWER_FIFO_RDY_INT |
                REG_INTMAP2_LOWER_DATA_RDY_INT);
            if(state->intmapn_lower[1] & REG_INTMAP1_LOWER_INT_LOW_INT){
                if(state->pin[1].pin_handle){
                    coremodel_gpio_set(state->pin[1].pin_handle, 1, 3300);
                }
            }
            break;
        case REG_FILTER_CTL:
            state->filter_ctl = data[i] & (REG_FILTER_CTL_RANGE_MASK | REG_FILTER_CTL_I2C_HS | REG_FILTER_CTL_EXT_SAMPLE | REG_FILTER_CTL_ODR_MASK);

            state->data_range = (state->filter_ctl & REG_FILTER_CTL_RANGE_MASK) >> REG_FILTER_CTL_RANGE_SHIFT;

            switch(state->filter_ctl & REG_FILTER_CTL_ODR_MASK){
            case 0: // 12.5 Hz
                state->odr = 12.5; // 800; tick offset at 10Khz
                break;
            case 1: // 25 Hz
                state->odr = 25; // 400;
                break;
            case 2: // 50 Hz
                state->odr = 50; // 200;
                break;
            case 3: // 100 Hz
                state->odr = 100; // 100;
                break;
            case 4: // 200 Hz
                state->odr = 200; // 50;
                break;
            case 5: // 400 Hz
                state->odr = 400; // 25;
                break;
            }
            state->timer_count = 0;
            timer_evt_set_frequency(state, state->odr);
            break;
        case REG_POWER_CTL:
            state->power_ctl = data[i] & (REG_POWER_CTL_EXT_CLK | REG_POWER_CTL_NOISE_MASK | REG_POWER_CTL_WAKEUP | REG_POWER_CTL_AUTOSLEEP |
                REG_POWER_CTL_MEASURE_MASK);
            break;
        case REG_SELF_TEST:
            state->self_test = data[i] & (REG_SELF_TEST_ST_FORCE | REG_SELF_TEST_ST);
            break;
        case REG_TAP_THRESH:
            state->tap_thresh = data[i];
            break;
        case REG_TAP_DUR:
            state->tap_dur = data[i];
            break;
        case REG_TAP_LATENT:
            state->tap_latent = data[i];
            break;
        case REG_TAP_WINDOW:
            state->tap_window = data[i];
            break;
        case REG_X_OFFSET:
            state->x_offset = data[i] & REG_X_OFFSET_USER_OFFSET_MASK;
            break;
        case REG_Y_OFFSET:
            state->y_offset = data[i] & REG_Y_OFFSET_USER_OFFSET_MASK;
            break;
        case REG_Z_OFFSET:
            state->z_offset = data[i] & REG_Z_OFFSET_USER_OFFSET_MASK;
            break;
        case REG_X_SENS:
            state->x_sens = data[i] & REG_X_SENS_USER_SENS_MASK;
            break;
        case REG_Y_SENS:
            state->y_sens = data[i] & REG_Y_SENS_USER_SENS_MASK;
            break;
        case REG_Z_SENS:
            state->z_sens = data[i] & REG_Z_SENS_USER_SENS_MASK;
            break;
        case REG_TIMER_CTL:
            state->timer_ctl = data[i] & (REG_TIMER_CTL_WAKEUP_RATE_MASK | REG_TIMER_CTL_TIMER_KEEP_ALIVE_MASK);
            break;
        case REG_INTMAP1_UPPER:
            state->intmapn_upper[0] = data[i] & (REG_INTMAP1_UPPER_ERR_FUSE_INT | REG_INTMAP1_UPPER_ERR_USER_REGS_INT | REG_INTMAP1_UPPER_KPLAV_TIMER_INT |
                REG_INTMAP1_UPPER_TEMP_ADC_HI_INT | REG_INTMAP1_UPPER_TEMP_ADC_LOW_INT | REG_INTMAP1_UPPER_TAP_TWO_INT | REG_INTMAP1_UPPER_TAP_ONE_INT);
            break;
        case REG_INTMAPS2_UPPER:
            state->intmapn_upper[1] = data[i] & (REG_INTMAPS2_UPPER_ERR_FUSE_INT | REG_INTMAPS2_UPPER_ERR_USER_REGS_INT | REG_INTMAPS2_UPPER_KPLAV_TIMER_INT |
                REG_INTMAPS2_UPPER_TEMP_ADC_HI_INT | REG_INTMAPS2_UPPER_TEMP_ADC_LOW_INT | REG_INTMAPS2_UPPER_TAP_TWO_INT | REG_INTMAPS2_UPPER_TAP_ONE_INT);
            break;
        case REG_ADC_CTL:
            state->adc_ctl = data[i] & (REG_ADC_CTL_FIFO_8_12BIT_MASK | REG_ADC_CTL_ADC_INACT_EN | REG_ADC_CTL_ADC_ACT_EN | REG_ADC_CTL_ADC_EN);
            break;
        case REG_TEMP_CTL:
            state->temp_ctl = data[i] & (REG_TEMP_CTL_NL_COMP_EN | REG_TEMP_CTL_TEMP_INACT_EN | REG_TEMP_CTL_TEMP_ACT_EN | REG_TEMP_CTL_TEMP_EN);
            break;
        case REG_TEMP_ADC_OVER_THRSH_H:
            state->temp_adc_over_thrsh_h = data[i] & REG_TEMP_ADC_OVER_THRSH_H_THRESH_MASK;
            state->temp_threshold_h = state->temp_adc_over_thrsh_h << 6;
            break;
        case REG_TEMP_ADC_OVER_THRSH_L:
            state->temp_adc_over_thrsh_l = data[i] & REG_TEMP_ADC_OVER_THRSH_L_THRESH_MASK;
            state->temp_threshold_h |= state->temp_adc_over_thrsh_l >> 2;
            break;
        case REG_TEMP_ADC_UNDER_THRSH_H:
            state->temp_adc_under_thrsh_h = data[i] & REG_TEMP_ADC_UNDER_THRSH_H_THRESH_MASK;
            state->temp_threshold_l = state->temp_adc_under_thrsh_h << 6;
            break;
        case REG_TEMP_ADC_UNDER_THRSH_L:
            state->temp_adc_under_thrsh_l = data[i] & REG_TEMP_ADC_UNDER_THRSH_L_THRESH_MASK;
            state->temp_threshold_l |= state->temp_adc_under_thrsh_l >> 2;
            break;
        case REG_TEMP_ADC_TIMER:
            state->temp_adc_timer = data[i] & (REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_MASK | REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_ACT_MASK);
            state->time_temp_inact = (state->temp_adc_timer & REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_MASK) >> REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_SHIFT;
            state->time_temp_act = state->temp_adc_timer & REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_ACT_MASK;

            if(state->temp_ctl & REG_TEMP_CTL_TEMP_ACT_EN){
                timer_evt_set_oneshot(&state->tmevt[4], (unsigned)(((float)state->time_temp_act / state->odr) * 1000000000));
            }
            if(state->temp_ctl & REG_TEMP_CTL_TEMP_INACT_EN){
                timer_evt_set_oneshot(&state->tmevt[5], (unsigned)(((float)state->time_temp_inact / state->odr) * 1000000000));
            }
            break;
        case REG_AXIS_MASK:
            state->axis_mask = data[i] & (REG_AXIS_MASK_TAP_AXIS_MASK | REG_AXIS_MASK_ACT_INACT_Z | REG_AXIS_MASK_ACT_INACT_Y | REG_AXIS_MASK_ACT_INACT_X);
            break;
        case REG_PEDOMETER_CTL:
            state->pedometer_ctl = data[i] & (REG_PEDOMETER_CTL_RESET_STEP | REG_PEDOMETER_CTL_RESET_OF | REG_PEDOMETER_CTL_EN);
            break;
        case REG_PEDOMETER_THRES_H:
            state->pedometer_thres_h = data[i] & REG_PEDOMETER_THRES_H_THRESHOLD_MASK;
            break;
        case REG_PEDOMETER_THRES_L:
            state->pedometer_thres_l = data[i] & REG_PEDOMETER_THRES_L_THRESHOLD_MASK;
            break;
        case REG_PEDOMETER_SENS_H:
            state->pedometer_sens_h = data[i] & REG_PEDOMETER_SENS_H_SENSITIVITY_MASK;
            break;
        case REG_PEDOMETER_SENS_L:
            state->pedometer_sens_l = data[i] & REG_PEDOMETER_SENS_L_SENSITIVITY_MASK;
            break;
        default:
            break;
        }

        if(state->count){
            state->count++;
            state->addr++;
        }

    }

    if((state->power_ctl & REG_POWER_CTL_MEASURE_MASK) == 2){
        state->tme_init = 1;
        timer_evt_start(state);
    }else{
        state->tme_init = 0;
    }
    pthread_mutex_unlock(&state->lock);
    return i;
}

static int adi_adxl36x_read(void *priv, unsigned len, uint8_t *data)
{
    adi_adxl36x_state_t *state = priv;
    unsigned i;

    pthread_mutex_lock(&state->lock);
    for(i=0; i<len; i++){

        switch(state->addr) {
        case REG_DEVID_AD:
            data[i] = state->devid_ad;
            break;
        case REG_DEVID_MST:
            data[i] = state->devid_mst;
            break;
        case REG_PART_ID:
            data[i] = state->part_id;
            break;
        case REG_REV_ID:
            data[i] = state->rev_id;
            break;
        case REG_SERIAL_NUMBER_2:
            data[i] = state->serial_number_2;
            break;
        case REG_SERIAL_NUMBER_1:
            data[i] = state->serial_number_1;
            break;
        case REG_SERIAL_NUMBER_0:
            data[i] = state->serial_number_0;
            break;
        case REG_XDATA:
            data[i] = state->xdata_h;
            if(state->self_test & 1){
                data[i] = (self_test_value[(state->filter_ctl & REG_FILTER_CTL_RANGE_MASK) >> REG_FILTER_CTL_RANGE_SHIFT][state->stc] & 0xff00) >> 8;
                state->stc++;
                if(state->stc > 1){
                    state->stc = 0;
                }
            }
            break;
        case REG_YDATA:
            data[i] = state->ydata_h;
            break;
        case REG_ZDATA:
            data[i] = state->zdata_h;
            break;
        case REG_STATUS:
            data[i] = state->status[0];
            switch(state->activity_mode){
            case 0:
            case 2:
                //default
                if(state->status[0] & REG_STATUS_ACT){
                    state->status[0] &= ~REG_STATUS_ACT;
                    if(state->act_inact_ctl & REG_ACT_INACT_CTL_INTACT_EN_MASK){
                        timer_evt_set_oneshot(&state->tmevt[2], (unsigned)(((float)state->time_inact / state->odr) * 1000000000));
                    }
                }
                if(state->status[0] & REG_STATUS_INACT){
                    state->status[0] &= ~REG_STATUS_INACT;
                    if(state->act_inact_ctl & REG_ACT_INACT_CTL_ACT_EN_MASK){
                        timer_evt_set_oneshot(&state->tmevt[1], (unsigned)(((float)state->time_act / state->odr) * 1000000000));
                    }
                }
                break;
            case 1:
                // link
                // set after interrupt clear
                if(state->status[0] & REG_STATUS_ACT){
                    state->status[0] &= ~REG_STATUS_ACT;
                    state->status[0] |= REG_STATUS_AWAKE;
                    if(state->act_inact_ctl & REG_ACT_INACT_CTL_INTACT_EN_MASK){
                        timer_evt_set_oneshot(&state->tmevt[2], (unsigned)(((float)state->time_inact / state->odr) * 1000000000));
                    }
                }else if(state->status[0] & REG_STATUS_INACT){
                    state->status[0] &= ~REG_STATUS_INACT;
                    state->status[0] &= ~REG_STATUS_AWAKE;
                    if(state->act_inact_ctl & REG_ACT_INACT_CTL_ACT_EN_MASK){
                        timer_evt_set_oneshot(&state->tmevt[1], (unsigned)(((float)state->time_act / state->odr) * 1000000000));
                    }
                }
                break;
            case 3:
                //loop
                state->status[0] &= ~(REG_STATUS_ACT | REG_STATUS_INACT);
                break;
            }
            state->status[0] &= ~(REG_STATUS_DATA_READY | REG_STATUS_FIFO_WATER_MARK | REG_STATUS_FIFO_OVER_RUN | REG_STATUS_FIFO_READY | REG_STATUS_ERR_USER_REGS);
            break;
        case REG_FIFO_ENTRIES_L:
            data[i] = state->fifo_entries_l;
            break;
        case REG_FIFO_ENTRIES_H:
            data[i] = state->fifo_entries_h;
            break;
        case REG_XDATA_H:
            data[i] = state->xdata_h;
            if(state->self_test & 1){
                data[i] = (self_test_value[(state->filter_ctl & REG_FILTER_CTL_RANGE_MASK) >> REG_FILTER_CTL_RANGE_SHIFT][state->stc] & 0xff00) >> 8;
            }
            break;
        case REG_XDATA_L:
            data[i] = state->xdata_l;
            if(state->self_test & 1){
                data[i] = self_test_value[(state->filter_ctl & REG_FILTER_CTL_RANGE_MASK) >> REG_FILTER_CTL_RANGE_SHIFT][state->stc] & 0xff;
                state->stc++;
                if(state->stc > 1){
                    state->stc = 0;
                }
            }
            break;
        case REG_YDATA_H:
            data[i] = state->ydata_h;
            break;
        case REG_YDATA_L:
            data[i] = state->ydata_l;
            break;
        case REG_ZDATA_H:
            data[i] = state->zdata_h;
            break;
        case REG_ZDATA_L:
            data[i] = state->zdata_l;
            break;
        case REG_TEMP_H:
            data[i] = state->temp_h;
            break;
        case REG_TEMP_L:
            data[i] = state->temp_l;
            break;
        case REG_EX_ADC_H:
            data[i] = state->ex_adc_h;
            break;
        case REG_EX_ADC_L:
            data[i] = state->ex_adc_l;
            break;
        case REG_I2C_FIFO_DATA:
            if(state->fifo_count == 0){
                break;
            }
            switch((state->adc_ctl & REG_ADC_CTL_FIFO_8_12BIT_MASK) >> REG_ADC_CTL_FIFO_8_12BIT_SHIFT){
            case 1:
            //8 bit
                data[i] = state->sample_fifo[state->sample_fifo_rpt%FIFO_SIZE];
                state->sample_fifo_rpt++;
                state->fifo_count--;
                break;
            case 2:
            //12 bit
                switch(state->read_count%3){
                case 0:
                    data[i] = state->sample_fifo[state->sample_fifo_rpt%FIFO_SIZE] >> 4;
                    break;
                case 1:
                    data[i] = (state->sample_fifo[state->sample_fifo_rpt%FIFO_SIZE] & 0xf) << 4;
                    state->sample_fifo_rpt++;
                    state->fifo_count--;
                    data[i] |= (state->sample_fifo[state->sample_fifo_rpt%FIFO_SIZE] & 0xf00) >> 8;
                    break;
                case 2:
                    data[i] = state->sample_fifo[state->sample_fifo_rpt%FIFO_SIZE] & 0xff;
                    state->sample_fifo_rpt++;
                    state->fifo_count--;
                    break;
                }
                break;
            case 0:
            case 3:
            //16 bit
                switch(state->read_count%2){
                case 0:
                    data[i] = state->sample_fifo[state->sample_fifo_rpt%FIFO_SIZE] >> 8;
                    break;
                case 1:
                    data[i] = state->sample_fifo[state->sample_fifo_rpt%FIFO_SIZE] & 0xff;
                    state->sample_fifo_rpt++;
                    state->fifo_count--;
                    break;
                break;
                }
            }
            if(!(state->fifo_count%state->sample_size)){
                state->sample_count--;
            }
            state->read_count++;
            break;
        case REG_SOFT_RESET:
            data[i] = state->soft_reset;
            break;
        case REG_THRESH_ACT_H:
            data[i] = state->thresh_act_h;
            break;
        case REG_THRESH_ACT_L:
            data[i] = state->thresh_act_l;
            break;
        case REG_TIME_ACT:
            data[i] = state->time_act;
            break;
        case REG_THRESH_INACT_H:
            data[i] = state->thresh_inact_h;
            break;
        case REG_THRESH_INACT_L:
            data[i] = state->thresh_inact_l;
            break;
        case REG_TIME_INACT_H:
            data[i] = state->time_inact_h;
            break;
        case REG_TIME_INACT_L:
            data[i] = state->time_inact_l;
            break;
        case REG_ACT_INACT_CTL:
            data[i] = state->act_inact_ctl;
            break;
        case REG_FIFO_CONTROL:
            data[i] = state->fifo_control;
            break;
        case REG_FIFO_SAMPLES:
            data[i] = state->fifo_samples;
            break;
        case REG_INTMAP1_LOWER:
            data[i] = state->intmapn_lower[0];
            break;
        case REG_INTMAP2_LOWER:
            data[i] = state->intmapn_lower[1];
            break;
        case REG_FILTER_CTL:
            data[i] = state->filter_ctl;
            break;
        case REG_POWER_CTL:
            data[i] = state->power_ctl;
            break;
        case REG_SELF_TEST:
            data[i] = state->self_test;
            break;
        case REG_TAP_THRESH:
            data[i] = state->tap_thresh;
            break;
        case REG_TAP_DUR:
            data[i] = state->tap_dur;
            break;
        case REG_TAP_LATENT:
            data[i] = state->tap_latent;
            break;
        case REG_TAP_WINDOW:
            data[i] = state->tap_window;
            break;
        case REG_X_OFFSET:
            data[i] = state->x_offset;
            break;
        case REG_Y_OFFSET:
            data[i] = state->y_offset;
            break;
        case REG_Z_OFFSET:
            data[i] = state->z_offset;
            break;
        case REG_X_SENS:
            data[i] = state->x_sens;
            break;
        case REG_Y_SENS:
            data[i] = state->y_sens;
            break;
        case REG_Z_SENS:
            data[i] = state->z_sens;
            break;
        case REG_TIMER_CTL:
            data[i] = state->timer_ctl;
            break;
        case REG_INTMAP1_UPPER:
            data[i] = state->intmapn_upper[0];
            break;
        case REG_INTMAPS2_UPPER:
            data[i] = state->intmapn_upper[1];
            break;
        case REG_ADC_CTL:
            data[i] = state->adc_ctl;
            break;
        case REG_TEMP_CTL:
            data[i] = state->temp_ctl;
            break;
        case REG_TEMP_ADC_OVER_THRSH_H:
            data[i] = state->temp_adc_over_thrsh_h;
            break;
        case REG_TEMP_ADC_OVER_THRSH_L:
            data[i] = state->temp_adc_over_thrsh_l;
            break;
        case REG_TEMP_ADC_UNDER_THRSH_H:
            data[i] = state->temp_adc_under_thrsh_h;
            break;
        case REG_TEMP_ADC_UNDER_THRSH_L:
            data[i] = state->temp_adc_under_thrsh_l;
            break;
        case REG_TEMP_ADC_TIMER:
            data[i] = state->temp_adc_timer;
            break;
        case REG_AXIS_MASK:
            data[i] = state->axis_mask;
            break;
        case REG_STATUS_COPY:
            data[i] = state->status[0];
            break;
        case REG_STATUS_2:
            data[i] = state->status[1];

            if((state->temp_ctl & REG_TEMP_CTL_TEMP_ACT_EN) && (state->status[1] & REG_STATUS_2_TEMP_ADC_HI)){
                timer_evt_set_oneshot(&state->tmevt[4], (unsigned)(((float)state->time_temp_act / state->odr) * 1000000000));
                state->status[1] &= ~REG_STATUS_2_TEMP_ADC_HI;
            }
            if((state->temp_ctl & REG_TEMP_CTL_TEMP_INACT_EN) && (state->status[1] & REG_STATUS_2_TEMP_ADC_LOW)){
                timer_evt_set_oneshot(&state->tmevt[5], (unsigned)(((float)state->time_temp_inact / state->odr) * 1000000000));
                state->status[1] &= ~REG_STATUS_2_TEMP_ADC_LOW;
            }
            state->status[1] &= ~(REG_STATUS_2_TAP_TWO | REG_STATUS_2_TAP_ONE);
            break;
        case REG_STATUS_3:
            data[i] = state->status[2];
            break;
        case REG_PEDOMETER_STEP_CNT_H:
            data[i] = state->pedometer_step_cnt_h;
            break;
        case REG_PEDOMETER_STEP_CNT_L:
            data[i] = state->pedometer_step_cnt_l;
            break;
        case REG_PEDOMETER_CTL:
            data[i] = state->pedometer_ctl;
            break;
        case REG_PEDOMETER_THRES_H:
            data[i] = state->pedometer_thres_h;
            break;
        case REG_PEDOMETER_THRES_L:
            data[i] = state->pedometer_thres_l;
            break;
        case REG_PEDOMETER_SENS_H:
            data[i] = state->pedometer_sens_h;
            break;
        case REG_PEDOMETER_SENS_L:
            data[i] = state->pedometer_sens_l;
            break;
        }

        adi_adxl36x_mmio_debug(0, state->addr, len, data[i], "r");

        if(state->count && (state->addr != REG_I2C_FIFO_DATA)){
            state->count++;
            state->addr++;
            if(state->addr == REG_I2C_FIFO_DATA){
                state->addr = REG_SOFT_RESET;
            }
        }
    }
    pthread_mutex_unlock(&state->lock);
    return i;
}

static void adi_adxl36x_comp_cb(void *handle, unsigned icomp)
{
    adi_adxl36x_state_t *state = handle;
    uint64_t check = 0;

#if ADI_ADXL36x_DEBUG > 1
    printf("[adi-adxl36x] comp %d\n", icomp);
#endif

    timer_evt_stop(state);
    state->tme_cnt = state->timer_count;

    switch(icomp){
    case 0:
        //Continuous sample
        if((state->power_ctl & REG_POWER_CTL_MEASURE_MASK) == 2){

            adi_adxl36x_accel_convert(state);

            state->status[0] |= REG_STATUS_DATA_READY;

            if(state->tap_thresh && !(state->tap_sample & 1)){

                if(((state->axis_mask & REG_AXIS_MASK_TAP_AXIS_MASK) >> REG_AXIS_MASK_TAP_AXIS_SHIFT) < 3){
                    check = state->accel[(state->axis_mask & REG_AXIS_MASK_TAP_AXIS_MASK) >> REG_AXIS_MASK_TAP_AXIS_SHIFT];
                }

                switch(state->tap_state){
                case 0:
                    if(abs(check) > state->tap_thresh){
                        state->tap_sample = 1;
                        timer_evt_set_oneshot(&state->tmevt[3], (unsigned)(((float)state->tap_dur / state->odr) * 1000000000));
                    }
                    break;
                case 2:
                    if(state->tme_cnt < (state->tap_time + state->tap_window)){
                        if(abs(check) > state->tap_thresh){
                            state->tap_sample = 1;
                            timer_evt_set_oneshot(&state->tmevt[3], (unsigned)(((float)state->tap_dur / state->odr) * 1000000000));
                        }
                    }else{
                        state->tap_time = 0;
                        state->tap_state = 0;
                    }
                    break;
                }
            }
        }

        if(state->temp_ctl & REG_TEMP_CTL_TEMP_EN){
            adi_adxl36x_temp_convert(state);
            state->status[0] |= REG_STATUS_DATA_READY;
        }

        if(state->fifo_control & REG_FIFO_CONTROL_FIFO_MODE_MASK){
            adi_adxl36x_fifo_sample(state);
        }
        break;
    case 1:
        //act
        for(int i = 0; i < 3; i++){
            if(state->axis_mask & (1 << i)){
                continue;
            }
            switch(state->act_inact_ctl & REG_ACT_INACT_CTL_ACT_EN_MASK){
            case 1:
                check = abs(state->accel[i] >> 2);
                break;
            case 3:
                //ref
                check = abs((state->accel[i] >> 2) - state->act_ref[i]);
                break;
            }
            if(check > state->act_threshold){
                state->status[0] |= (REG_STATUS_ACT | REG_STATUS_AWAKE);
                switch(state->activity_mode){
                case 0:
                case 2:
                    //default
                    //set again once status cleared
                    break;
                case 1:
                    //link
                    break;
                case 3:
                    //loop
                    timer_evt_set_oneshot(&state->tmevt[2], (unsigned)(((float)state->time_inact / state->odr) * 1000000000));
                    break;
                }
            }else{
                state->act_ref[i] = state->accel[i] >> 2;
            }
        }

        break;
    case 2:
        //inact
        for(int i = 0; i < 3; i++){
            if(state->axis_mask & (1 << i)){
                continue;
            }
            switch((state->act_inact_ctl & REG_ACT_INACT_CTL_INTACT_EN_MASK) >> REG_ACT_INACT_CTL_INTACT_EN_SHIFT){
            case 1:
                check = abs(state->accel[i] >> 2);
                break;
            case 3:
                //ref
                check = abs((state->accel[i] >> 2) - state->inact_ref[i]);
                break;
            }
            if(check < state->inact_threshold){
                state->status[0] |= REG_STATUS_INACT;
                switch(state->activity_mode){
                case 0:
                case 2:
                    //default
                    //set again once status cleared
                    break;
                case 1:
                    //link
                    break;
                case 3:
                    //loop
                    timer_evt_set_oneshot(&state->tmevt[1], (unsigned)(((float)state->time_act / state->odr) * 1000000000));
                    state->status[0] &= ~REG_STATUS_AWAKE;
                    break;
                }
            }else{
                state->inact_ref[i] = state->accel[i] >> 2;
            }
        }

        break;
    case 3:
        //tap
        if(((state->axis_mask & REG_AXIS_MASK_TAP_AXIS_MASK) >> REG_AXIS_MASK_TAP_AXIS_SHIFT) < 3){
            check = state->accel[(state->axis_mask & REG_AXIS_MASK_TAP_AXIS_MASK) >> REG_AXIS_MASK_TAP_AXIS_SHIFT];
        }

        switch(state->tap_state){
        case 0: //tap 1 check
            if(abs(check) < state->tap_thresh){
                state->status[1] |= REG_STATUS_2_TAP_ONE;
                state->tap_time = state->tme_cnt + state->tap_latent;
                state->tap_state = 1;
                timer_evt_set_oneshot(&state->tmevt[3], (unsigned)(((float)state->tap_latent / state->odr) * 1000000000));
            }else{
                state->tap_sample = 0;
            }
            break;
        case 1: //tap latency
            state->tap_sample = 0;
            state->tap_state = 2;
            break;
        case 2: //tap 2 check
            if(abs(check) < state->tap_thresh){
                state->status[1] |= REG_STATUS_2_TAP_TWO;

            }
            state->tap_time = 0;
            state->tap_state = 0;
            state->tap_sample = 0;
            break;
        }
        break;
    case 4:
        //temp act
        if(state->temp >= state->temp_threshold_h){
            state->status[1] |= REG_STATUS_2_TEMP_ADC_HI;
        }
        break;
    case 5:
        //temp inact
        if(state->temp <= state->temp_threshold_l){
            state->status[1] |= REG_STATUS_2_TEMP_ADC_LOW;
        }
        break;
    case 6://wake/timeout timer
        break;
    }

    adi_adxl36x_irq_update(state);
    timer_evt_start(state);

 }

void (*comp_cb[MAX_TIMERS])(void *priv, unsigned idx) = { adi_adxl36x_comp_cb, adi_adxl36x_comp_cb, adi_adxl36x_comp_cb, adi_adxl36x_comp_cb,
                                                          adi_adxl36x_comp_cb, adi_adxl36x_comp_cb, adi_adxl36x_comp_cb, adi_adxl36x_comp_cb};

void timer_evt_init(void *priv, struct epoll_event *eevt, struct epoll_event *events)
{
    adi_adxl36x_state_t *state = priv;

    if(!eevt || !events || !state->epfd){
        return;
    }

    for(int i = 0; i < MAX_TIMERS; i++){
        state->tmevt[i].state = state;
        eevt[i].events = EPOLLIN;
        eevt[i].data.ptr = &state->tmevt[i];
        state->tmevt[i].tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        state->tmevt[i].eevt = &eevt[i];
        state->tmevt[i].comp_cb = comp_cb[i];
        state->tmevt[i].itime.it_value.tv_sec = 0;
        state->tmevt[i].itime.it_value.tv_nsec = 0;
        state->tmevt[i].itime.it_interval.tv_sec = 0;
        state->tmevt[i].itime.it_interval.tv_nsec = 0;
        events[i].data.ptr = NULL;
        epoll_ctl(state->epfd, EPOLL_CTL_ADD, state->tmevt[i].tfd, state->tmevt[i].eevt);
    }

}

void timer_evt_reset(void *priv)
{
    adi_adxl36x_state_t *state = priv;

    for(int i = 0; i < MAX_TIMERS; i++){
        epoll_ctl(state->epfd,  EPOLL_CTL_DEL, state->tmevt[i].tfd, state->tmevt[i].eevt);
        state->tmevt[i].itime.it_value.tv_sec = 0;
        state->tmevt[i].itime.it_value.tv_nsec = 0;
        state->tmevt[i].itime.it_interval.tv_sec = 0;
        state->tmevt[i].itime.it_interval.tv_nsec = 0;
        timerfd_settime(state->tmevt[i].tfd, 0, &state->tmevt[i].itime, NULL);
        epoll_ctl(state->epfd, EPOLL_CTL_ADD, state->tmevt[i].tfd, state->tmevt[i].eevt);
    }
}

void timer_evt_free(void *priv)
{
    adi_adxl36x_state_t *state = priv;

    for(int i = 0; i < MAX_TIMERS; i++){
        epoll_ctl(state->epfd,  EPOLL_CTL_DEL, state->tmevt[i].tfd, state->tmevt[i].eevt);
        state->tmevt[i].itime.it_value.tv_sec = 0;
        state->tmevt[i].itime.it_value.tv_nsec = 0;
        state->tmevt[i].itime.it_interval.tv_sec = 0;
        state->tmevt[i].itime.it_interval.tv_nsec = 0;
        timerfd_settime(state->tmevt[i].tfd, 0, &state->tmevt[i].itime, NULL);
        close(state->tmevt[i].tfd);
    }
}

void timer_evt_start(void *priv)
{
    adi_adxl36x_state_t *state = priv;

    epoll_ctl(state->epfd, EPOLL_CTL_ADD, state->tmevt[0].tfd, state->tmevt[0].eevt);
    timerfd_settime(state->tmevt[0].tfd, 0, &state->tmevt[0].itime, NULL);

}

void timer_evt_stop(void *priv)
{
    adi_adxl36x_state_t *state = priv;
    struct itimerspec itime;

    itime.it_value.tv_sec = 0;
    itime.it_value.tv_nsec = 0;
    itime.it_interval.tv_sec = 0;
    itime.it_interval.tv_nsec = 0;

    epoll_ctl(state->epfd, EPOLL_CTL_DEL, state->tmevt[0].tfd, state->tmevt[0].eevt);
    timerfd_settime(state->tmevt[0].tfd, 0, &itime, NULL);//set timer off and clears timerfd events

}

void timer_evt_set_frequency(adi_adxl36x_state_t *state, float hz)
{
    unsigned nsec = 0;
    float period = 0;

    period = 1.0/hz;
    nsec =   period * 1000000000;

    printf("hertz %f period %f nsec %d \n", hz, period, nsec);

    epoll_ctl(state->epfd,  EPOLL_CTL_DEL, state->tmevt[0].tfd, state->tmevt[0].eevt); //remove timerfd events from epool
    state->tmevt[0].itime.it_value.tv_sec = 0;
    state->tmevt[0].itime.it_value.tv_nsec = 0;
    state->tmevt[0].itime.it_interval.tv_sec = 0;
    state->tmevt[0].itime.it_interval.tv_nsec = 0;
    timerfd_settime(state->tmevt[0].tfd, 0, &state->tmevt[0].itime, NULL);//set timer off and clears timerfd events
    epoll_ctl(state->epfd, EPOLL_CTL_ADD, state->tmevt[0].tfd, state->tmevt[0].eevt); //add the timer

    state->tmevt[0].itime.it_value.tv_nsec = nsec;
    state->tmevt[0].itime.it_interval.tv_nsec = nsec;

}

void timer_evt_set_oneshot(struct timer_evt *tmevt, unsigned nsec)// timer_evt 1-7
{
    adi_adxl36x_state_t *state = tmevt->state;
    unsigned idx = tmevt - state->tmevt;

    if(!idx){
        return;
    }

    epoll_ctl(state->epfd,  EPOLL_CTL_DEL, tmevt->tfd, tmevt->eevt);
    tmevt->itime.it_value.tv_sec = 0;
    tmevt->itime.it_value.tv_nsec = 0;
    tmevt->itime.it_interval.tv_sec = 0;
    tmevt->itime.it_interval.tv_nsec = 0;
    timerfd_settime(tmevt->tfd, 0, &tmevt->itime, NULL);
    epoll_ctl(state->epfd, EPOLL_CTL_ADD, tmevt->tfd, tmevt->eevt);
    tmevt->itime.it_interval.tv_nsec = nsec;
    timerfd_settime(tmevt->tfd, 0, &tmevt->itime, NULL);

}

void process_cb(void *priv)
{
    struct timer_evt *timer_evt = (struct timer_evt *) priv;
    adi_adxl36x_state_t *state = (adi_adxl36x_state_t *) timer_evt->state;
    unsigned idx = timer_evt - state->tmevt;
    uint64_t exp;

    if(idx == 0){
        state->timer_count++;
    }

    if(state->tmevt[idx].comp_cb){
        state->tmevt[idx].comp_cb(state, idx);
    }

    read(timer_evt->tfd, &exp, sizeof(uint64_t)); // Consume timer event

}

void *process_timer_evt(void *priv)
{
    adi_adxl36x_state_t *state = priv;

    while (state->run) {
        int nfds = epoll_wait(state->epfd, state->eevto, 1, 300);

        pthread_mutex_lock(&state->lock);

        if(!state->run){
            pthread_mutex_unlock(&state->lock);
            return NULL;
        }

        for (int n = 0; n < nfds; ++n) {
            if (state->eevto[n].data.ptr) {
                process_cb(state->eevto[n].data.ptr);
            }
        }

        pthread_mutex_unlock(&state->lock);

    }

    return NULL;
}

void *process_cm(void *priv)
{
    adi_adxl36x_state_t *state = priv;

    int ret = 0;

    while (state->run) {
        if(state->cycle_time == -1){
            ret = coremodel_mainloop(state->cm, 1000000);
        }else{
            ret = coremodel_mainloop(state->cm, state->cycle_time);
        }

        if(ret < 0){
            pthread_mutex_lock(&state->lock);
            state->run = 0;
            pthread_cond_signal(&state->cm_cond);
            pthread_mutex_unlock(&state->lock);
        }
    }

    return NULL;
}

static const coremodel_i2c_func_t adi_adxl36x_func = {
    .start = adi_adxl36x_start,
    .write = adi_adxl36x_write,
    .read = adi_adxl36x_read,
    .stop = adi_adxl36x_stop };

int main(int argc, char *argv[])
{
    adi_adxl36x_state_t *state = calloc(1, sizeof(adi_adxl36x_state_t));
    struct epoll_event eevt[8], events[8];
    unsigned ret = 0, res = 0;
    int gpios[PIN_NUM] = { -1, -1 };

    if(!state){
        return 0;
    }

    if(argc != 3) {
        printf("usage: coremodel-i2c <address[:port]> <i2c>\n");
        free(state);
        return 1;
    }

    res = coremodel_connect(&state->cm, argv[1]);
    if(res) {
        free(state);
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    state->cycle_time = 1000000;

    state->epfd = epoll_create1(0);

    pthread_mutex_init(&state->lock, NULL);
    pthread_cond_init(&state->cm_cond, NULL);

    timer_evt_init(state, eevt, events);

    state->eevto = events;

    adi_adxl36x_reset(state);

    state->handle = coremodel_attach_i2c(state->cm, argv[2], 0x53, &adi_adxl36x_func, state, COREMODEL_I2C_WRITE_ACK);
    if(!state->handle) {
        fprintf(stderr, "error: failed to attach i2c.\n");
        timer_evt_free(state);
        pthread_mutex_destroy(&state->lock);
        coremodel_disconnect(state->cm);
        free(state);
        return 1;
    }

    for(int i = 0; i < PIN_NUM; i++) {
        if(gpios[i] < 0){
            continue;
        }

        state->pin[i].state = state;
        state->pin[i].pin_handle = coremodel_attach_gpio(state->cm, "gpio", gpios[i], &gpio_func, &state->pin[i]);

        if(!state->pin[i].pin_handle) {
            fprintf(stderr, "error: failed to attach gpio %d.\n", gpios[i]);
            timer_evt_free(state);
            pthread_mutex_destroy(&state->lock);
            coremodel_disconnect(state->cm);
            free(state);
            return 1;
        }
    }

    state->run = 1;

    ret = pthread_create(&state->timer_thread, NULL, &process_timer_evt, (void *)state);
    if(ret){
        state->run = 0;
    }

    ret = pthread_create(&state->cm_thread, NULL, &process_cm, (void *)state);
    if(ret){
        state->run = 0;
    }

    pthread_mutex_lock(&state->lock);
    while (state->run) {
        pthread_cond_wait(&state->cm_cond, &state->lock);
    }
    pthread_mutex_unlock(&state->lock);

    if(state->timer_thread){
        ret = pthread_join(state->timer_thread, NULL);
        if(ret){
            printf("failed to join thread %ld error %d", state->timer_thread, ret);
        }
    }

    if(state->cm_thread){
        ret = pthread_join(state->cm_thread, NULL);
        if(ret){
            printf("failed to join thread %ld error %d", state->cm_thread, ret);
        }
    }

    timer_evt_free(state);

    pthread_mutex_destroy(&state->lock);
    pthread_cond_destroy(&state->cm_cond);

    for(int i = 0; i < PIN_NUM; i++) {
        if(gpios[i] < 0){
            continue;
        }
        if(state->pin[i].pin_handle){
            coremodel_gpio_set(state->pin[i].pin_handle, 1, 0);
            coremodel_mainloop(state->cm, 100000);
            coremodel_gpio_set(state->pin[i].pin_handle, 0, 0);
            coremodel_mainloop(state->cm, 100000);
            coremodel_detach(state->pin[i].pin_handle);
        }
    }

    coremodel_detach(state->handle);
    coremodel_disconnect(state->cm);

    free(state);

    printf("exit \n");

    return 0;
}
