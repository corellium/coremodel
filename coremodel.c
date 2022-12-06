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

/*
Common protocol header:
    u16 len             packet length in bytes, including this header
    u16 conn            connection index (0xFFFF for query)
    u8 pkt              packet type
    u8 bflag            flag byte
    u16 hflag           flag half-word
Headers always aligned to 4 bytes.
 */

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

static int coremodel_fd = -1;

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

    res = 1;
    setsockopt(coremodel_fd, IPPROTO_TCP, TCP_NODELAY, (void *)&res, sizeof(res));

    return 0;
}

int coremodel_preparefds(int nfds, fd_set *readfds, fd_set *writefds)
{
    //
    if(1) {
        FD_SET(coremodel_fd, readfds);
        if(coremodel_fd >= nfds)
            nfds = coremodel_fd + 1;
    }
    return nfds;
}

int coremodel_processfds(fd_set *readfds, fd_set *writefds)
{
    if(coremodel_fd < 0)
        return -ENOTCONN;

    if(FD_ISSET(coremodel_fd, readfds)) {
        //
    }

    return 0;
}

static uint64_t coremodel_get_microtime(void)
{
    struct timespec tsp;
    clock_gettime(CLOCK_MONOTONIC, &tsp);
    return tsp.tv_sec * 1000000ul + (tsp.tv_nsec / 1000ul);
}

int coremodel_mainloop(long long usec)
{
    long long now_us = coremodel_get_microtime();
    long long end_us = now_us + usec;
    fd_set readfds, writefds;
    struct timeval tv = { 0, 0 };
    int res;

    while(usec < 0 || end_us >= now_us) {
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

void coremodel_disconnect(void)
{
    close(coremodel_fd);
    coremodel_fd = -1;
}
