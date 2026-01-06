#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <coremodel.h>

int test_i2c_start(void *priv)
{
    printf("START\n");
    fflush(stdout);
    return 1;
}

int test_i2c_write(void *priv, unsigned len, uint8_t *data)
{
    unsigned idx;
    printf("WRITE [%d]:", len);
    for(idx=0; idx<len; idx++)
        printf(" %02x", data[idx]);
    printf("\n");
    fflush(stdout);
    return len;
}

int test_i2c_read(void *priv, unsigned len, uint8_t *data)
{
    unsigned idx;
    printf("READ [%d]\n", len);
    for(idx=0; idx<len; idx++)
        data[idx] = 0xA0 + (idx & 0x3F);
    fflush(stdout);
    return len;
}

void test_i2c_stop(void *priv)
{
    printf("STOP\n");
    fflush(stdout);
}

