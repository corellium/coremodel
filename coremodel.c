/*
 *  CoreModel C API
 *
 *  Copyright Corellium 2022-23
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
#include <pthread.h>

#include "coremodel.h"

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

struct coremodel {
    int fd;

    struct coremodel_txbuf {
        struct coremodel_txbuf *next;
        unsigned size, rptr;
        uint8_t buf[0];
    } *txbufs, **etxbufs;

    int txflag;
    uint8_t rxq[RX_BUF];
    uint32_t rxqwp;
    uint32_t rxqrp;

    coremodel_device_list_t *device_list;
    unsigned device_list_size;

    unsigned query;

    pthread_mutexattr_t coremodel_mutex_attr;
    pthread_mutex_t coremodel_mutex;
    int coremodel_wake_fd[2];
    unsigned coremodel_need_wake;


    struct coremodel_if {
        struct coremodel *cm;
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
    } *ifs, *conn_if;
};

static int coremodel_mainloop_int(struct coremodel *cm, long long usec, unsigned query);
static void coremodel_advance_if(struct coremodel_if *cif);

static void *coremodel_init(void)
{
    struct coremodel *cm;

    cm = calloc(1, sizeof(*cm));
    if(!cm)
        return NULL;

    cm->fd = -1;
    cm->etxbufs = &cm->txbufs;
    cm->coremodel_wake_fd[0] = cm->coremodel_wake_fd[1] = -1;
    cm->coremodel_need_wake = 0;
    return cm;
}

static void coremodel_fini(void *priv)
{
    free(priv);
}

int coremodel_connect(void **priv, const char *target)
{
    struct coremodel *cm = NULL;
    char *strp = NULL;
    char *port = NULL;
    unsigned portn;
    struct hostent *hent;
    struct sockaddr_in saddr;
    int res, ret = -1;

    cm = coremodel_init();
    if(!cm){
        fprintf(stderr, "[coremodel] Memory allocation error.\n");
        errno = ENOMEM;
        goto cleanup;
    }

    if(pthread_mutexattr_init(&cm->coremodel_mutex_attr) || pthread_mutexattr_settype(&cm->coremodel_mutex_attr, PTHREAD_MUTEX_RECURSIVE) || pthread_mutex_init(&cm->coremodel_mutex, &cm->coremodel_mutex_attr)) {
        fprintf(stderr, "[coremodel] Failed to initialize mutex: %s.\n", strerror(errno));
        goto err_entry;
    }

    if(pipe(cm->coremodel_wake_fd)) {
        fprintf(stderr, "[coremodel] Failed to create wake-up pipe: %s.\n", strerror(errno));
        goto err_mutex;
    }
    if(fcntl(cm->coremodel_wake_fd[0], F_SETFL, fcntl(cm->fd, F_GETFL, 0) | O_NONBLOCK) < 0) {
        fprintf(stderr, "[coremodel] Failed to set pipe non-blocking: %s.\n", strerror(errno));
        goto err_pipe;
    }

    if(!target)
        target = getenv("COREMODEL_VM");
    if(!target) {
        fprintf(stderr, "[coremodel] Set environment variable COREMODEL_VM to the address:port of the Corellium VM and try again.\n");
        errno = EINVAL;
        goto err_pipe;
    }

    strp = malloc(strlen(target) + 1);
    if(!strp) {
        fprintf(stderr, "[coremodel] Memory allocation error.\n");
        errno = ENOMEM;
        goto err_pipe;
    }
    strcpy(strp, target);

    port = strchr(strp, ':');
    if(port) {
        *(port++) = 0;
        portn = strtol(port, NULL, 0);
    } else
        portn = COREMODEL_DFLT_PORT;

    hent = gethostbyname(strp);
    if(!hent) {
        fprintf(stderr, "[coremodel] Failed to resolve host %s: %s.\n", strp, hstrerror(h_errno));
        errno = ENOENT;
        goto err_pipe;
    }

    cm->fd = socket(AF_INET, SOCK_STREAM, 0);
    if(cm->fd < 0) {
        perror("[coremodel] Failed to create socket");
        goto err_pipe;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(portn);
    saddr.sin_addr = *(struct in_addr *)hent->h_addr;

    if(connect(cm->fd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
        fprintf(stderr, "[coremodel] Failed to connect to %s:%d: %s.\n", strp, portn, strerror(errno));
        goto err_socket;
    }

    if(fcntl(cm->fd, F_SETFL, fcntl(cm->fd, F_GETFL, 0) | O_NONBLOCK) < 0) {
        fprintf(stderr, "[coremodel] Failed to set non-blocking: %s.\n", strerror(errno));
        goto err_socket;
    }

    res = 1;
    setsockopt(cm->fd, IPPROTO_TCP, TCP_NODELAY, (void *)&res, sizeof(res));

    ret = 0;
    *priv = cm;

    return 0;


err_socket:
    close(cm->fd);
    cm->fd = -1;
err_pipe:
    close(cm->coremodel_wake_fd[0]);
    close(cm->coremodel_wake_fd[1]);
    cm->coremodel_wake_fd[0] = cm->coremodel_wake_fd[1] = -1;
err_mutex:
    pthread_mutex_destroy(&cm->coremodel_mutex);
    pthread_mutexattr_destroy(&cm->coremodel_mutex_attr);
cleanup:
    if(ret && cm){
        if(cm->fd){
            close(cm->fd);
                cm->fd = -1;
        }
        coremodel_fini(cm);
        cm = NULL;
    }
    if(strp)
        free(strp);
err_entry:
    return -errno;
}

static int coremodel_push_packet(void *priv, struct coremodel_packet *pkt, void *data)
{
    struct coremodel *cm = priv;
    unsigned len = pkt->len, dlen = (len + 3) & ~3;
    struct coremodel_txbuf *txb = calloc(1, sizeof(struct coremodel_txbuf) + dlen);
    char wake;
    int res;

    if(!txb)
        return 1;
    txb->size = dlen;
    if(data) {
        memcpy(txb->buf, pkt, 8);
        memcpy(txb->buf + 8, data, len - 8);
    } else
        memcpy(txb->buf, pkt, len);

    *cm->etxbufs = txb;
    cm->etxbufs = &txb->next;
    cm->txflag = 1;
    if(cm->coremodel_need_wake) {
        wake = 0;
        while(1) {
            res = write(cm->coremodel_wake_fd[1], &wake, 1);
            if(res >= 0)
                break;
            if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
                break;
        }
    }
    return 0;
}

coremodel_device_list_t *coremodel_list(void *priv)
{
    struct coremodel *cm = priv;
    struct coremodel_packet pkt = { .len = 8, .conn = CONN_QUERY, .pkt = PKT_QUERY_REQ_LIST };
    coremodel_device_list_t *res;

    pthread_mutex_lock(&cm->coremodel_mutex);
    if(cm->query ) {
        pthread_mutex_unlock(&cm->coremodel_mutex);
        return NULL;
    }
    if(coremodel_push_packet(cm, &pkt, NULL)) {
        pthread_mutex_unlock(&cm->coremodel_mutex);
        return NULL;
    }
    cm->query = 1;

    if(coremodel_mainloop_int(cm, -1, 1)) {
        coremodel_free_list(cm->device_list);
        cm->device_list = NULL;
        cm->query = 0;
    }

    res = cm->device_list;
    cm->device_list = NULL;
    cm->device_list_size = 0;
    pthread_mutex_unlock(&cm->coremodel_mutex);

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

static int coremodel_process_list_response(void *priv, struct coremodel_packet *pkt)
{
    struct coremodel *cm = priv;
    struct coremodel_packet npkt = { .len = 8, .conn = CONN_QUERY, .pkt = PKT_QUERY_REQ_LIST };
    unsigned offs, len, step, num, base;
    uint8_t *ptr;
    void *prev;

    if(pkt->len == 8) {
        cm->query = 0;
        return 0;
    }

    len = pkt->len - 8;
    base = pkt->hflag;
    for(offs=0,num=base; offs<len; offs+=step,num++) {
        ptr = pkt->data + offs;
        step = (11 + *(uint16_t *)(ptr + 2)) & ~3;
    }

    prev = cm->device_list;
    cm->device_list = realloc(prev, sizeof(coremodel_device_list_t) * (num + 1));
    if(!cm->device_list) {
        free(prev);
        cm->query = 0;
        return 0;
    }
    if(cm->device_list_size < num + 1)
        memset(cm->device_list + cm->device_list_size, 0, sizeof(coremodel_device_list_t) * (num + 1 - cm->device_list_size));
    cm->device_list_size = num + 1;

    for(offs=0,num=base; offs<len; offs+=step,num++) {
        ptr = pkt->data + offs;
        step = *(uint16_t *)(ptr + 2);
        cm->device_list[num].type = *(uint16_t *)ptr;
        cm->device_list[num].name = calloc(step + 1, 1);
        if(!cm->device_list[num].name) {
            coremodel_free_list(cm->device_list);
            cm->device_list = NULL;
            cm->query = 0;
            return 0;
        }
        memcpy(cm->device_list[num].name, ptr + 8, step);
        cm->device_list[num].num = *(uint32_t *)(ptr + 4);
        step = (11 + step) & ~3;
    }
    cm->device_list[num].type = COREMODEL_INVALID;

    npkt.hflag = num;
    return coremodel_push_packet(cm, &npkt, NULL);
}

static void *coremodel_attach_int(void *priv, unsigned type, const char *name, unsigned addr, const void *func, void *ifpriv, uint16_t flags)
{
    struct coremodel *cm = priv;
    struct coremodel_if *cif;
    struct coremodel_packet *pkt;
    unsigned nlen = strlen(name);

    pthread_mutex_lock(&cm->coremodel_mutex);
    if(cm->query) {
        pthread_mutex_unlock(&cm->coremodel_mutex);
        return NULL;
    }

    cif = calloc(1, sizeof(struct coremodel_if));
    if(!cif) {
        pthread_mutex_unlock(&cm->coremodel_mutex);
        return NULL;
    }

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
    if(coremodel_push_packet(cm, pkt, NULL)) {
        free(cif);
        pthread_mutex_unlock(&cm->coremodel_mutex);
        return NULL;
    }

    cif->cm = cm;
    cif->conn = CONN_QUERY;
    cif->type = type;
    cif->func = func;
    cif->priv = ifpriv;
    cif->erxbufs = &cif->rxbufs;
    cm->conn_if = cif;
    cm->query = 1;

    if(coremodel_mainloop_int(cm, -1, 1)) {
        cm->query = 0;
        cm->conn_if = NULL;
        cif->conn = CONN_QUERY;
    }

    if(cif->conn == CONN_QUERY) {
        free(cm->conn_if);
        cm->conn_if = NULL;
        pthread_mutex_unlock(&cm->coremodel_mutex);
        return NULL;
    }
    cm->conn_if = NULL;
    pthread_mutex_unlock(&cm->coremodel_mutex);
    return cif;
}

static int coremodel_process_conn_response(void *priv, struct coremodel_packet *pkt)
{
    struct coremodel *cm = priv;

    if(cm->conn_if) {
        cm->conn_if->conn = pkt->hflag;
        if(pkt->len >= 12)
            cm->conn_if->cred = *(uint32_t *)pkt->data;
        if(pkt->hflag != CONN_QUERY) {
            cm->conn_if->next = cm->ifs;
            cm->ifs = cm->conn_if;
        }
        cm->query = 0;
    }
    return 0;
}

void *coremodel_attach_uart(void *priv, const char *name, const coremodel_uart_func_t *func, void *ifpriv)
{
    return coremodel_attach_int(priv, COREMODEL_UART, name, 0, func, ifpriv, 0);
}
void *coremodel_attach_i2c(void *priv, const char *name, uint8_t addr, const coremodel_i2c_func_t *func, void *ifpriv, uint16_t flags)
{
    return coremodel_attach_int(priv, COREMODEL_I2C, name, addr, func, ifpriv, flags);
}
void *coremodel_attach_spi(void *priv, const char *name, unsigned csel, const coremodel_spi_func_t *func, void *ifpriv, uint16_t flags)
{
    return coremodel_attach_int(priv, COREMODEL_SPI, name, csel, func, ifpriv, flags);
}
void *coremodel_attach_gpio(void *priv, const char *name, unsigned pin, const coremodel_gpio_func_t *func, void *ifpriv)
{
    return coremodel_attach_int(priv, COREMODEL_GPIO, name, pin, func, ifpriv, 0);
}
void *coremodel_attach_usbh(void *priv, const char *name, unsigned port, const coremodel_usbh_func_t *func, void *ifpriv, unsigned speed)
{
    return coremodel_attach_int(priv, COREMODEL_USBH, name, port, func, ifpriv, speed);
}
void *coremodel_attach_can(void *priv, const char *name, const coremodel_can_func_t *func, void *ifpriv)
{
    return coremodel_attach_int(priv, COREMODEL_CAN, name, 0, func, ifpriv, 0);
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
    struct coremodel *cm = cif->cm;

    pthread_mutex_lock(&cm->coremodel_mutex);
    if(!cif || !cif->cred) {
        pthread_mutex_unlock(&cm->coremodel_mutex);
        return 0;
    }
    if(len > cif->cred)
        len = cif->cred;

    pkt.len = 8 + len;
    pkt.conn = cif->conn;

    if(coremodel_push_packet(cif->cm, &pkt, data)) {
        pthread_mutex_unlock(&cm->coremodel_mutex);
        return 0;
    }

    cif->cred -= len;
    pthread_mutex_unlock(&cm->coremodel_mutex);
    return len;
}

void coremodel_uart_txrdy(void *uart)
{
    struct coremodel_if *cif = uart;
    struct coremodel *cm = cif->cm;

    pthread_mutex_lock(&cm->coremodel_mutex);
    coremodel_ready_int(uart);
    pthread_mutex_unlock(&cm->coremodel_mutex);
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
            coremodel_push_packet(cif->cm, &npkt, NULL);
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
                coremodel_push_packet(cif->cm, &npkt, NULL);
            }
            return 0;
        }
        cif->offs += res;
        if(cif->offs < pkt->len - 8)
            return 1;
        cif->offs = 0;
        if(pkt->bflag & 1)
            coremodel_push_packet(cif->cm, &npkt, NULL);
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
        coremodel_push_packet(cif->cm, &npkt, cif->rdbuf);
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
    struct coremodel *cm = cif->cm;

    if(!cif)
        return 0;

    pthread_mutex_lock(&cm->coremodel_mutex);
    if(len > 255)
        len = 255;
    pkt.len = 8 + len;
    pkt.conn = cif->conn;
    pkt.hflag = cif->trnidx;

    if(coremodel_push_packet(cif->cm, &pkt, data)) {
        pthread_mutex_unlock(&cm->coremodel_mutex);
        return 0;
    }
    pthread_mutex_unlock(&cm->coremodel_mutex);
    return len;
}

void coremodel_i2c_ready(void *i2c)
{
    struct coremodel_if *cif = i2c;
    struct coremodel *cm = cif->cm;

    pthread_mutex_lock(&cm->coremodel_mutex);
    coremodel_ready_int(i2c);
    pthread_mutex_unlock(&cm->coremodel_mutex);
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
        coremodel_push_packet(cif->cm, &npkt, cif->rdbuf);
        return 0;
    }

    return 0;
}

void coremodel_spi_ready(void *spi)
{
    struct coremodel_if *cif = spi;
    struct coremodel *cm = cif->cm;

    pthread_mutex_lock(&cm->coremodel_mutex);
    coremodel_ready_int(spi);
    pthread_mutex_unlock(&cm->coremodel_mutex);
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
    struct coremodel_packet pkt = { .len = 8, .pkt = PKT_GPIO_FORCE };
    struct coremodel *cm = cif->cm;

    if(!cif)
        return;

    pthread_mutex_lock(&cm->coremodel_mutex);
    pkt.conn = cif->conn;
    pkt.bflag = !!drven;
    pkt.hflag = mvolt;

    coremodel_push_packet(cif->cm, &pkt, NULL);
    pthread_mutex_unlock(&cm->coremodel_mutex);
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
            coremodel_push_packet(cif->cm, &npkt, &size);
        } else {
            if(tkn == USB_TKN_IN) {
                npkt.len = 8 + res;
                coremodel_push_packet(cif->cm, &npkt, cif->rdbuf);
            } else {
                npkt.len = 10;
                size = res;
                coremodel_push_packet(cif->cm, &npkt, &size);
            }
        }
        return 0;
    }
    return 0;
}

void coremodel_usbh_ready(void *usb, uint8_t ep, uint8_t tkn)
{
    struct coremodel_if *cif = usb;
    struct coremodel *cm = cif->cm;

    if(cif) {
        pthread_mutex_lock(&cm->coremodel_mutex);
        cif->ebusy &= ~(1ul << (ep * 4 + tkn));
        coremodel_advance_if(cif);
        pthread_mutex_unlock(&cm->coremodel_mutex);
    }
}

static const unsigned coremodel_can_datalen[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 };

static int coremodel_advance_if_can(struct coremodel_if *cif, struct coremodel_packet *pkt)
{
    int res;
    struct coremodel_packet npkt = { .len = 8, .pkt = PKT_CAN_TX_ACK };
    void *data;
    unsigned dlc, dlen;

    switch(pkt->pkt) {
    case PKT_CAN_TX:
        if(cif->busy)
            return -1;
        if(pkt->len < 16)
            return 0;
        dlc = (*(uint64_t *)pkt->data & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT;
        dlen = (dlc < 16) ? coremodel_can_datalen[dlc] : dlc + 1;
        if(pkt->len < 16 + dlen)
            return 0;
        data = pkt->data + 16;
        if(cif->canf->tx)
            res = cif->canf->tx(cif->priv, (uint64_t *)pkt->data, data);
        else
            res = CAN_NAK;
        if(res == CAN_STALL) {
            cif->busy = 1;
            return 1;
        }
        npkt.conn = cif->conn;
        npkt.bflag = pkt->bflag;
        npkt.hflag = !!res;
        coremodel_push_packet(cif->cm, &npkt, NULL);
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

int coremodel_can_rx_busy(void *can)
{
    struct coremodel_if *cif = can;
    return cif->ebusy;
}

int coremodel_can_rx(void *can, uint64_t *ctrl, uint8_t *data)
{
    struct coremodel_if *cif = can;
    struct coremodel *cm = cif->cm;
    unsigned dlc, dlen;
    struct coremodel_packet pkt = { .pkt = PKT_CAN_RX };
    struct { uint64_t ctrl[2]; uint8_t data[2048]; } edata;

    dlc = (ctrl[0] & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT;
    dlen = (dlc < 16) ? coremodel_can_datalen[dlc] : dlc + 1;

    pthread_mutex_lock(&cm->coremodel_mutex);
    if(!cif || cif->ebusy || (dlen && !data)){
        pthread_mutex_unlock(&cm->coremodel_mutex);
        return 1;
    }

    cif->trnidx = (cif->trnidx + 1) & 255;
    memcpy(&edata.ctrl, ctrl, sizeof(edata.ctrl));
    if(dlen)
        memcpy(edata.data, data, dlen);

    pkt.len = sizeof(pkt) + sizeof(edata.ctrl) + dlen;
    pkt.conn = cif->conn;
    pkt.bflag = cif->trnidx;
    pkt.hflag = 0;
    coremodel_push_packet(cif->cm, &pkt, (void *)&edata);

    cif->ebusy = 1;
    pthread_mutex_unlock(&cm->coremodel_mutex);
    return 0;
}

void coremodel_can_ready(void *can)
{
    struct coremodel_if *cif = can;
    struct coremodel *cm = cif->cm;

    pthread_mutex_lock(&cm->coremodel_mutex);
    coremodel_ready_int(can);
    pthread_mutex_unlock(&cm->coremodel_mutex);
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

static int coremodel_process_packet(struct coremodel *cm, struct coremodel_packet *pkt)
{
    struct coremodel_if *cif;
    struct coremodel_rxbuf *rxb;

    if(pkt->conn == CONN_QUERY) {
        if(cm->query)
            switch(pkt->pkt) {
            case PKT_QUERY_RSP_LIST:
                return coremodel_process_list_response(cm, pkt);
            case PKT_QUERY_RSP_CONN:
                return coremodel_process_conn_response(cm, pkt);
            }
        return 0;
    }

    for(cif=cm->ifs; cif; cif=cif->next)
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

static void coremodel_process_rxq(void *priv)
{
    struct coremodel *cm = priv;
    unsigned len, dlen, offs, step;
    uint8_t pkt[MAX_PKT], *buf;

    while(cm->rxqwp - cm->rxqrp >= 8) {
        offs = cm->rxqrp % RX_BUF;
        if(cm->rxqrp == RX_BUF - 1) {
            len = cm->rxq[RX_BUF - 1];
            len |= 256 * cm->rxq[0];
        } else
            len = *(uint16_t *)(cm->rxq + offs);
        dlen = (len + 3) & ~3;
        if(cm->rxqwp - cm->rxqrp < dlen)
            break;

        if(dlen > MAX_PKT) {
            cm->rxqrp += dlen;
            continue;
        }

        step = RX_BUF - offs;
        if(step < dlen) {
            memcpy(pkt, cm->rxq + offs, step);
            memcpy(pkt + step, cm->rxq, dlen - step);
            buf = pkt;
        } else
            buf = cm->rxq + offs;
        if(coremodel_process_packet(cm, (void *)buf))
            break;

        cm->rxqrp += dlen;
    }
}

int coremodel_preparefds(void *priv, int nfds, fd_set *readfds, fd_set *writefds)
{
    struct coremodel *cm = priv;

    pthread_mutex_lock(&cm->coremodel_mutex);

    if(cm->fd < 0) {
        pthread_mutex_unlock(&cm->coremodel_mutex);
        return nfds;
    }

    FD_SET(cm->coremodel_wake_fd[0], readfds);
    if(cm->coremodel_wake_fd[0] >= nfds)
        nfds = cm->coremodel_wake_fd[0] + 1;
    cm->coremodel_need_wake = 1;

    if(cm->rxqwp - cm->rxqrp < RX_BUF) {
        FD_SET(cm->fd, readfds);
        if(cm->fd >= nfds)
            nfds = cm->fd + 1;
    }
    if(cm->txbufs) {
        FD_SET(cm->fd, writefds);
        if(cm->fd >= nfds)
            nfds = cm->fd + 1;
    }
    cm->txflag = 0;

    pthread_mutex_unlock(&cm->coremodel_mutex);
    return nfds;
}

int coremodel_processfds(void *priv, fd_set *readfds, fd_set *writefds)
{
    struct coremodel *cm = priv;
    struct coremodel_txbuf *txb;
    unsigned offs, tx_flag;
    int step, res;
    char tmp[16];

    pthread_mutex_lock(&cm->coremodel_mutex);

    cm->coremodel_need_wake = 0;
    if(cm->fd < 0) {
        errno = ENOTCONN;
        goto err_lock;
    }

    if(FD_ISSET(cm->coremodel_wake_fd[0], readfds)) {
        while(1) {
            res = read(cm->coremodel_wake_fd[0], &tmp, sizeof(tmp));
            if(res <= 0)
                break;
        }
    }

    if(FD_ISSET(cm->fd, readfds))
        while(1) {
            step = RX_BUF - (cm->rxqwp - cm->rxqrp);
            if(!step)
                break;
            offs = cm->rxqwp % RX_BUF;
            if(step > RX_BUF - offs)
                step = RX_BUF - offs;
            res = read(cm->fd, cm->rxq + offs, step);
            if(res == 0) {
                close(cm->fd);
                cm->fd = -1;
                errno = ECONNRESET;
                goto err_lock;
            }
            if(res < 0) {
                if(errno == EINTR)
                    continue;
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                    break;

                close(cm->fd);
                cm->fd = -1;
                goto err_lock;
            }
            cm->rxqwp += res;
        }

    coremodel_process_rxq(priv);
    tx_flag = cm->txbufs && !cm->txflag;

    if(FD_ISSET(cm->fd, writefds) || tx_flag)
        while(cm->txbufs) {
            txb = cm->txbufs;
            step = txb->size - txb->rptr;
            offs = txb->rptr;
            res = write(cm->fd, txb->buf + offs, step);
            if(res == 0) {
                close(cm->fd);
                cm->fd = -1;
                errno = ECONNRESET;
                goto err_lock;
            }
            if(res < 0) {
                if(errno == EINTR)
                    continue;
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                close(cm->fd);
                cm->fd = -1;
                goto err_lock;
            }
            txb->rptr += res;
            if(txb->rptr >= txb->size) {
                cm->txbufs = txb->next;
                if(!cm->txbufs)
                    cm->etxbufs = &cm->txbufs;
                free(txb);
            }
        }

    pthread_mutex_unlock(&cm->coremodel_mutex);
    return 0;

err_lock:
    pthread_mutex_unlock(&cm->coremodel_mutex);
    return -errno;
}

static uint64_t coremodel_get_microtime(void)
{
    struct timespec tsp;
    clock_gettime(CLOCK_MONOTONIC, &tsp);
    return tsp.tv_sec * 1000000ul + (tsp.tv_nsec / 1000ul);
}

static int coremodel_mainloop_int(struct coremodel *cm, long long usec, unsigned query)
{
    long long now_us = coremodel_get_microtime();
    long long end_us = now_us + usec;
    fd_set readfds, writefds;
    struct timeval tv = { 0, 0 };
    int res;

    while((usec < 0 || end_us >= now_us) && (!query || cm->query)) {
        if(usec >= 0) {
            tv.tv_sec = (end_us - now_us) / 1000000ull;
            tv.tv_usec = (end_us - now_us) % 1000000ull;
        }
        FD_ZERO(&writefds);
        FD_ZERO(&readfds);
        int nfds = coremodel_preparefds(cm, 0, &readfds, &writefds);
        if(nfds < 0)
            return nfds;
        select(nfds, &readfds, &writefds, NULL, usec >= 0 ? &tv : NULL);
        res = coremodel_processfds(cm, &readfds, &writefds);
        if(res)
            return res;
        now_us = coremodel_get_microtime();
    }
    return 0;
}

int coremodel_mainloop(void *priv, long long usec)
{
    return coremodel_mainloop_int(priv, usec, 0);
}

void coremodel_detach(void *handle)
{
    struct coremodel_packet pkt = { .len = 8, .conn = CONN_QUERY, .pkt = PKT_QUERY_REQ_DISC };
    struct coremodel_if *cif = handle, **pcif;
    struct coremodel_rxbuf *rxb;
    struct coremodel *cm = cif->cm;

    if(!cif)
        return;

    pthread_mutex_lock(&cm->coremodel_mutex);
    for(pcif=&cif->cm->ifs; *pcif; )
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
    coremodel_push_packet(cif->cm, &pkt, NULL);
    free(cif);
    pthread_mutex_unlock(&cm->coremodel_mutex);
}

void coremodel_disconnect(void *priv)
{
    struct coremodel *cm = priv;
    struct coremodel_txbuf *txb;

    while(cm->ifs)
        coremodel_detach(cm->ifs);
    close(cm->fd);
    cm->fd = -1;

    close(cm->coremodel_wake_fd[0]);
    close(cm->coremodel_wake_fd[1]);
    cm->coremodel_wake_fd[0] = cm->coremodel_wake_fd[1] = -1;

    while(cm->txbufs) {
        txb = cm->txbufs;
        cm->txbufs = txb->next;
        free(txb);
    }
    cm->etxbufs = &cm->txbufs;

    cm->rxqwp = cm->rxqrp = 0;

    coremodel_free_list(cm->device_list);
    cm->device_list = NULL;
    cm->device_list_size = 0;

    free(cm->conn_if);
    cm->conn_if = NULL;

    cm->query = 0;
    coremodel_fini(cm);
    pthread_mutex_destroy(&cm->coremodel_mutex);
    pthread_mutexattr_destroy(&cm->coremodel_mutex_attr);

    cm->coremodel_need_wake = 0;
    cm->query = 0;
}
