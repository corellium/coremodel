from coremodel import *
import time

USB_CTRL_RCPT_DEV    = 0x00
USB_CTRL_RCPT_IF     = 0x01
USB_CTRL_RCPT_EP     = 0x02
USB_CTRL_RCPT_OTHER  = 0x03

USB_CTRL_TYPE_STD    = 0x00
USB_CTRL_TYPE_CLASS  = 0x20
USB_CTRL_TYPE_VEND   = 0x40

USB_CTRL_DIR_OUT     = 0x00
USB_CTRL_DIR_IN      = 0x80

USB_REQ_GET_STATUS   = 0x00
USB_REQ_CLR_FEATURE  = 0x01
USB_REQ_SET_FEATURE  = 0x03
USB_REQ_SET_ADDRESS  = 0x05
USB_REQ_GET_DESCR    = 0x06
USB_REQ_SET_DESCR    = 0x07
USB_REQ_GET_CONFIG   = 0x08
USB_REQ_SET_CONFIG   = 0x09
USB_REQ_GET_IF       = 0x0A
USB_REQ_SET_IF       = 0x0B
USB_REQ_SYNCH_FRAME  = 0x0C

USB_DT_DEVICE        = 0x01
USB_DT_CONFIG        = 0x02
USB_DT_STRING        = 0x03
USB_DT_IF            = 0x04
USB_DT_EP            = 0x05
USB_DT_DEVQUAL       = 0x06
USB_DT_OTHER_SPEED   = 0x07
USB_DT_IF_POWER      = 0x08
USB_DT_HID           = 0x21
USB_DT_HID_REPORT    = 0x22

USB_DD_SIZE          = 18
USB_CD_SIZE          = 9
USB_ID_SIZE          = 9
USB_ED_SIZE          = 7
USB_HIDD_SIZE        = 9

vkb_dev_desc = [
    USB_DD_SIZE, USB_DT_DEVICE,
    0x00, 0x02, #  bcdUSB 
    0, 0, 0, #  class, subclass, protocol 
    8, #  ep0 maxpkt 
    0x6b, 0x1d, 0x04, 0x01, #  idVendor, idProduct 
    0x01, 0x01, #  bcdDevice 
    1, 2, 0, #  iManufacturer, iProduct, iSerial 
    1 ]      #  bNumConfigurations 

vkb_conf_desc = [
    USB_CD_SIZE, USB_DT_CONFIG,
    0, 0, # wTotalLength - will patch when reading
    1,    # bNumInterfaces
    0x01, # bConfigurationValue
    3,    # iConfiguration
    0xa0, # bmAttributes
    0x00, # bMaxPower

    USB_ID_SIZE, USB_DT_IF,
    0x00, # bInterfaceNumber
    0x00, # bAlternateSetting
    1, # bNumEndpoints
    3, 1, 1, # class, subclass, protocol
    4, # iInterface

        USB_HIDD_SIZE, USB_DT_HID,
        0x11, 0x01, # bcdHID
        0x21, # bCountryCode = US
        0x01, # bNumDescriptors
        USB_DT_HID_REPORT, # bDescriptorType
        0x3F, 0x00, # wDescriptorLength

        USB_ED_SIZE, USB_DT_EP,
        0x81, # address (in)
        0x03, # interrupt
        0x08, 0x00, # pkt size
        2 ] # poll interval


