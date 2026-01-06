#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>


#include "coremodel-i2c-mux.h"
#include <coremodel.h>


#define I2C_MUX_DEBUG 0
#define CHANNEL_LIMIT 8

typedef struct i2c_mux_state{

    void *cm;
    void *handle;
    char busname[32];

    struct i2c_channel{
        struct i2c_coremodel_device{
            char name[32];
            uint8_t addr;
            void *dev_state;
            const coremodel_i2c_func_t *i2c_func;
            void *handle;
            void *mux_state;
            void *channel;
        }device[128];
        uint32_t count;
    }channel[CHANNEL_LIMIT];

    uint8_t count, addr, chan, chanp, write, stop;

} i2c_mux_state_t;

int i2c_mux_connect(void *priv, unsigned channel, unsigned addr, const coremodel_i2c_func_t *dev_func, void *dev_priv, const char *name)
{
    i2c_mux_state_t *state = priv;

    printf("i2c_mux_connect %s addr %x channel %d \n", name, addr, channel);

    if((addr >= 128) || (channel >= CHANNEL_LIMIT) || (state->channel[channel].count >= 128)){
        return 1;
    }

    sprintf(state->channel[channel].device[state->channel[channel].count].name, "%s", name);
    state->channel[channel].device[state->channel[channel].count].addr = addr;
    state->channel[channel].device[state->channel[channel].count].dev_state = dev_priv;
    state->channel[channel].device[state->channel[channel].count].i2c_func = dev_func;
    state->channel[channel].device[state->channel[channel].count].mux_state = state;
    state->channel[channel].device[state->channel[channel].count].channel = &state->channel[channel];
    state->channel[channel].count++;

    return 0;
}

static void i2c_mux_update_channels(i2c_mux_state_t *state)
{
    uint8_t changed = state->chanp ^ state->chan;

    for(int i = 0; i < CHANNEL_LIMIT; i++){
        if(changed & (1 << i)){
            if(state->chan & (1 << i)){
                for(int j = 0; j < state->channel[i].count; j++){

                    state->channel[i].device[j].handle = coremodel_attach_i2c(state->cm, state->busname, state->channel[i].device[j].addr, state->channel[i].device[j].i2c_func,
                                                                              state->channel[i].device[j].dev_state, COREMODEL_I2C_WRITE_ACK);
                    if(!state->channel[i].device[j].handle){
                        printf("failed to attach %s with address %d on channel %d \n", state->channel[i].device[j].name, state->channel[i].device[j].addr, i);
                    }else{
                        printf("attach success\n");
                    }
                }
            }else{
                for(int j = 0; j < state->channel[i].count; j++){

                    if(state->channel[i].device[j].handle){
                        coremodel_detach(state->channel[i].device[j].handle);
                        state->channel[i].device[j].handle = NULL;
                    }else{
                        printf("no handle to detach %s with address %d on channel %d \n", state->channel[i].device[j].name, state->channel[i].device[j].addr, i);
                    }
                }
            }

        }
    }

}

static int i2c_mux_start(void *priv)
{

    printf("START\n");

    return 1;
}

static int i2c_mux_write(void *priv, unsigned len, uint8_t *data)
{
    i2c_mux_state_t *state = priv;
    unsigned i;

    for(i=0; i<len; i++){

#if I2C_MUX_DEBUG
            printf("i2c_mux write data %04x length %d \n", data[i], len);
#endif


        if(state->count == 0){
            state->chanp = state->chan;
            state->write = 1;
        }
        state->chan = data[i];
        state->count++;
    }
    return i;
}

static int i2c_mux_read(void *priv, unsigned len, uint8_t *data)
{
    i2c_mux_state_t *state = priv;
    unsigned i;
    printf("READ [%d]\n", len);

    for(i=0; i<len; i++){
        data[i] = state->chan;
    }

    return i;
}

static void i2c_mux_stop(void *priv)
{
    i2c_mux_state_t *state = priv;

    printf("STOP write %d\n", state->write);

    state->stop = 1;
    state->count = 0;

}

static const coremodel_i2c_func_t i2c_mux_func = {
    .start = i2c_mux_start,
    .write = i2c_mux_write,
    .read = i2c_mux_read,
    .stop = i2c_mux_stop };

const coremodel_i2c_func_t test_i2c_func = {
    .start = test_i2c_start,
    .write = test_i2c_write,
    .read = test_i2c_read,
    .stop = test_i2c_stop };

int main(int argc, char *argv[])
{
    i2c_mux_state_t *state = calloc(1, sizeof(i2c_mux_state_t));
    int res;
    char name[16];

    if(!state){
        return 0;
    }

    if(argc != 3) {
        printf("usage: coremodel-mux <address[:port]> <i2c>\n");
        free(state);
        return 1;
    }

    res = coremodel_connect(&state->cm, argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        free(state);
        return 1;
    }

    sprintf(state->busname, "%s", argv[2]);

    state->handle = coremodel_attach_i2c(state->cm, argv[2], 0x70, &i2c_mux_func, state, 0); //COREMODEL_I2C_WRITE_ACK);
    if(!state->handle) {
        fprintf(stderr, "error: failed to attach i2c mux.\n");
        coremodel_disconnect(state->cm);
        free(state);
        return 1;
    }

    for(int i = 0; i < CHANNEL_LIMIT; i++){
        sprintf(name, "i2c-dummy:%d", i);
        res = i2c_mux_connect(state, i, 0x60 + i, &test_i2c_func, NULL, name);

    }

    while(1){
        coremodel_mainloop(state->cm, 1000000);
        if(state->write && state->stop){
            state->write = 0;
            state->stop = 0;
            i2c_mux_update_channels(state);
        }
    }

    coremodel_detach(state->handle);
    coremodel_disconnect(state->cm);


    free(state);

    return 0;
}
