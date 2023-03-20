/*
 *  CoreModel C API
 *
 *  Copyright Corellium 2022
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#define _BSD_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <alloca.h>

#include "coremodel.h"

#define DFLT_PORT               1900

#define CONN_QUERY              0xFFFF

struct coremodel_packet {
    uint16_t len;
    uint16_t conn;
    uint8_t pkt;
    uint8_t bflag;
    uint16_t hflag;
    uint8_t data[0];
};

/* packet types for query connection: bflag = Dom0 connection ID */
#define PKT_QUERY_REQ_LIST      0x00    /* list controllers, starting with index hflag */
#define PKT_QUERY_RSP_LIST      0x01    /* list of controllers, starting with index hflag, data: { u16 type, u16 strlen, u32 num, u8 str[] } struct per controller */
#define PKT_QUERY_REQ_CONN      0x02    /* connection request, data: one struct as above to connect to, where num = index of endpoint in controller, hflag: passed to connection type */
#define PKT_QUERY_RSP_CONN      0x03    /* connection response: hflag: connection index (0xFFFF for failure), data: initial credit (uint32, optional) */
#define PKT_QUERY_REQ_DISC      0x04    /* disconnection request, connection index hflag (no response) */

/* packet types for UART connection */
#define PKT_UART_TX             0x00    /* data from VM to host */
#define PKT_UART_RX             0x01    /* data from host to VM */
#define PKT_UART_RX_ACK         0x02    /* credits returned to host; hflag: number of credits */
#define PKT_UART_BRK            0x03    /* break condition received */

/* packet types for I2C connection; REQ_CONN hflag[0]: start always ACKs, hflag[1]: write always ACKs */
#define PKT_I2C_START           0x00    /* bflag[0]: requires DONE; hflag: transaction index */
#define PKT_I2C_WRITE           0x01    /* bflag[0]: requires DONE; hflag: transaction index; data: write data */
#define PKT_I2C_READ            0x02    /* bflag: number of bytes, hflag: transaction index */
#define PKT_I2C_STOP            0x03    /* hflag: transaction index */
#define PKT_I2C_DONE            0x04    /* bflag[0]: NAK status, hflag: transaction index; data: read data */

/* packet types for SPI connection; REQ_CONN hflag[0]: accept bulk data (otherwise byte-by-byte) */
#define PKT_SPI_CS              0x00    /* bflag[0]: chip select enabled */
#define PKT_SPI_TX              0x01    /* hflag: transaction index; data: from VM to host */
#define PKT_SPI_RX              0x02    /* hflag: transaction index; data: from host to VM */

/* packet types for GPIO connection */
#define PKT_GPIO_UPDATE         0x00    /* hflag: new GPIO state in mV */
#define PKT_GPIO_FORCE          0x01    /* bflag[0]: enable driver; hflag: GPIO state in mV */

/* packet types for USB host connection; REQ_CONN hflag[3:0]: connection speed enum */
#define PKT_USBH_RESET          0x00
#define PKT_USBH_XFR            0x01    /* bflag: transaction index, hflag[3:0]: token, hflag[7:4]: ep, hflag[14:8]: dev, hflag[15]: end, data: write data (OUT/SETUP) or 16-bit length (IN) */
#define PKT_USBH_DONE           0x02    /* bflag: transaction index, hflag[3:0]: token, hflag[7:4]: ep, hflag[14:8]: dev, hflag[15]: stall, data: 16-bit length (OUT/SETUP), read data (IN) */

/* packet types for CAN connection */
#define PKT_CAN_TX              0x00    /* packet from VM to host; bflag: transaction index, hflag[0]: response expected; data: 64-bit control field followed by packet data */
#define PKT_CAN_TX_ACK          0x01    /* bflag: transaction index, hflag[0]: NAK */
#define PKT_CAN_RX              0x02    /* packet from host to VM; bflag: transaction index; data: 64-bit control field followed by packet data */
#define PKT_CAN_RX_ACK          0x03    /* bflag: transaction index, hflag[0]: NAK */
#define PKT_CAN_SET_NNAK        0x04    /* set filter of packets that will not auto-NAK */
#define PKT_CAN_SET_ACK         0x05    /* set filter of packets that will auto-ACK */

#define RX_BUF                  4096
#define MAX_PKT                 1024

static int coremodel_fd = -1;

static struct coremodel_txbuf {
    struct coremodel_txbuf *next;
    unsigned size, rptr;
    uint8_t buf[0];
} *coremodel_txbufs = NULL, **coremodel_etxbufs = &coremodel_txbufs;
static int coremodel_txflag = 0;

