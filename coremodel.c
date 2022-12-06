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

static int coremodel_mainloop_int(long long usec, unsigned query);

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

static int coremodel_push_packet(struct coremodel_packet *pkt)
{
    unsigned len = pkt->len, dlen = (len + 3) & ~3;
    struct coremodel_txbuf *txb = calloc(1, sizeof(struct coremodel_txbuf) + dlen);

    if(!txb)
        return 1;
    txb->size = dlen;
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
    if(coremodel_push_packet(&pkt))
        return NULL;
    coremodel_query = 1;

    coremodel_mainloop_int(-1, 1);

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
    return coremodel_push_packet(&npkt);
}

static int coremodel_process_packet(struct coremodel_packet *pkt)
{
    if(pkt->conn == CONN_QUERY) {
        if(coremodel_query)
            switch(pkt->pkt) {
            case PKT_QUERY_RSP_LIST:
                return coremodel_process_list_response(pkt);
            case PKT_QUERY_RSP_CONN:
                break;
            }
        return 0;
    }

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
            memcpy(pkt + step, coremodel_rxq, 8 - step);
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

void coremodel_disconnect(void)
{
    close(coremodel_fd);
    coremodel_fd = -1;
}
