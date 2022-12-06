#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <coremodel.h>

static int test_uart_tx(void *priv, unsigned len, uint8_t *data)
{
    return len;
}

static void test_uart_brk(void *priv)
{
}

static void test_uart_rxrdy(void *priv)
{
}

static const coremodel_uart_func_t test_uart_func = {
    .tx = test_uart_tx,
    .brk = test_uart_brk,
    .rxrdy = test_uart_rxrdy };
 
int main(int argc, char *argv[])
{
    int res;
    void *handle;

    if(argc != 3) {
        printf("usage: coremodel-uart <address[:port]> <uart>\n");
        return 1;
    }

    res = coremodel_connect(argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    handle = coremodel_attach_uart(argv[2], &test_uart_func, NULL);
    if(!handle) {
        fprintf(stderr, "error: failed to attach UART.\n");
        coremodel_disconnect();
        return 1;
    }

    //

    coremodel_detach(handle);
    coremodel_disconnect();

    return 0;
}
