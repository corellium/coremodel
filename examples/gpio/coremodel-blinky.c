#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <coremodel.h>

#define PIN_NUM 54

typedef struct coremodel_gpio_state {

    struct coremodel_gpio_pin {
        struct coremodel_gpio_state *state;
        void *pin_handle; //coremodel_if
        int mvolt;
    } pin[PIN_NUM];

    uint32_t live;

} coremodel_gpio_state_t;

static void test_gpio_notify(void *priv, int mvolt)
{

    struct coremodel_gpio_pin *pin = priv;
    coremodel_gpio_state_t *state = pin->state;
    unsigned pin_idx = pin - state->pin;


    printf("GPIO[%d] = %d mV\n", pin_idx, mvolt);
    fflush(stdout);

    switch(pin_idx){
    case 15:
        if(mvolt){
            state->live = 0;
        }
        break;
    case 16:
        if(state->pin[17].pin_handle){
            coremodel_gpio_set(state->pin[17].pin_handle, 1, mvolt ? 0 : 3300);
        }
        break;
    case 17:
        break;
    }

    pin->mvolt = mvolt;
}

static const coremodel_gpio_func_t test_gpio_func = {
    .notify = test_gpio_notify };

int main(int argc, char *argv[])
{
    int res, num, idx, *gpios;
    void *handle;
    void *cm;

    coremodel_gpio_state_t *state;
    state = calloc(1, sizeof(coremodel_gpio_state_t));
    if(!state){
        fprintf(stderr, "error: out of memory.\n");
        return 1;
    }

    if(argc < 3) {
        printf("usage: coremodel-gpio <address[:port]> <gpio0> [...]\n");
        return 1;
    }
    num = argc - 3;
    gpios = calloc(sizeof(int), num);
    if(!gpios) {
        fprintf(stderr, "error: out of memory.\n");
        return 1;
    }
    for(idx=0; idx<num; idx++)
        gpios[idx] = strtoul(argv[3 + idx], NULL, 0);


    res = coremodel_connect(&cm ,argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    for(idx=0; idx<num; idx++) {
        if((gpios[idx] < 0) || (gpios[idx] >= PIN_NUM)){
            continue;
        }

        state->pin[gpios[idx]].state = state;
        handle = coremodel_attach_gpio(cm, argv[2], gpios[idx], &test_gpio_func, &state->pin[gpios[idx]]);

        if(!handle) {
            fprintf(stderr, "error: failed to attach gpio %d.\n", gpios[idx]);
            coremodel_disconnect(cm);
            return 1;
        }
        state->pin[gpios[idx]].pin_handle = handle;
    }

    state->live = 1;
    while(state->live) {
        coremodel_mainloop(cm, 800000);
    }

    for(idx=0; idx<num; idx++) {
        if((gpios[idx] < 0) || (gpios[idx] >= PIN_NUM)){
            continue;
        }
        if(state->pin[gpios[idx]].pin_handle){
            coremodel_gpio_set(state->pin[gpios[idx]].pin_handle, 1, 0);
            coremodel_mainloop(cm, 100000);
            coremodel_gpio_set(state->pin[gpios[idx]].pin_handle, 0, 0);
            coremodel_mainloop(cm, 100000);
            coremodel_detach(state->pin[gpios[idx]].pin_handle);
        }
    }

    coremodel_disconnect(cm);
    free(gpios);
    free(state);

    printf("\n coremodel gpio disconnected by remote \n");


    return 0;
}