static uint8_t coremodel_rxq[RX_BUF];
static uint32_t coremodel_rxqwp = 0, coremodel_rxqrp = 0;

static coremodel_device_list_t *coremodel_device_list = NULL;
static unsigned coremodel_device_list_size = 0;

static unsigned coremodel_query = 0;

static struct coremodel_if {
    struct coremodel_if *next;
    uint16_t conn, trnidx;
    unsigned type;
    unsigned cred, busy, offs;
    uint64_t ebusy;
    union {
        const void *func;
        const coremodel_uart_func_t *uartf;
        const coremodel_i2c_func_t *i2cf;
        const coremodel_spi_func_t *spif;
        const coremodel_gpio_func_t *gpiof;
        const coremodel_usbh_func_t *usbhf;
        const coremodel_can_func_t *canf;
    };
    void *priv;
    struct coremodel_rxbuf {
        struct coremodel_rxbuf *next;
        struct coremodel_packet pkt;
    } *rxbufs, **erxbufs;
    uint8_t rdbuf[512];
} *coremodel_ifs = NULL, *coremodel_conn_if = NULL;

static int coremodel_mainloop_int(long long usec, unsigned query);
static void coremodel_advance_if(struct coremodel_if *cif);

int coremodel_connect(const char *target)
{
    char *strp;
    char *port = NULL;
    unsigned portn;
    struct hostent *hent;
    struct sockaddr_in saddr;
    int res;

    if(!target)
        target = getenv("COREMODEL_VM");
    if(!target) {
        fprintf(stderr, "[coremodel] Set environment variable COREMODEL_VM to the address:port of the Corellium VM and try again.\n");
        return -EINVAL;
    }

    strp = malloc(strlen(target) + 1);
    if(!strp) {
        fprintf(stderr, "[coremodel] Memory allocation error.\n");
        return -ENOMEM;
    }
    strcpy(strp, target);

    port = strchr(strp, ':');
    if(port) {
        *(port++) = 0;
        portn = strtol(port, NULL, 0);
    } else
        portn = DFLT_PORT;

    hent = gethostbyname(strp);
    if(!hent) {
        fprintf(stderr, "[coremodel] Failed to resolve host %s: %s.\n", strp, hstrerror(h_errno));
        return -ENOENT;
    }

    coremodel_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(coremodel_fd < 0) {
        perror("[coremodel] Failed to create socket");
        return -errno;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(portn);
    saddr.sin_addr = *(struct in_addr *)hent->h_addr;

    if(connect(coremodel_fd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
        fprintf(stderr, "[coremodel] Failed to connect to %s:%d: %s.\n", strp, portn, strerror(errno));
        close(coremodel_fd);
        coremodel_fd = -1;
        return -errno;
    }

    if(fcntl(coremodel_fd, F_SETFL, fcntl(coremodel_fd, F_GETFL, 0) | O_NONBLOCK) < 0) {
        fprintf(stderr, "[coremodel] Failed to set non-blocking: %s.\n", strerror(errno));
        close(coremodel_fd);
        coremodel_fd = -1;
        return -errno;
    }

    res = 1;
    setsockopt(coremodel_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&res, sizeof(res));

    return 0;
}

static int coremodel_push_packet(struct coremodel_packet *pkt, void *data)
{
    unsigned len = pkt->len, dlen = (len + 3) & ~3;
    struct coremodel_txbuf *txb = calloc(1, sizeof(struct coremodel_txbuf) + dlen);

    if(!txb)
        return 1;
    txb->size = dlen;
    if(data) {
        memcpy(txb->buf, pkt, 8);
        memcpy(txb->buf + 8, data, len - 8);
    } else
        memcpy(txb->buf, pkt, len);

    *coremodel_etxbufs = txb;
    coremodel_etxbufs = &txb->next;
    coremodel_txflag = 1;
    return 0;
}

coremodel_device_list_t *coremodel_list(void)
{
    struct coremodel_packet pkt = { .len = 8, .conn = CONN_QUERY, .pkt = PKT_QUERY_REQ_LIST };
    coremodel_device_list_t *res;

    if(coremodel_query)
        return NULL;
    if(coremodel_push_packet(&pkt, NULL))
        return NULL;
    coremodel_query = 1;

    if(coremodel_mainloop_int(-1, 1)) {
        coremodel_free_list(coremodel_device_list);
        coremodel_device_list = NULL;
        coremodel_query = 0;
    }

    res = coremodel_device_list;
    coremodel_device_list = NULL;
    coremodel_device_list_size = 0;
    return res;
}

void coremodel_free_list(coremodel_device_list_t *list)
{
    unsigned idx;

    if(!list)
        return;

    for(idx=0; list[idx].type!=COREMODEL_INVALID; idx++)
        free(list[idx].name);
    free(list);
}

static int coremodel_process_list_response(struct coremodel_packet *pkt)
{
    struct coremodel_packet npkt = { .len = 8, .conn = CONN_QUERY, .pkt = PKT_QUERY_REQ_LIST };
    unsigned offs, len, step, num, base;
    uint8_t *ptr;
    void *prev;

    if(pkt->len == 8) {
        coremodel_query = 0;
        return 0;
    }

    len = pkt->len - 8;
    base = pkt->hflag;
    for(offs=0,num=base; offs<len; offs+=step,num++) {
        ptr = pkt->data + offs;
        step = (11 + *(uint16_t *)(ptr + 2)) & ~3;
    }

    prev = coremodel_device_list;
    coremodel_device_list = realloc(prev, sizeof(coremodel_device_list_t) * (num + 1));
    if(!coremodel_device_list) {
        free(prev);
        coremodel_query = 0;
        return 0;
    }
    if(coremodel_device_list_size < num + 1)
        memset(coremodel_device_list + coremodel_device_list_size, 0, sizeof(coremodel_device_list_t) * (num + 1 - coremodel_device_list_size));
    coremodel_device_list_size = num + 1;

    for(offs=0,num=base; offs<len; offs+=step,num++) {
        ptr = pkt->data + offs;
        step = *(uint16_t *)(ptr + 2);
        coremodel_device_list[num].type = *(uint16_t *)ptr;
        coremodel_device_list[num].name = calloc(step + 1, 1);
        if(!coremodel_device_list[num].name) {
            coremodel_free_list(coremodel_device_list);
            coremodel_device_list = NULL;
            coremodel_query = 0;
            return 0;
        }
        memcpy(coremodel_device_list[num].name, ptr + 8, step);
        coremodel_device_list[num].num = *(uint32_t *)(ptr + 4);
        step = (11 + step) & ~3;
    }
    coremodel_device_list[num].type = COREMODEL_INVALID;

    npkt.hflag = num;
    return coremodel_push_packet(&npkt, NULL);
}

static void *coremodel_attach_int(unsigned type, const char *name, unsigned addr, const void *func, void *priv, uint16_t flags)
{
    struct coremodel_if *cif;
    struct coremodel_packet *pkt;
    unsigned nlen = strlen(name);

    if(coremodel_query)
        return NULL;

    cif = calloc(1, sizeof(struct coremodel_if));
    if(!cif)
        return NULL;

    pkt = alloca(sizeof(*pkt) + 8 + nlen);
    memset(pkt, 0, sizeof(*pkt));
    pkt->len = 16 + nlen;
    pkt->conn = CONN_QUERY;
    pkt->pkt = PKT_QUERY_REQ_CONN;
    pkt->hflag = flags;
    *(uint16_t *)pkt->data = type;
    *(uint16_t *)(pkt->data + 2) = nlen;
    *(uint32_t *)(pkt->data + 4) = addr;
    memcpy(pkt->data + 8, name, nlen);
    if(coremodel_push_packet(pkt, NULL)) {
        free(cif);
        return NULL;
    }

    cif->conn = CONN_QUERY;
    cif->type = type;
    cif->func = func;
    cif->priv = priv;
    cif->erxbufs = &cif->rxbufs;
    coremodel_conn_if = cif;
    coremodel_query = 1;

    if(coremodel_mainloop_int(-1, 1)) {
        coremodel_query = 0;
        coremodel_conn_if = NULL;
        cif->conn = CONN_QUERY;
    }

    if(cif->conn == CONN_QUERY) {
        free(coremodel_conn_if);
        coremodel_conn_if = NULL;
        return NULL;
    }
    coremodel_conn_if = NULL;
    return cif;
}

static int coremodel_process_conn_response(struct coremodel_packet *pkt)
{
    if(coremodel_conn_if) {
        coremodel_conn_if->conn = pkt->hflag;
        if(pkt->len >= 12)
            coremodel_conn_if->cred = *(uint32_t *)pkt->data;
        if(pkt->hflag != CONN_QUERY) {
            coremodel_conn_if->next = coremodel_ifs;
            coremodel_ifs = coremodel_conn_if;
        }
        coremodel_query = 0;
    }
    return 0;
}

void *coremodel_attach_uart(const char *name, const coremodel_uart_func_t *func, void *priv)
{
    return coremodel_attach_int(COREMODEL_UART, name, 0, func, priv, 0);
}
void *coremodel_attach_i2c(const char *name, uint8_t addr, const coremodel_i2c_func_t *func, void *priv, uint16_t flags)
{
    return coremodel_attach_int(COREMODEL_I2C, name, addr, func, priv, flags);
}
void *coremodel_attach_spi(const char *name, unsigned csel, const coremodel_spi_func_t *func, void *priv, uint16_t flags)
{
    return coremodel_attach_int(COREMODEL_SPI, name, csel, func, priv, flags);
}
void *coremodel_attach_gpio(const char *name, unsigned pin, const coremodel_gpio_func_t *func, void *priv)
{
    return coremodel_attach_int(COREMODEL_GPIO, name, pin, func, priv, 0);
}
void *coremodel_attach_usbh(const char *name, unsigned port, const coremodel_usbh_func_t *func, void *priv, unsigned speed)
{
    return coremodel_attach_int(COREMODEL_USBH, name, port, func, priv, speed);
}
void *coremodel_attach_can(const char *name, const coremodel_can_func_t *func, void *priv)
{
    return coremodel_attach_int(COREMODEL_CAN, name, 0, func, priv, 0);
}

static void coremodel_ready_int(void *handle)
{
    struct coremodel_if *cif = handle;

    if(cif) {
        cif->busy = 0;
        coremodel_advance_if(cif);
    }
}

static int coremodel_advance_if_uart(struct coremodel_if *cif, struct coremodel_packet *pkt)
{
    int res;

    switch(pkt->pkt) {
    case PKT_UART_TX:
        if(cif->busy)
            return 1;
        if(cif->uartf->tx)
            res = cif->uartf->tx(cif->priv, (pkt->len - 8) - cif->offs, pkt->data + cif->offs);
        else
            res = (pkt->len - 8) - cif->offs;
        if(!res) {
            cif->busy = 1;
            break;
        }
        cif->offs += res;
        if(cif->offs < pkt->len - 8)
            return 1;
        cif->offs = 0;
        return 0;

    case PKT_UART_RX_ACK:
        if(!cif->cred) {
            cif->cred += pkt->hflag;
            if(cif->uartf->rxrdy)
                cif->uartf->rxrdy(cif->priv);
        } else
            cif->cred += pkt->hflag;
        return 0;

    case PKT_UART_BRK:
        if(cif->uartf->brk)
            cif->uartf->brk(cif->priv);
        return 0;
    }

    return 0;
}

int coremodel_uart_rx(void *uart, unsigned len, uint8_t *data)
{
    struct coremodel_if *cif = uart;
    struct coremodel_packet pkt = { .pkt = PKT_UART_RX };

    if(!cif || !cif->cred)
        return 0;
    if(len > cif->cred)
        len = cif->cred;

    pkt.len = 8 + len;
    pkt.conn = cif->conn;
    if(coremodel_push_packet(&pkt, data))
        return 0;

    cif->cred -= len;
    return len;
}

void coremodel_uart_txrdy(void *uart)
{
    coremodel_ready_int(uart);
}

static int coremodel_advance_if_i2c(struct coremodel_if *cif, struct coremodel_packet *pkt)
{
    struct coremodel_packet npkt = { .len = 8, .conn = cif->conn, .pkt = PKT_I2C_DONE };
    int res;

    if(cif->busy)
        return 1;
    cif->trnidx = pkt->hflag;
    npkt.hflag = cif->trnidx;

    switch(pkt->pkt) {
    case PKT_I2C_START:
        if(cif->i2cf->start)
            res = cif->i2cf->start(cif->priv);
        else
            res = 1;
        if(res == 0) {
            cif->busy = 1;
            return 1;
        }
        if(pkt->bflag & 1) {
            npkt.bflag = (res < 0) ? 1 : 0;
            coremodel_push_packet(&npkt, NULL);
        }
        return 0;

    case PKT_I2C_WRITE:
        if(cif->i2cf->write)
            res = cif->i2cf->write(cif->priv, (pkt->len - 8) - cif->offs, pkt->data + cif->offs);
        else
            res = -1;
        if(res == 0) {
            cif->busy = 1;
            return 1;
        }
        if(res < 0) {
            if(pkt->bflag & 1) {
                npkt.bflag = 1;
                coremodel_push_packet(&npkt, NULL);
            }
            return 0;
        }
        cif->offs += res;
        if(cif->offs < pkt->len - 8)
            return 1;
        cif->offs = 0;
        if(pkt->bflag & 1)
            coremodel_push_packet(&npkt, NULL);
        return 0;

    case PKT_I2C_READ:
        if(cif->i2cf->read)
            res = cif->i2cf->read(cif->priv, pkt->bflag - cif->offs, cif->rdbuf + cif->offs);
        else
            res = pkt->bflag - cif->offs;
        if(res == 0) {
            cif->busy = 1;
            return 1;
        }
        cif->offs += res;
        if(cif->offs < pkt->bflag)
            return 1;
        cif->offs = 0;
        npkt.len = 8 + pkt->bflag;
        coremodel_push_packet(&npkt, cif->rdbuf);
        return 0;

    case PKT_I2C_STOP:
        if(cif->i2cf->stop)
            cif->i2cf->stop(cif->priv);
        return 0;
    }

    return 0;
}

int coremodel_i2c_push_read(void *i2c, unsigned len, uint8_t *data)
{
    struct coremodel_if *cif = i2c;
    struct coremodel_packet pkt = { .pkt = PKT_I2C_DONE };

    if(!cif)
        return 0;

    if(len > 255)
        len = 255;
    pkt.len = 8 + len;
    pkt.conn = cif->conn;
    pkt.hflag = cif->trnidx;
    if(coremodel_push_packet(&pkt, data))
        return 0;
    return len;
}

void coremodel_i2c_ready(void *i2c)
{
    coremodel_ready_int(i2c);
}

static int coremodel_advance_if_spi(struct coremodel_if *cif, struct coremodel_packet *pkt)
{
    int res;
    struct coremodel_packet npkt = { .pkt = PKT_SPI_RX };

    switch(pkt->pkt) {
    case PKT_SPI_CS:
        if(cif->spif->cs)
            cif->spif->cs(cif->priv, pkt->bflag & 1);
        return 0;
    case PKT_SPI_TX:
        if(cif->busy)
            return 1;
        cif->trnidx = pkt->hflag;
        res = (pkt->len - 8) - cif->offs;
        if(res > 256)
            res = 256;
        if(cif->spif->xfr)
            res = cif->spif->xfr(cif->priv, res, pkt->data + cif->offs, cif->rdbuf + cif->offs);
        if(!res) {
            cif->busy = 1;
            return 1;
        }
        cif->offs += res;
        if(cif->offs < pkt->len - 8)
            return 1;
        cif->offs = 0;
        npkt.len = pkt->len;
        npkt.conn = cif->conn;
        npkt.hflag = cif->trnidx;
        coremodel_push_packet(&npkt, cif->rdbuf);
        return 0;
    }

    return 0;
}

void coremodel_spi_ready(void *spi)
{
    coremodel_ready_int(spi);
}

static int coremodel_advance_if_gpio(struct coremodel_if *cif, struct coremodel_packet *pkt)
{
    switch(pkt->pkt) {
    case PKT_GPIO_UPDATE:
        if(cif->gpiof->notify)
            cif->gpiof->notify(cif->priv, (int16_t)pkt->hflag);
        return 0;
    }
    return 0;
}

void coremodel_gpio_set(void *pin, unsigned drven, int mvolt)
{
    struct coremodel_if *cif = pin;
    struct coremodel_packet pkt = { .len = 8, .pkt = PKT_I2C_DONE };

    if(!cif)
        return;

    pkt.conn = cif->conn;
    pkt.bflag = !!drven;
    pkt.hflag = mvolt;
    coremodel_push_packet(&pkt, NULL);
}

static int coremodel_advance_if_usbh(struct coremodel_if *cif, struct coremodel_packet *pkt)
{
    struct coremodel_packet npkt = { .pkt = PKT_USBH_DONE };
    uint8_t dev, ep, tkn, end;
    uint16_t size;
    int res;

    switch(pkt->pkt) {
    case PKT_USBH_RESET:
        if(!cif->busy) {
            cif->busy = 1;
            return -2;
        }
        cif->busy = 0;
        cif->ebusy = 0;
        if(cif->usbhf->rst)
            cif->usbhf->rst(cif->priv);
        return 0;

    case PKT_USBH_XFR:
        if(cif->busy)
            return 0;
        ep = (pkt->hflag >> 4) & 15;
        tkn = pkt->hflag & 15;
        if(tkn == USB_TKN_SETUP)
            cif->ebusy &= ~(1ul << (ep * 4 + tkn));
        if(cif->ebusy & (1ul << (ep * 4 + tkn)))
            return -1;
        dev = (pkt->hflag >> 8) & 127;
        end = pkt->hflag >> 15;
        if(cif->usbhf->xfr) {
            if(tkn == USB_TKN_IN) {
                if(pkt->len < 10)
                    return 0;
                size = *(uint16_t *)pkt->data;
                if(size > sizeof(cif->rdbuf))
                    size = sizeof(cif->rdbuf);
                res = cif->usbhf->xfr(cif->priv, dev, ep, tkn, cif->rdbuf, size, end);
            } else
                res = cif->usbhf->xfr(cif->priv, dev, ep, tkn, pkt->data, pkt->len - 8, end);
        } else
            res = USB_XFR_NAK;
        if(tkn == USB_TKN_SETUP)
            return 0;
        if(res == USB_XFR_NAK) {
            cif->ebusy |= 1ul << (ep * 4 + tkn);
            return -1;
        }
        npkt.conn = cif->conn;
        npkt.bflag = pkt->bflag;
        npkt.hflag = tkn | (ep << 4) | (dev << 8);
        if(res < 0) {
            npkt.len = (tkn == USB_TKN_IN) ? 8 : 10;
            npkt.hflag |= 0x8000;
            size = 0;
            coremodel_push_packet(&npkt, &size);
        } else {
            if(tkn == USB_TKN_IN) {
                npkt.len = 8 + res;
                coremodel_push_packet(&npkt, cif->rdbuf);
            } else {
                npkt.len = 10;
                size = res;
                coremodel_push_packet(&npkt, &size);
            }
        }
        return 0;
    }
    return 0;
}

void coremodel_usbh_ready(void *usb, uint8_t ep, uint8_t tkn)
{
    struct coremodel_if *cif = usb;

    if(cif) {
        cif->ebusy &= ~(1ul << (ep * 4 + tkn));
        coremodel_advance_if(cif);
    }
}

static const unsigned coremodel_can_datalen[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };

static int coremodel_advance_if_can(struct coremodel_if *cif, struct coremodel_packet *pkt)
{
    int res;
    struct { uint64_t ctrl; uint8_t data[64]; } *edata;
    struct coremodel_packet npkt = { .len = 8, .pkt = PKT_CAN_TX_ACK };
    unsigned dlen;

    switch(pkt->pkt) {
    case PKT_CAN_TX:
        if(cif->busy)
            return -1;
        if(pkt->len < 16)
            return 0;
        edata = (void *)pkt->data;
        dlen = coremodel_can_datalen[(edata->ctrl & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT];
        if(pkt->len < 16 + dlen)
            return 0;
        if(cif->canf->tx)
            res = cif->canf->tx(cif->priv, edata->ctrl, edata->data);
        else
            res = CAN_NAK;
        if(res == CAN_STALL) {
            cif->busy = 1;
            return 1;
        }
        npkt.conn = cif->conn;
        npkt.bflag = pkt->bflag;
        npkt.hflag = !!res;
        coremodel_push_packet(&npkt, NULL);
        return 0;
    case PKT_CAN_RX_ACK:
        if(pkt->bflag == cif->trnidx) {
            cif->ebusy = 0;
            if(cif->canf->rxcomplete)
                cif->canf->rxcomplete(cif->priv, pkt->hflag);
        }
        return 0;
    }

    return 0;
}

int coremodel_can_rx(void *can, uint64_t ctrl, uint8_t *data)
{
    struct coremodel_if *cif = can;
    unsigned dlen = coremodel_can_datalen[(ctrl & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT];
    struct coremodel_packet pkt = { .len = 8, .pkt = PKT_CAN_RX };
    struct { uint64_t ctrl; uint8_t data[64]; } edata;

    if(!cif || cif->ebusy || (dlen && !data))
        return 1;

    cif->trnidx = (cif->trnidx + 1) & 255;

    edata.ctrl = ctrl;
    if(dlen)
        memcpy(edata.data, data, dlen);

    pkt.len = 16 + dlen;
    pkt.conn = cif->conn;
    pkt.bflag = cif->trnidx;
    pkt.hflag = 0;
    coremodel_push_packet(&pkt, &edata);

    cif->ebusy = 1;
    return 0;
}

void coremodel_can_ready(void *can)
{
    coremodel_ready_int(can);
}

static void coremodel_advance_if(struct coremodel_if *cif)
{
    struct coremodel_rxbuf *rxb, **prxb;
    int res;

    for(prxb=&cif->rxbufs; *prxb; ) {
        rxb = *prxb;
        switch(cif->type) {
        case COREMODEL_UART:
            res = coremodel_advance_if_uart(cif, &rxb->pkt);
            break;
        case COREMODEL_I2C:
            res = coremodel_advance_if_i2c(cif, &rxb->pkt);
            break;
        case COREMODEL_SPI:
            res = coremodel_advance_if_spi(cif, &rxb->pkt);
            break;
        case COREMODEL_GPIO:
            res = coremodel_advance_if_gpio(cif, &rxb->pkt);
            break;
        case COREMODEL_USBH:
            res = coremodel_advance_if_usbh(cif, &rxb->pkt);
            break;
        case COREMODEL_CAN:
            res = coremodel_advance_if_can(cif, &rxb->pkt);
            break;
        default:
            res = 0;
        }
        if(res > 0)
            break;
        if(res == 0) {
            if(!rxb->next)
                cif->erxbufs = prxb;
            *prxb = rxb->next;
            free(rxb);
            prxb = &cif->rxbufs;
            continue;
        }
        if(res == -2) {
            prxb = &cif->rxbufs;
            continue;
        }
        prxb = &rxb->next;
    }
}

static int coremodel_process_packet(struct coremodel_packet *pkt)
{
    struct coremodel_if *cif;
    struct coremodel_rxbuf *rxb;

    if(pkt->conn == CONN_QUERY) {
        if(coremodel_query)
            switch(pkt->pkt) {
            case PKT_QUERY_RSP_LIST:
                return coremodel_process_list_response(pkt);
            case PKT_QUERY_RSP_CONN:
                return coremodel_process_conn_response(pkt);
            }
        return 0;
    }

    for(cif=coremodel_ifs; cif; cif=cif->next)
        if(cif->conn == pkt->conn)
            break;
    if(!cif)
        return 0;

    rxb = calloc(1, sizeof(*rxb) + pkt->len - 8);
    if(!rxb)
        return 1;
    memcpy(&rxb->pkt, pkt, pkt->len);
    *cif->erxbufs = rxb;
    cif->erxbufs = &(rxb->next);

    coremodel_advance_if(cif);
    return 0;
}

static void coremodel_process_rxq(void)
{
    unsigned len, dlen, offs, step;
    uint8_t pkt[MAX_PKT], *buf;

    while(coremodel_rxqwp - coremodel_rxqrp >= 8) {
        offs = coremodel_rxqrp % RX_BUF;
        if(coremodel_rxqrp == RX_BUF - 1) {
            len = coremodel_rxq[RX_BUF - 1];
            len |= 256 * coremodel_rxq[0];
        } else
            len = *(uint16_t *)(coremodel_rxq + offs);
        dlen = (len + 3) & ~3;
        if(coremodel_rxqwp - coremodel_rxqrp < dlen)
            break;

        if(dlen > MAX_PKT) {
            coremodel_rxqrp += dlen;
            continue;
        }

        step = RX_BUF - offs;
        if(step < dlen) {
            memcpy(pkt, coremodel_rxq + offs, step);
            memcpy(pkt + step, coremodel_rxq, dlen - step);
            buf = pkt;
        } else
            buf = coremodel_rxq + offs;
        if(coremodel_process_packet((void *)buf))
            break;

        coremodel_rxqrp += dlen;
    }
}

int coremodel_preparefds(int nfds, fd_set *readfds, fd_set *writefds)
{
    if(coremodel_fd < 0)
        return nfds;

    if(coremodel_rxqwp - coremodel_rxqrp < RX_BUF) {
        FD_SET(coremodel_fd, readfds);
        if(coremodel_fd >= nfds)
            nfds = coremodel_fd + 1;
    }
    if(coremodel_txbufs) {
        FD_SET(coremodel_fd, writefds);
        if(coremodel_fd >= nfds)
            nfds = coremodel_fd + 1;
    }
    coremodel_txflag = 0;

    return nfds;
}

int coremodel_processfds(fd_set *readfds, fd_set *writefds)
{
    struct coremodel_txbuf *txb;
    unsigned offs, tx_flag;
    int step, res;

    if(coremodel_fd < 0)
        return -ENOTCONN;

    if(FD_ISSET(coremodel_fd, readfds))
        while(1) {
            step = RX_BUF - (coremodel_rxqwp - coremodel_rxqrp);
            if(!step)
                break;
            offs = coremodel_rxqwp % RX_BUF;
            if(step > RX_BUF - offs)
                step = RX_BUF - offs;
            res = read(coremodel_fd, coremodel_rxq + offs, step);
            if(res == 0) {
                close(coremodel_fd);
                coremodel_fd = -1;
                return -ECONNRESET;
            }
            if(res < 0) {
                if(errno == EINTR)
                    continue;
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                close(coremodel_fd);
                coremodel_fd = -1;
                return -errno;
            }
            coremodel_rxqwp += res;
        }

    coremodel_process_rxq();
    tx_flag = coremodel_txbufs && !coremodel_txflag;

    if(FD_ISSET(coremodel_fd, writefds) || tx_flag)
        while(coremodel_txbufs) {
            txb = coremodel_txbufs;
            step = txb->size - txb->rptr;
            offs = txb->rptr;
            res = write(coremodel_fd, txb->buf + offs, step);
            if(res == 0) {
                close(coremodel_fd);
                coremodel_fd = -1;
                return -ECONNRESET;
            }
            if(res < 0) {
                if(errno == EINTR)
                    continue;
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                close(coremodel_fd);
                coremodel_fd = -1;
                return -errno;
            }
            txb->rptr += res;
            if(txb->rptr >= txb->size) {
                coremodel_txbufs = txb->next;
                if(!coremodel_txbufs)
                    coremodel_etxbufs = &coremodel_txbufs;
                free(txb);
            }
        }

    return 0;
}

static uint64_t coremodel_get_microtime(void)
{
    struct timespec tsp;
    clock_gettime(CLOCK_MONOTONIC, &tsp);
    return tsp.tv_sec * 1000000ul + (tsp.tv_nsec / 1000ul);
}

static int coremodel_mainloop_int(long long usec, unsigned query)
{
    long long now_us = coremodel_get_microtime();
    long long end_us = now_us + usec;
    fd_set readfds, writefds;
    struct timeval tv = { 0, 0 };
    int res;

    while((usec < 0 || end_us >= now_us) && (!query || coremodel_query)) {
        if(usec >= 0) {
            tv.tv_sec = (end_us - now_us) / 1000000ull;
            tv.tv_usec = (end_us - now_us) % 1000000ull;
        }
        FD_ZERO(&writefds);
        FD_ZERO(&readfds);
        int nfds = coremodel_preparefds(0, &readfds, &writefds);
        if(nfds < 0)
            return nfds;
        select(nfds, &readfds, &writefds, NULL, usec >= 0 ? &tv : NULL);
        res = coremodel_processfds(&readfds, &writefds);
        if(res)
            return res;
        now_us = coremodel_get_microtime();
    }
    return 0;
}

int coremodel_mainloop(long long usec)
{
    return coremodel_mainloop_int(usec, 0);
}

void coremodel_detach(void *handle)
{
    struct coremodel_packet pkt = { .len = 8, .conn = CONN_QUERY, .pkt = PKT_QUERY_REQ_DISC };
    struct coremodel_if *cif = handle, **pcif;
    struct coremodel_rxbuf *rxb;

    if(!cif)
        return;

    for(pcif=&coremodel_ifs; *pcif; )
        if(*pcif == cif)
            *pcif = cif->next;
        else
            pcif= &((*pcif)->next);

    while(cif->rxbufs) {
        rxb = cif->rxbufs;
        cif->rxbufs = rxb->next;
        free(rxb);
    }

    pkt.hflag = cif->conn;
    free(cif);

    coremodel_push_packet(&pkt, NULL);
}

void coremodel_disconnect(void)
{
    struct coremodel_txbuf *txb;

    while(coremodel_ifs)
        coremodel_detach(coremodel_ifs);
    close(coremodel_fd);
    coremodel_fd = -1;

    while(coremodel_txbufs) {
        txb = coremodel_txbufs;
        coremodel_txbufs = txb->next;
        free(txb);
    }
    coremodel_etxbufs = &coremodel_txbufs;

    coremodel_rxqwp = coremodel_rxqrp = 0;

    coremodel_free_list(coremodel_device_list);
    coremodel_device_list = NULL;
    coremodel_device_list_size = 0;

    free(coremodel_conn_if);
    coremodel_conn_if = NULL;

    coremodel_query = 0;
}
