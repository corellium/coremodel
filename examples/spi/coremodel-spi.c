#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <coremodel.h>

static void test_spi_cs(void *priv, unsigned csel)
{
}

static int test_spi_xfr(void *priv, unsigned len, uint8_t *wrdata, uint8_t *rddata)
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

static const coremodel_spi_func_t test_spi_func = {
    .cs = test_spi_cs,
    .xfr = test_spi_xfr };
 
int main(int argc, char *argv[])
{
    int res;
    void *handle;

    if(argc != 4) {
        printf("usage: coremodel-spi <address[:port]> <spi> <cs>\n");
        return 1;
    }

    res = coremodel_connect(argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    handle = coremodel_attach_spi(argv[2], atoi(argv[3]), &test_spi_func, NULL, COREMODEL_SPI_BLOCK);
    if(!handle) {
        fprintf(stderr, "error: failed to attach SPI.\n");
        coremodel_disconnect();
        return 1;
    }

    coremodel_mainloop(-1);

    coremodel_detach(handle);
    coremodel_disconnect();

    return 0;
}
