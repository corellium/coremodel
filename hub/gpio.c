#include <stdint.h>
#include <coremodel.h>
#include <hub.h>

static void gpio_notify_net(struct vif_node *src, int mvolt)
{
    struct vif_node *vif;
    struct netlist *net;

    net = src->net.head;
    if(!net)
        return;

    net = net->head;
    while(net){
        vif = container_of(net, struct vif_node, net);
        if(vif != src)
            coremodel_gpio_set(vif->handle, 1, mvolt);

        net = net->next;
    }
}

static void gpio_notify(void *priv, int mvolt)
{
    struct vif_node *vif = priv;
    gpio_notify_net(vif, mvolt);
}

const coremodel_gpio_func_t vif_gpio = {
    .notify = gpio_notify
};
