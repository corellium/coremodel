/*
 * CoreModel SPI Mux Example
 *
 * Copyright (c) 2026 Corellium Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#include <coremodel.h>
#include "coremodel-spi-mux.h"

#define CHANNEL_LIMIT 8
#define PIN_NUM       4

typedef struct spi_mux_state{

    void *cm;
    //void *handle;
    char spibusname[32];
    char gpiobusname[32];
    int cs;

    struct coremodel_gpio_pin {
        struct spi_mux_state *state;
        void *pin_handle; //coremodel_if
        int mvolt;
    } pin[PIN_NUM]; // bit 0-2 and enable

    struct spi_channel{
            char name[32];
            void *dev_state;
            const coremodel_spi_func_t *spi_func;
            void *handle;
            void *mux_state;
            void *channel;
    }channel[CHANNEL_LIMIT];

    uint8_t count, chan, chanp, update_pin, en;

} spi_mux_state_t;

static void spi_mux_gpio_notify(void *priv, int mvolt)
{

    struct coremodel_gpio_pin *pin = priv;
    spi_mux_state_t *state = pin->state;
    unsigned pin_idx = pin - state->pin;

    printf("gpio notifiy idx %d\n", pin_idx);

    state->chanp = state->chan;
    if(pin_idx < 3){
        pin->mvolt = mvolt;
        state->chan &= ~(1 << pin_idx);
        state->chan |= !!state->pin[pin_idx].mvolt << pin_idx;
    }else if(pin_idx == 3){
        pin->mvolt = mvolt;
        state->en = !!pin->mvolt;
    }

    printf("chanp %d chan %d \n", state->chanp, state->chan);
    state->update_pin = 1;
}

int spi_mux_connect(void *priv, unsigned channel, const coremodel_spi_func_t *dev_func, void *dev_priv, const char *name)
{
    spi_mux_state_t *state = priv;

    if(channel >= CHANNEL_LIMIT){
        return 1;
    }

    sprintf(state->channel[channel].name, "%s", name);
    state->channel[channel].dev_state = dev_priv;
    state->channel[channel].spi_func = dev_func;
    state->channel[channel].mux_state = state;
    state->channel[channel].channel = &state->channel[channel];

    return 0;
}

static const coremodel_spi_func_t test_spi_func = {
    .cs = test_spi_cs,
    .xfr = test_spi_xfr };

static const coremodel_gpio_func_t spi_mux_gpio_func = {
    .notify = spi_mux_gpio_notify };

int main(int argc, char *argv[])
{
    int res;
    char name[16];
    int gpios[PIN_NUM] = { -1, -1, -1, -1 };

    spi_mux_state_t *state = calloc(1, sizeof(spi_mux_state_t));

    if(!state){
        return 0;
    }

    if(argc != 9) {
        printf("usage: coremodel-spi-mux <address[:port]> <spi> <cs> <gpio> [gpio A0...A2] [gpio EN]\n");
        free(state);
        return 1;
    }

    res = coremodel_connect(&state->cm, argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        free(state);
        return 1;
    }


    sprintf(state->spibusname, "%s", argv[2]);
    state->cs = atoi(argv[3]);
    sprintf(state->gpiobusname, "%s", argv[4]);

    for(int i = 0; i < PIN_NUM; i++){
        gpios[i] = strtoul(argv[5 + i], NULL, 0);
        printf("%d ", gpios[i]);
    }
    printf("\n");

    for(int i = 0; i < PIN_NUM; i++) {
        if(gpios[i] < 0){
            continue;
        }

        state->pin[i].state = state;
        state->pin[i].pin_handle = coremodel_attach_gpio(state->cm, state->gpiobusname, gpios[i], &spi_mux_gpio_func, &state->pin[i]);

        if(!state->pin[i].pin_handle) {
            fprintf(stderr, "error: failed to attach gpio %d.\n", gpios[i]);
            coremodel_disconnect(state->cm);
            free(state);
            return 1;
        }
    }

    for(int i = 0; i < CHANNEL_LIMIT; i++){
        sprintf(name, "spi-dummy:%d", i);
        res = spi_mux_connect(state, i, &test_spi_func, NULL, name);

    }

    state->chanp = 0;
    state->chan = 0;
    state->update_pin = 0;

    while(1){
        res = coremodel_mainloop(state->cm, 1000000);
        if(res < 0){
            break;
        }
        if(state->update_pin){
            state->update_pin = 0;
            if(state->channel[state->chanp].handle){
                printf("detach channel %d\n", state->chanp);
                coremodel_detach(state->channel[state->chanp].handle);
                state->channel[state->chanp].handle = NULL;
            }

            coremodel_mainloop(state->cm, 1000000);

            if(state->en){
                state->channel[state->chan].handle = coremodel_attach_spi(state->cm, state->spibusname, state->cs, state->channel[state->chan].spi_func, NULL, COREMODEL_SPI_BLOCK);
                if(state->channel[state->chan].handle){
                    printf("attached channel %d\n", state->chan);
                }else{
                    printf("failed to attached %d\n", state->chan);
                }
            }

            coremodel_mainloop(state->cm, 1000000);
        }
    }

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

    coremodel_disconnect(state->cm);
    free(state);

    return 0;
}
