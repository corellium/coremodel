#include <stdint.h>
#include <coremodel.h>
#include <hub.h>

#define GPIO_PRIV_MVOLT_GET(_vif)           (((int)(uintptr_t)(_vif)->priv))
#define GPIO_PRIV_MVOLT_SET(_vif, _mvolt)   ((_vif)->priv = (void *)(uintptr_t)(_mvolt))

static int gpio_priv_init(struct vif_node *vif)
{
    GPIO_PRIV_MVOLT_SET(vif, 0);
    return 0;
}

static int gpio_priv_free(struct vif_node *vif)
{
    return 0;
}

static void gpio_notify_net(struct vif_node *src, int mvolt)
{
    struct vif_node *vif;
    struct netlist *net;

    net = src->net.head;
    if(!net)
        return;

    GPIO_PRIV_MVOLT_SET(src, mvolt);

    net = net->head;
    while(net){
        vif = container_of(net, struct vif_node, net);
        if(vif != src){
            if(GPIO_PRIV_MVOLT_GET(vif) != mvolt){
                coremodel_gpio_set(vif->handle, 1, mvolt);
                GPIO_PRIV_MVOLT_SET(vif, mvolt);
            }
        }

        net = net->next;
    }
}

static void gpio_notify(void *priv, int mvolt)
{
    struct vif_node *vif = priv;
    gpio_notify_net(vif, mvolt);
}

const struct vif_node_config vif_gpio = {
    .gpio = {
        .notify = gpio_notify,
    },
    .init = gpio_priv_init,
    .free = gpio_priv_free
};
