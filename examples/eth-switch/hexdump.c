/*
 * CoreModel Ethernet Switch Example
 *
 * Copyright (c) 2026 Corellium Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>

void hexdump(void *buf, size_t len)
{
    int i;
    char ascii[17] = {0};
    uint8_t *data = (uint8_t *)buf;

    for(i=0; i<len; i++){
        if( (i != 0) && (0 == (i % 16)) )
            fprintf(stderr, "%s\n\r", ascii);

        if(data[i] >= 0x20 && data[i] <= 0x7e)
            ascii[i % 16] = data[i];
        else
            ascii[i % 16] = '.';

        fprintf(stderr, "%02x ", data[i]);
    }

    /* Special case when len != 16 byte multiple */
    if(0 != (i % 16))
        for(int j=0; j < 16 - (i % 16); j++){
            fprintf(stderr, "   ");
            ascii[ (i + j) % 16 ] = ' ';
        }

    fprintf(stderr, "%s", ascii);
    fprintf(stderr, "\n\r");
}
