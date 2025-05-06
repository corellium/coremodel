#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <nshostport.h>

void nshostport_clean(struct nshostport *addr)
{
    if (addr->ns) {
        free(addr->ns);
        addr->ns = NULL;
    }

    if (addr->host) {
        free(addr->host);
        addr->host = NULL;
    }

    addr->port = 0;
}

int nshostport_parse(const char *str, struct nshostport *out)
{
    const char *hostStart = NULL;
    const char *hostEnd = NULL;
    const char *portStart = NULL;
    const char *portEnd = NULL;
    const char *nsEnd = NULL;
    const char *afterNS = NULL;
    const char *colon = NULL;
    char *portPart = NULL;
    size_t nsSize = 0;
    size_t hostSize = 0;
    size_t portSize = 0;

    nshostport_clean(out);

    nsEnd = strrchr(str, '/');
    if (nsEnd)
        afterNS = nsEnd + 1;
    else
        afterNS = str;

    hostStart = strchr(afterNS, '[');
    if (hostStart) {
        ++hostStart;
        hostEnd = strchr(hostStart, ']');
        if (!hostEnd)
            goto error;
    }

    if (!hostStart) {
        if (strchr(afterNS, '.')) {
            /* Case where there's an IPv4 address. */
            hostStart = afterNS;
            colon = strchr(afterNS, ':');
            if (colon) {
                /* There's a host and port. */
                hostEnd = colon;
            } else {
                /* There's only the host. */
                hostEnd = afterNS + strlen(afterNS);
            }
        } else {
            colon = strchr(afterNS, ':');
            if (colon) {
                if (!strchr(colon + 1, ':')) {
                    /* Only one colon, and no dots. This is invalid. */
                    goto error;
                }

                /* Multiple colons, but no brackets. There's no port. */
                hostStart = afterNS;
                hostEnd = afterNS + strlen(afterNS);
            } else {
                /* No colons, no dots. Interpret as port. */
            }
        }
    }

    if (hostEnd) {
        if (*hostEnd != 0)
            portStart = hostEnd + 1;
        else
            portStart = hostEnd;
        portEnd = portStart + strlen(portStart);
    } else {
        /* No host at all. After namespace is port. */
        portStart = afterNS;
        portEnd = portStart + strlen(portStart);
    }

    if (nsEnd) {
        nsSize = (uintptr_t)(nsEnd - str) + 1;
        out->ns = calloc(1, nsSize);
        if (!out->ns)
            goto error;

        strncpy(out->ns, str, nsSize);
        out->ns[nsSize - 1] = 0;
    }

    hostSize = (uintptr_t)(hostEnd - hostStart) + 1;
    if (hostStart && hostSize) {
        out->host = calloc(1, hostSize);
        if (!out->host)
            goto error;

        strncpy(out->host, hostStart, hostSize);
        out->host[hostSize - 1] = 0;
    }

    portSize = (uintptr_t)(portEnd - portStart) + 1;
    if (portStart && portSize) {
        portPart = calloc(1, portSize);
        if (!portPart)
            goto error;

        strncpy(portPart, portStart, portSize);
        portPart[portSize - 1] = 0;

        out->port = strtoull(portPart, NULL, 10);
    }

    if (portPart) {
        free(portPart);
        portPart = NULL;
    }

    return 0;

error:
    if (portPart) {
        free(portPart);
        portPart = NULL;
    }

    nshostport_clean(out);
    return -1;
}
