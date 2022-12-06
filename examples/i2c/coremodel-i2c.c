#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <coremodel.h>

static int test_i2c_start(void *priv)
{
    printf("START\n");
    fflush(stdout);
    return 1;
}

static int test_i2c_write(void *priv, unsigned len, uint8_t *data)
{
    unsigned idx;
    printf("WRITE [%d]:", len);
    for(idx=0; idx<len; idx++)
        printf(" %02x", data[idx]);
    printf("\n");
    fflush(stdout);
    return len;
}

static int test_i2c_read(void *priv, unsigned len, uint8_t *data)
{
    unsigned idx;
    printf("READ [%d]\n", len);
    for(idx=0; idx<len; idx++)
        data[idx] = 0xA0 + (idx & 0x3F);
    fflush(stdout);
    return len;
}

static void test_i2c_stop(void *priv)
{
    printf("STOP\n");
    fflush(stdout);
}

static const coremodel_i2c_func_t test_i2c_func = {
    .start = test_i2c_start,
    .write = test_i2c_write,
    .read = test_i2c_read,
    .stop = test_i2c_stop };
 
int main(int argc, char *argv[])
{
    int res;
    void *handle;

    if(argc != 3) {
        printf("usage: coremodel-i2c <address[:port]> <i2c>\n");
        return 1;
    }

    res = coremodel_connect(argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    handle = coremodel_attach_i2c(argv[2], 0x42, &test_i2c_func, NULL, 0);
    if(!handle) {
        fprintf(stderr, "error: failed to attach i2c.\n");
        coremodel_disconnect();
        return 1;
    }

    coremodel_mainloop(-1);

    coremodel_detach(handle);
    coremodel_disconnect();

    return 0;
}
