#include <stdint.h>

#include <hub.h>
#include <coremodel.h>

static int can_notify_net(struct vif_node *src, uint64_t *ctrl, uint8_t *data)
{
    struct vif_node *vif;
    struct netlist *net;

    if(!src->net.head){
        pr_error("Received a message on a net with no devices?\n");
        return CAN_ACK;
    }

    /* Check if we can send CAN clients our message */
    net = src->net.head->head;
    while(net){
        vif = container_of(net, struct vif_node, net);
        if(coremodel_can_rx_busy(vif->handle)){
            //XXX: STALL?
        }
        net = net->next;
    }

    net = src->net.head->head;
    while(net){
        vif = container_of(net, struct vif_node, net);
        if(vif != src){
            if(coremodel_can_rx(vif->handle, ctrl, data)){
                //XXX: defer ?
//                pr_error("Connected endpoint not busy but failed RX\n");
            }
        }

        net = net->next;
    }

    return CAN_ACK;
}

static int vif_can_tx(void *priv, uint64_t *ctrl, uint8_t *data)
{
    struct vif_node *vif = priv;
    return can_notify_net(vif, ctrl, data);
}

static void vif_can_rx_complete(void *priv, int nak)
{
}

const struct vif_node_config vif_can = {
    .can = {
        .tx = vif_can_tx,
        .rxcomplete = vif_can_rx_complete
    }
};