vkb_str_desc_00 = [USB_DT_STRING << 8, 0x0409 ] # language descriptor (US-English)
vkb_str_desc_01 = [USB_DT_STRING << 8, 'C', 'o', 'r', 'e', 'l', 'l', 'i', 'u', 'm' ]
vkb_str_desc_02 = [USB_DT_STRING << 8, 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd' ]
vkb_str_desc_03 = [USB_DT_STRING << 8, 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd' ]
vkb_str_desc_04 = [USB_DT_STRING << 8, 'H', 'I', 'D', ' ', 'D', 'e', 'v', 'i', 'c', 'e' ]

vkb_str_desc_tbl = [
    [vkb_str_desc_00, len(vkb_str_desc_00)],
    [vkb_str_desc_01, len(vkb_str_desc_01)],
    [vkb_str_desc_02, len(vkb_str_desc_02)],
    [vkb_str_desc_03, len(vkb_str_desc_03)],
    [vkb_str_desc_04, len(vkb_str_desc_04)]
]

vkb_hid_report_desc = [
    0x05, 0x01,       # Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,       # Usage (Keyboard)
    0xA1, 0x01,       # Collection (Application)
    0x05, 0x07,       # Usage Page (Kbrd/Keypad)
    0x19, 0xE0,       # Usage Minimum (0xE0)
    0x29, 0xE7,       # Usage Maximum (0xE7)
    0x15, 0x00,       # Logical Minimum (0)
    0x25, 0x01,       # Logical Maximum (1)
    0x75, 0x01,       # Report Size (1)
    0x95, 0x08,       # Report Count (8)
    0x81, 0x02,       # Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,       # Report Count (1)
    0x75, 0x08,       # Report Size (8)
    0x81, 0x01,       # Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,       # Report Count (5)
    0x75, 0x01,       # Report Size (1)
    0x05, 0x08,       # Usage Page (LEDs)
    0x19, 0x01,       # Usage Minimum (Num Lock)
    0x29, 0x05,       # Usage Maximum (Kana)
    0x91, 0x02,       # Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,       # Report Count (1)
    0x75, 0x03,       # Report Size (3)
    0x91, 0x01,       # Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,       # Report Count (6)
    0x75, 0x08,       # Report Size (8)
    0x15, 0x00,       # Logical Minimum (0)
    0x25, 0x65,       # Logical Maximum (101)
    0x05, 0x07,       # Usage Page (Kbrd/Keypad)
    0x19, 0x00,       # Usage Minimum (0x00)
    0x29, 0x65,       # Usage Maximum (0x65)
    0x81, 0x00,       # Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0 ]            # End Collection

usb_tkn_names = [ "OUT", "IN", "SETUP" ]

class Usbh(CoreModelUsbh):
    def __init__(self, busname, port, speed):
        super().__init__(busname = busname, port = port, speed = speed)
        self.bAddr = 0
        self.in_setup = 0

        self.bmReqType = 0
        self.bRequest = 0
        self.wValue = 0
        self.wIndex = 0
        self.wLength = 0
        self.ep0buf = [0] * 512
        self.ep0ptr = 0

        self.last_report = [0] * 8
        self.has_report = 0

    def rst(self):
        print("reset")

    def usbh_xfr_int (self, dev, ep, tkn, buf, size, end):
        step = 0
        i = 0

        eptkn = ep | tkn << 7

        if(eptkn == 0x100): #  control EP - SETUP 
            self.bmReqType = buf[0]
            self.bRequest = buf[1]
            self.wValue = buf[2] | buf[3] << 8
            self.wIndex = buf[4] | buf[5] << 8
            self.wLength = buf[6] | buf[7] << 8
            self.in_setup = 1
            self.ep0ptr = 0

            if(not(self.bmReqType & USB_CTRL_DIR_IN)):
                return size

            #  device->host setup requests are handled here 
            if(self.bmReqType == (USB_CTRL_RCPT_DEV | USB_CTRL_TYPE_STD | USB_CTRL_DIR_IN)):
                if(self.bRequest == USB_REQ_GET_DESCR):
                    wvalue = self.wValue >> 8
                    if( wvalue == USB_DT_DEVICE):
                        self.wLength = min(len(vkb_dev_desc), self.wLength)
                        for index in range(self.wLength):
                            self.ep0buf[index] = vkb_dev_desc[index]
                        return size
                    if(self.bRequest == USB_DT_CONFIG):
                        self.wLength = min(len(vkb_conf_desc), self.wLength)
                        for index in range(self.wLength):
                            self.ep0buf[index] = vkb_dev_desc[index]
                        self.ep0buf[2] = self.wLength
                        self.ep0buf[3] = self.wLength >> 8
                        return size
                    if(self.bRequest == USB_DT_STRING):
                        i = self.wValue & 255
                        if(i < len(vkb_str_desc_tbl) / len(vkb_str_desc_tbl[0])):
                            self.wLength = min(vkb_str_desc_tbl[i][1], self.wLength)
                            for index in range(self.wLength):
                                self.ep0buf[index] = vkb_str_desc_tbl[i][0][index]
                            self.ep0buf[0] = self.wLength
                            return size
            if(self.bmReqType == (USB_CTRL_RCPT_IF | USB_CTRL_TYPE_STD | USB_CTRL_DIR_IN)):
                if(self.bRequest ==USB_REQ_GET_DESCR):
                    if((self.wValue >> 8) == USB_DT_HID_REPORT):
                        self.wLength = min(len(vkb_hid_report_desc), self.wLength)
                        for index in range(self.wLength):
                            self.ep0buf[index] = vkb_hid_report_desc[index]
                        return size
            return USB_XFR_STALL

        if(eptkn == 0x00): #  control EP - OUT 
            if(not(self.in_setup)):
                return USB_XFR_STALL

            if(self.bmReqType & USB_CTRL_DIR_IN):
                #  acknowledge ZLP 
                self.in_setup = 0
                return size

            step = self.wLength - self.ep0ptr
            if(step > size):
                step = size
            for index in range(step):
                self.ep0buf[index + self.ep0ptr] = buf[index]
            self.ep0ptr += step
            return step

        if(eptkn == 0x80): #  control EP - IN 
            if(not(self.in_setup)):
                return USB_XFR_STALL

            if(not(self.bmReqType & USB_CTRL_DIR_IN)):
                # acknowledge ZLP 
                self.in_setup = 0

                # host->device setup requests are handled here 
                if(self.bmReqType == (USB_CTRL_RCPT_DEV | USB_CTRL_TYPE_STD | USB_CTRL_DIR_OUT)):
                    if(self.bRequest == USB_REQ_SET_ADDRESS):
                        self.bAddr = self.wValue
                        return size
                    if(self.bRequest == USB_REQ_SET_CONFIG):
                        return size
                if(self.bmReqType == (USB_CTRL_RCPT_EP | USB_CTRL_TYPE_STD | USB_CTRL_DIR_OUT)):
                    if(self.bRequest == USB_REQ_CLR_FEATURE):
                        # only legal option is ENDPOINT_HALT, which clears a STALL 
                        return size
                return USB_XFR_STALL

            step = self.wLength - self.ep0ptr
            if(step > size):
                step = size
            for index in range(step):
                buf[index] = self.ep0buf[index + self.ep0ptr]
            self.ep0ptr += step
            return step

        if(eptkn == 0x81): #  interrupt EP - IN 
            if(self.has_report):
                self.has_report = 0
                step = min(8, size)

                for index in range(step):
                    buf[index] = self.last_report[index]
                return step
        return USB_XFR_NAK

    def xfr(self, dev, ep, tkn, buf, size, end):
        i = 0
        res = 0

        print("XFR", dev, "EP", ep, usb_tkn_names[tkn], "[", size,"]")
        if((tkn == USB_TKN_OUT) or (tkn == USB_TKN_SETUP)):
            print(":")
            for index in range(size):
                print(buf[index], sep = " ")

        res = self.usbh_xfr_int(dev, ep, tkn, buf, size, end)

        if((tkn == USB_TKN_IN) and (res > 0)):
            print(" ->", end = "")
            for index in range(res):
                print(buf[i], sep = " ", end ="")
            print()
        else:
            print(" -> ", res)

        return res

    def __del__(self):
        pass

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("usage: python3 coremodel-usbh.py <address> <port> <usb bus> <usb port>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]
    usbport = sys.argv[4]

    cm0 = CoreModel("cm0", address, port, "./../../")

    usbh0 = Usbh(busname, int(usbport), 1)

    try:
        cm0.attach(usbh0)
    except NameError as e:
        print(str(e))
        sys.exit(1)

    cm0.cycle_time = 8000000

    cm0.start()

    for i in range(5):
        time.sleep(1)

    cm0.stop_event.set()
    cm0.join()