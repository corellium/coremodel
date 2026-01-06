#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <coremodel.h>

void test_spi_cs(void *priv, unsigned csel)
{
}

int test_spi_xfr(void *priv, unsigned len, uint8_t *wrdata, uint8_t *rddata)
{
    unsigned idx;
    printf("[%d]", len);
    for(idx=0; idx<len; idx++)
        printf(" %02x", wrdata[idx]);
    printf("\n");
    fflush(stdout);
    for(idx=0; idx<len; idx++)
        rddata[idx] = 'A' + (idx & 63);
    return len;
}
