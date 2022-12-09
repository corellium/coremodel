#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <coremodel.h>

#define USB_CTRL_RCPT_DEV    0x00
#define USB_CTRL_RCPT_IF     0x01
#define USB_CTRL_RCPT_EP     0x02
#define USB_CTRL_RCPT_OTHER  0x03

#define USB_CTRL_TYPE_STD    0x00
#define USB_CTRL_TYPE_CLASS  0x20
#define USB_CTRL_TYPE_VEND   0x40

#define USB_CTRL_DIR_OUT     0x00
#define USB_CTRL_DIR_IN      0x80

#define USB_REQ_GET_STATUS   0x00
#define USB_REQ_CLR_FEATURE  0x01
#define USB_REQ_SET_FEATURE  0x03
#define USB_REQ_SET_ADDRESS  0x05
#define USB_REQ_GET_DESCR    0x06
#define USB_REQ_SET_DESCR    0x07
#define USB_REQ_GET_CONFIG   0x08
#define USB_REQ_SET_CONFIG   0x09
#define USB_REQ_GET_IF       0x0A
#define USB_REQ_SET_IF       0x0B
#define USB_REQ_SYNCH_FRAME  0x0C

#define USB_DT_DEVICE        0x01
#define USB_DT_CONFIG        0x02
#define USB_DT_STRING        0x03
#define USB_DT_IF            0x04
#define USB_DT_EP            0x05
#define USB_DT_DEVQUAL       0x06
#define USB_DT_OTHER_SPEED   0x07
#define USB_DT_IF_POWER      0x08
#define USB_DT_HID           0x21
#define USB_DT_HID_REPORT    0x22

#define USB_DD_SIZE          18
#define USB_CD_SIZE          9
#define USB_ID_SIZE          9
#define USB_ED_SIZE          7
#define USB_HIDD_SIZE        9

struct {
    uint8_t bAddr, in_setup;
    uint8_t bmReqType, bRequest;
    uint16_t wValue, wIndex, wLength;
    uint8_t ep0buf[512];
    uint32_t ep0ptr;

    uint8_t last_report[8];
    uint8_t has_report;
} vkb_state;

static const uint8_t vkb_dev_desc[] = {
    USB_DD_SIZE, USB_DT_DEVICE,
    0x00, 0x02, /* bcdUSB */
    0, 0, 0, /* class, subclass, protocol */
    8, /* ep0 maxpkt */
    0x6b, 0x1d, 0x04, 0x01, /* idVendor, idProduct */
    0x01, 0x01, /* bcdDevice */
    1, 2, 0, /* iManufacturer, iProduct, iSerial */
    1 }; /* bNumConfigurations */

static const uint8_t vkb_conf_desc[] = {
    USB_CD_SIZE, USB_DT_CONFIG,
    0, 0, /* wTotalLength - will patch when reading */
    1, /* bNumInterfaces */
    0x01, /* bConfigurationValue */
    3, /* iConfiguration */
    0xa0, /* bmAttributes */
    0x00, /* bMaxPower */

    USB_ID_SIZE, USB_DT_IF,
    0x00, /* bInterfaceNumber */
    0x00, /* bAlternateSetting */
    1, /* bNumEndpoints */
    3, 1, 1, /* class, subclass, protocol */
    4, /* iInterface */

        USB_HIDD_SIZE, USB_DT_HID,
        0x11, 0x01, /* bcdHID */
        0x21, /* bCountryCode = US */
        0x01, /* bNumDescriptors */
        USB_DT_HID_REPORT, /* bDescriptorType */
        0x3F, 0x00, /* wDescriptorLength */

        USB_ED_SIZE, USB_DT_EP,
        0x81, /* address (in) */
        0x03, /* interrupt */
        0x08, 0x00, /* pkt size */
        2 }; /* poll interval */

static const uint16_t vkb_str_desc_00[] = {
    USB_DT_STRING << 8, 0x0409 }; /* language descriptor (US-English) */
static const uint16_t vkb_str_desc_01[] = {
    USB_DT_STRING << 8, 'C', 'o', 'r', 'e', 'l', 'l', 'i', 'u', 'm' };
