/*
 * CoreModel EventBus Example
 *
 * Copyright (c) 2026 Corellium Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <coremodel.h>

static void test_event_update(void *priv, uint64_t data0, uint64_t data1, unsigned initial)
{
    printf("event[%s]: %016lx %016lx%s\n", (char *)priv, data0, data1, initial ? " *" : "");
    fflush(stdout);
}

static const coremodel_event_func_t test_event_func = {
    .update = test_event_update };

int main(int argc, char *argv[])
{
    int res, num, idx;
    void *handle;
    void *cm;
    char **names, *datastr;
    uint64_t data[2];

    if(argc < 2) {
        printf("usage: coremodel-event <address[:port]> <eventname0> [...]\n");
        return 1;
    }

    num = argc - 2;
    names = calloc(sizeof(char *), num);
    if(!names)
        return 1;
    for(idx=0; idx<num; idx++) {
        names[idx] = strdup(argv[2 + idx]);
        if(!names[idx])
            return 1;
    }

    res = coremodel_connect(&cm, argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    for(idx=0; idx<num; idx++) {
        datastr = strchr(names[idx], '=');
        if(datastr)
            *(datastr ++) = 0;

        handle = coremodel_attach_event_name(cm, names[idx], &test_event_func, (void *)names[idx]);
        if(!handle) {
            fprintf(stderr, "error: failed to attach event %s.\n", names[idx]);
            coremodel_disconnect(cm);
            return 1;
        }

        if(datastr) {
            data[0] = strtoul(datastr, &datastr, 0);
            if(*datastr == ',')
                data[1] = strtoul(datastr + 1, NULL, 0);
            else
                data[1] = 0;
            coremodel_event_signal(handle, data[0], data[1], 0);
        }
    }

    coremodel_mainloop(cm, -1);

    coremodel_detach(handle);
    coremodel_disconnect(cm);

    for(idx=0; idx<num; idx++)
        free(names[idx]);
    free(names);

    return 0;
}
