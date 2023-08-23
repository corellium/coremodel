#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <coremodel.h>

static void test_gpio_notify(void *priv, int mvolt)
{
    printf("GPIO[%d] = %d mV\n", *(int *)priv, mvolt);
    fflush(stdout);
}

static const coremodel_gpio_func_t test_gpio_func = {
    .notify = test_gpio_notify };
 
int main(int argc, char *argv[])
{
    int res, num, idx, *gpios;
    void *handle;

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


    res = coremodel_connect(argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    for(idx=0; idx<num; idx++) {
        handle = coremodel_attach_gpio(argv[2], gpios[idx], &test_gpio_func, &gpios[idx]);
        if(!handle) {
            fprintf(stderr, "error: failed to attach gpio %d.\n", gpios[idx]);
            coremodel_disconnect();
            return 1;
        }
    }

    coremodel_mainloop(-1);

    coremodel_detach(handle);
    coremodel_disconnect();
    free(gpios);

    return 0;
}