static const uint16_t vkb_str_desc_02[] = {
    USB_DT_STRING << 8, 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd' };
static const uint16_t vkb_str_desc_03[] = {
    USB_DT_STRING << 8, 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd' };
static const uint16_t vkb_str_desc_04[] = {
    USB_DT_STRING << 8, 'H', 'I', 'D', ' ', 'D', 'e', 'v', 'i', 'c', 'e' };
static const struct {
    const void *data;
    unsigned size;
} vkb_str_desc_tbl[] = {
    { vkb_str_desc_00, sizeof(vkb_str_desc_00) },
    { vkb_str_desc_01, sizeof(vkb_str_desc_01) },
    { vkb_str_desc_02, sizeof(vkb_str_desc_02) },
    { vkb_str_desc_03, sizeof(vkb_str_desc_03) },
    { vkb_str_desc_04, sizeof(vkb_str_desc_04) } };

static const uint8_t vkb_hid_report_desc[] = {
    0x05, 0x01,        /* Usage Page (Generic Desktop Ctrls) */
    0x09, 0x06,        /* Usage (Keyboard) */
    0xA1, 0x01,        /* Collection (Application) */
    0x05, 0x07,        /*   Usage Page (Kbrd/Keypad) */
    0x19, 0xE0,        /*   Usage Minimum (0xE0) */
    0x29, 0xE7,        /*   Usage Maximum (0xE7) */
    0x15, 0x00,        /*   Logical Minimum (0) */
    0x25, 0x01,        /*   Logical Maximum (1) */
    0x75, 0x01,        /*   Report Size (1) */
    0x95, 0x08,        /*   Report Count (8) */
    0x81, 0x02,        /*   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position) */
    0x95, 0x01,        /*   Report Count (1) */
    0x75, 0x08,        /*   Report Size (8) */
    0x81, 0x01,        /*   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position) */
    0x95, 0x05,        /*   Report Count (5) */
    0x75, 0x01,        /*   Report Size (1) */
    0x05, 0x08,        /*   Usage Page (LEDs) */
    0x19, 0x01,        /*   Usage Minimum (Num Lock) */
    0x29, 0x05,        /*   Usage Maximum (Kana) */
    0x91, 0x02,        /*   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile) */
    0x95, 0x01,        /*   Report Count (1) */
    0x75, 0x03,        /*   Report Size (3) */
    0x91, 0x01,        /*   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile) */
    0x95, 0x06,        /*   Report Count (6) */
    0x75, 0x08,        /*   Report Size (8) */
    0x15, 0x00,        /*   Logical Minimum (0) */
    0x25, 0x65,        /*   Logical Maximum (101) */
    0x05, 0x07,        /*   Usage Page (Kbrd/Keypad) */
    0x19, 0x00,        /*   Usage Minimum (0x00) */
    0x29, 0x65,        /*   Usage Maximum (0x65) */
    0x81, 0x00,        /*   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position) */
    0xC0 };            /* End Collection */

static void test_usbh_rst(void *priv)
{
    printf("RESET\n");
    fflush(stdout);
}

static inline int32_t imin32(int32_t a, int32_t b) { return a < b ? a : b; }

static int test_usbh_xfr_int(void *priv, uint8_t dev, uint8_t ep, uint8_t tkn, uint8_t *buf, unsigned size, uint8_t end)
{
    unsigned step;
    int i;

    switch(ep | ((unsigned)tkn << 7)) {
    case 0x100: /* control EP - SETUP */
        vkb_state.bmReqType = buf[0];
        vkb_state.bRequest = buf[1];
        vkb_state.wValue = buf[2] | ((uint16_t)buf[3] << 8);
        vkb_state.wIndex = buf[4] | ((uint16_t)buf[5] << 8);
        vkb_state.wLength = buf[6] | ((uint16_t)buf[7] << 8);
        vkb_state.in_setup = 1;
        vkb_state.ep0ptr = 0;

        if(!(vkb_state.bmReqType & USB_CTRL_DIR_IN))
            return size;

        /* device->host setup requests are handled here */
        switch(vkb_state.bmReqType) {
        case USB_CTRL_RCPT_DEV | USB_CTRL_TYPE_STD | USB_CTRL_DIR_IN:
            switch(vkb_state.bRequest) {
            case USB_REQ_GET_DESCR:
                switch(vkb_state.wValue >> 8) {
                case USB_DT_DEVICE:
                    vkb_state.wLength = imin32(sizeof(vkb_dev_desc), vkb_state.wLength);
                    memcpy(vkb_state.ep0buf, vkb_dev_desc, vkb_state.wLength);
                    return size;
                case USB_DT_CONFIG:
                    vkb_state.wLength = imin32(sizeof(vkb_conf_desc), vkb_state.wLength);
                    memcpy(vkb_state.ep0buf, vkb_conf_desc, vkb_state.wLength);
                    vkb_state.ep0buf[2] = sizeof(vkb_conf_desc);
                    vkb_state.ep0buf[3] = sizeof(vkb_conf_desc) >> 8;
                    return size;
                case USB_DT_STRING:
                    i = vkb_state.wValue & 255;
                    if(i < sizeof(vkb_str_desc_tbl) / sizeof(vkb_str_desc_tbl[0])) {
                        vkb_state.wLength = imin32(vkb_str_desc_tbl[i].size, vkb_state.wLength);
                        memcpy(vkb_state.ep0buf, vkb_str_desc_tbl[i].data, vkb_state.wLength);
                        vkb_state.ep0buf[0] = vkb_str_desc_tbl[i].size;
                        return size;
                    }
                }
                break;
            }
            break;
        case USB_CTRL_RCPT_IF | USB_CTRL_TYPE_STD | USB_CTRL_DIR_IN:
            switch(vkb_state.bRequest) {
            case USB_REQ_GET_DESCR:
                switch(vkb_state.wValue >> 8) {
                case USB_DT_HID_REPORT:
                    vkb_state.wLength = imin32(sizeof(vkb_hid_report_desc), vkb_state.wLength);
                    memcpy(vkb_state.ep0buf, vkb_hid_report_desc, vkb_state.wLength);
                    return size;
                }
                break;
            }
            break;
        }
        return USB_XFR_STALL;

    case 0x00: /* control EP - OUT */
        if(!vkb_state.in_setup)
            return USB_XFR_STALL;

        if(vkb_state.bmReqType & USB_CTRL_DIR_IN) {
            /* acknowledge ZLP */
            vkb_state.in_setup = 0;
            return size;
        }
        step = vkb_state.wLength - vkb_state.ep0ptr;
        if(step > size)
            step = size;
        memcpy(vkb_state.ep0buf + vkb_state.ep0ptr, buf, step);
        vkb_state.ep0ptr += step;
        return step;

    case 0x80: /* control EP - IN */
        if(!vkb_state.in_setup)
            return USB_XFR_STALL;

        if(!(vkb_state.bmReqType & USB_CTRL_DIR_IN)) {
            /* acknowledge ZLP */
            vkb_state.in_setup = 0;

            /* host->device setup requests are handled here */
            switch(vkb_state.bmReqType) {
            case USB_CTRL_RCPT_DEV | USB_CTRL_TYPE_STD | USB_CTRL_DIR_OUT:
                switch(vkb_state.bRequest) {
                case USB_REQ_SET_ADDRESS:
                    vkb_state.bAddr = vkb_state.wValue;
                    return size;
                case USB_REQ_SET_CONFIG:
                    return size;
                }
                break;
            case USB_CTRL_RCPT_EP | USB_CTRL_TYPE_STD | USB_CTRL_DIR_OUT:
                switch(vkb_state.bRequest) {
                case USB_REQ_CLR_FEATURE:
                    /* only legal option is ENDPOINT_HALT, which clears a STALL */
                    return size;
                }
                break;
            }
            return USB_XFR_STALL;
        }

        step = vkb_state.wLength - vkb_state.ep0ptr;
        if(step > size)
            step = size;
        memcpy(buf, vkb_state.ep0buf + vkb_state.ep0ptr, step);
        vkb_state.ep0ptr += step;
        return step;

    case 0x81: /* interrupt EP - IN */
        if(vkb_state.has_report) {
            vkb_state.has_report = 0;
            step = imin32(8, size);
            memcpy(buf, vkb_state.last_report, step);
            return step;
        }
        break;
    }

    return USB_XFR_NAK;
}

static const char *const usb_tkn_names[] = {
    [ USB_TKN_IN ] = "IN",
    [ USB_TKN_OUT ] = "OUT",
    [ USB_TKN_SETUP ] = "SETUP" };

static int test_usbh_xfr(void *priv, uint8_t dev, uint8_t ep, uint8_t tkn, uint8_t *buf, unsigned size, uint8_t end)
{
    int i, res;

    printf("XFR %02x EP%d %s [%d]", dev, ep, usb_tkn_names[tkn], size);
    if(tkn == USB_TKN_OUT || tkn == USB_TKN_SETUP) {
        printf(":");
        for(i=0; i<size; i++)
            printf(" %02x", buf[i]);
    }

    res = test_usbh_xfr_int(priv, dev, ep, tkn, buf, size, end);

    if(tkn == USB_TKN_IN && res > 0) {
        printf(" ->");
        for(i=0; i<res; i++)
            printf(" %02x", buf[i]);
        printf("\n");
    } else
        printf(" -> %d\n", res);
    fflush(stdout);

    return res;
}

static const coremodel_usbh_func_t test_usbh_func = {
    .rst = test_usbh_rst,
    .xfr = test_usbh_xfr };
 
int main(int argc, char *argv[])
{
    int res;
    void *handle;

    if(argc != 4) {
        printf("usage: coremodel-usbh <address[:port]> <usbh> <usbh-port>\n");
        return 1;
    }

    res = coremodel_connect(argv[1]);
    if(res) {
        fprintf(stderr, "error: failed to connect: %s.\n", strerror(-res));
        return 1;
    }

    handle = coremodel_attach_usbh(argv[2], strtoul(argv[3], NULL, 0), &test_usbh_func, NULL, USB_SPEED_FULL);
    if(!handle) {
        fprintf(stderr, "error: failed to attach to USB host.\n");
        coremodel_disconnect();
        return 1;
    }

    coremodel_mainloop(-1);

    coremodel_detach(handle);
    coremodel_disconnect();

    return 0;
}
