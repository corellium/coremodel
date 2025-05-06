#ifndef _NSHOSTPORT_H
#define _NSHOSTPORT_H

struct nshostport {
    char *ns;
    char *host;
    unsigned int port;
};

void nshostport_clean(struct nshostport *addr);
int nshostport_parse(const char *str, struct nshostport *out);

#endif /* _NSHOSTPORT_H */
