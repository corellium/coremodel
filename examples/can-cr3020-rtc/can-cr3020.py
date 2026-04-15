from coremodel import *
import time
from datetime import datetime

class cr3020(CoreModelCan):
    def __init__(self, busname, devname):
        super().__init__(busname = busname)
        self.devname = devname
        self.initialized = 0
        self.node_id = 0x13
        self.alarm_day = 0
        self.alarm_hour = 0
        self.alarm_minute = 0
        self.alarm_enabled = 0

    def format_7bit_bcd (self, binary_value):
        return ((int(binary_value / 10) << 4) + binary_value % 10) & 0x7f

    def format_6bit_bcd(self, binary_value):
        return self.format_7bit_bcd(binary_value) & 0x3f

    def format_5bit_bcd(self, binary_value):
        return self.format_7bit_bcd(binary_value) & 0x1f

    def tx(self, ctrl, data):

        unix_timestamp = time.time()
        dt_object = datetime.fromtimestamp(unix_timestamp)

        # find the length of data portion of the CAN frame
        dlen = CAN_DATALEN[(ctrl[0] & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT]
        ID = (ctrl[0] & CAN_CTRL_ID_MASK) >> CAN_CTRL_ID_SHIFT # 11 bit ID
        IDE = (ctrl[0] & CAN_CTRL_IDE) >> 34  # Extended Frame Format Flag
        RTR = (ctrl[0] & CAN_CTRL_RTR) >> 35  # Remote Transmission Request Flag
        EID = (ctrl[0] & CAN_CTRL_EID_MASK) >> CAN_CTRL_EID_SHIFT # 18 bit EID
        CEID = (ID << 18) | EID # combined ID and EID as shown by candump
        if IDE is not None: # if extended ID
            RTR = (ctrl[0] & CAN_CTRL_ERTR) >> 15  # Extended Remote Transmission Request Flag
            print("ID", hex(CEID), "RTR", hex(RTR))
        else:
            print("ID", hex(ID), "RTR", hex(RTR))

        # note fields for CAN XL not addressed

        # print raw CTRL and data
        if dlen > 0:
            print(" [", hex(ctrl[0]), " ", hex(ctrl[1]), "] len ", dlen, sep="")
            for idx in range(dlen):
                print(hex(data[idx]), end="")
                if (idx % 2) and (idx<(dlen-1)): # print _ every 4 characters
                    print("_", end="")

            print("")
        else:
            print(" [", hex(ctrl[0]), hex(ctrl[1]), "]")


        # end packet dump, begin RTC functionality
        rxctrl = [ 0, 0 ]  # return control message
        txdata = [ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]                # return data

        if IDE is not None: # Note the RTC only handles SFF frames
            return CAN_ACK

        # programming manual section 6.3.1
        if (ID == 0x7ff) and (dlen == 0): # broadcast initialization
            self.initialized = 1
            print("Initialization received.")
            rxctrl[0] |= (0x7fe << CAN_CTRL_ID_SHIFT) # set return ID
            rxctrl[0] |= (0x2 << CAN_CTRL_DLC_SHIFT)  # set return size

            txdata[0] = self.node_id # send the node id
            txdata[1] = 0x2     # set the baud rate to 500 kBaud ignored
            # send the reply
            if self.rx(rxctrl, txdata) > 0:
                print("Rx send failed", file=sys.stderr)

            return CAN_ACK # only handle a single transaction

        # device doesn't listen until initialized
        if self.initialized == 0:
            return CAN_ACK

        # programming manual section 6.3.2
        if ID == (0x300 + self.node_id):
            if dlen != 6: # RTC set
                print("Incorrect argument count", dlen, "for RTC Set command.")
            else:
                print("RTC Set.")
                print("Ignoring request to set Day", hex(data[0]), "Month", hex(data[1]), "Year", hex(data[2]), "Hour", hex(data[3]), "Minute", hex(data[4]), "DOW", hex(data[5]))
                rxctrl[0] |= ((0x280 + self.node_id) << CAN_CTRL_ID_SHIFT) # set return ID
                rxctrl[0] |= (0x6 << CAN_CTRL_DLC_SHIFT) # set return size

                txdata[0] = self.format_6bit_bcd(dt_object.month)
                print("Day of Month [1-31]: ", hex(txdata[0]))
                txdata[1] = self.format_5bit_bcd(dt_object.month + 1)
                print("Month [1-12]", hex(txdata[1]), "+", end = "")
                temp = self.format_5bit_bcd(dt_object.year / 100)
                txdata[1] = txdata[1] + (temp << 7)
                print("[Century]", hex(temp), "=", hex(txdata[1]))
                txdata[2] = (int((dt_object.year % 100) / 10) << 4) + dt_object.year % 10
                print("Year since 1900 [0-99]:", hex(txdata[2]))
                txdata[3] = self.format_6bit_bcd(dt_object.hour)
                print("Hours [00-23]:", hex(txdata[3]))
                txdata[4] = self.format_7bit_bcd(dt_object.minute)
                print("Minutes [00-59]:", hex(txdata[4]))
                txdata[5] = dt_object.day + 1
                print("Day of Week [1-7]:", hex(txdata[5]))

                # send the reply
                if self.rx(rxctrl, txdata) > 0:
                    print("Rx send failed", file=sys.stderr)

                return CAN_ACK # only handle a single transaction

        # programming manual section 6.3.3
        if (ID == 0x200 + self.node_id) and (dlen == 0): # RTC request
            print("RTC Request.")
            rxctrl[0] |= ((0x180 + self.node_id) << CAN_CTRL_ID_SHIFT) # set return ID
            rxctrl[0] |= (0x8 << CAN_CTRL_DLC_SHIFT) # set return size



            txdata[0] = self.format_6bit_bcd(dt_object.day)
            print("Day of Month [1-31]:", hex(txdata[0]))
            txdata[1] = self.format_5bit_bcd(dt_object.month + 1)
            print("Month [1-12]", hex(txdata[1]) ,"+", end="")
            temp = self.format_5bit_bcd(dt_object.year / 100)
            txdata[1] = txdata[1] + (temp << 7)
            print("[Century]", hex(temp), "=", hex(txdata[1]), end="")
            txdata[2] = (int((dt_object.year % 100) / 10) << 4) + dt_object.year % 10
            print("Year since 1900 [0-99]:", hex(txdata[2]))
            txdata[3] = self.format_6bit_bcd(dt_object.hour)
            print("Hours [00-23]:", hex(txdata[3]), end="")
            txdata[4] = self.format_7bit_bcd(dt_object.minute)
            print("Minutes [00-59]:", hex(txdata[4]))
            txdata[5] = self.format_7bit_bcd(dt_object.second)
            print("Seconds [00-59]:", hex(txdata[5]))
            txdata[6] = dt_object.day + 1
            print("Day of Week [1-7]:", hex(txdata[6]))
            txdata[7] = 0x0 # Battery state sufficient
            print("Battery State Sufficient")

            # send the reply
            if self.rx(rxctrl, txdata) > 0:
                print("Rx send failed", file=sys.stderr)

            return CAN_ACK # only handle a single transaction

        # programming manual section 6.3.4
        if ID == 0x500 + self.node_id:
            if dlen != 7 : # Alarm Set
                print("Incorrect argument count", dlen ,"for Alarm Set command.")
            else:
                print("Setting Alarm.")
                rxctrl[0] |= (0x7 << CAN_CTRL_DLC_SHIFT) # set return size


                if data[0] < 7:
                    self.alarm_day = (((dt_object.day - 1) + data[0]) % 7) + 1
                txdata[0] = data[0]
                print("Days until alarm [0-6]", hex(txdata[0]))

                if data[1] < 24:
                    self.alarm_hour = (dt_object.hour  + data[1]) % 24
                txdata[1] = data[1]
                print("Hours until alarm [00-23]", hex(txdata[1]))

                if data[2] < 60:
                    self.alarm_minute = (dt_object.minute  + data[2]) % 60
                txdata[2] = data[2]
                print("Minutes until alarm [00-59]", hex(txdata[2]))

                if data[3] == data[4] and data[3] < 0x40:
                    self.node_id = data[3]
                    print("Setting NodeID", hex(data[3]))

                txdata[3] = self.node_id
                txdata[4] = self.node_id
                txdata[5] = 0x2     # set the baud rate to 500 kBaud ignored
                print("Setting Baud Rate to 500 kBd")
                if data[6] == 00 or data[6] == 0xff:
                    self.alarm_enabled = data[6]

                txdata[6] = self.alarm_enabled
                print("Setting Enabled to", hex(data[6]))

                # delay until after potential node_id update
                rxctrl[0] |= ((0x480 + self.node_id) << CAN_CTRL_ID_SHIFT) # set return ID

                # send the reply
                if(self.rx(rxctrl, txdata) > 0):
                    print("Rx send failed", file=sys.stderr)

                return CAN_ACK # only handle a single transaction

        # programming manual section 6.3.5
        if(ID == 0x400 + self.node_id) and (dlen == 0): # RTC request
            print("Request Time to Alarm.")
            rxctrl[0] |= ((0x380 + self.node_id) << CAN_CTRL_ID_SHIFT) # set return ID
            rxctrl[0] |= (0x7 << CAN_CTRL_DLC_SHIFT) # set return size

            temp = self.alarm_day
            if temp < dt_object.day:
                temp += 7
            txdata[0] = temp - dt_object.day
            print("Days until alarm [0-6]", hex(txdata[0]))

            temp = self.alarm_hour
            if temp < dt_object.hour:
                temp += 24
            txdata[1] = self.format_6bit_bcd(temp - dt_object.hour)
            print("Hours until alarm [00-23]", hex(txdata[1]))

            temp = self.alarm_minute
            if temp < dt_object.day:
                temp += 60
            txdata[2] = self.format_7bit_bcd(temp - dt_object.minute)
            print("Minutes until alarm [00-59]", hex(txdata[2]))

            txdata[3] = self.node_id
            txdata[4] = self.node_id
            print("NodeID", hex(txdata[3]))
            txdata[5] = 0x2     # set the baud rate to 500 kBaud ignored
            print("Baud Rate to 500 kBd")
            data[6] = self.alarm_enabled
            print("Enabled is", hex(data[6]))

            # send the reply
            if self.rx(rxctrl, txdata) > 0:
                print("Rx send failed", file=sys.stderr)

            return CAN_ACK # only handle a single transaction

        return CAN_ACK

    def rxcomplete(self, nak):
        print(" ->", nak)

    def __del__(self):
        pass

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("usage: python3 can-cr3020.py <address> <port> <canbus>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]

    cm0 = CoreModel("cm0", address, port, "./../../")

    cr3020_0 = cr3020(busname,"cr3020_0")

    try:
        cm0.attach(cr3020_0)
    except NameError as e:
        print(str(e))
        sys.exit(1)

    cm0.cycle_time = 8000000

    cm0.start()

    for i in range(3):
        time.sleep(1)

    cm0.stop_event.set()
    cm0.join()
