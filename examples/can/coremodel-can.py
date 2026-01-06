from coremodel import *
import time

class Can(CoreModelCan):
    def __init__(self, busname, devname):
        super().__init__(busname = busname)
        self.devname = devname

    def tx(self, ctrl, data):
        dlen = CAN_DATALEN[(ctrl[0] & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT]
        rxctrl = [CAN_CTRL_ERTR | (0x3FFFF << CAN_CTRL_EID_SHIFT) | (0x456 << CAN_CTRL_ID_SHIFT), 0]
        rx = [0]

        if dlen > 0:
            print("[ctrl0 ", hex(ctrl[0]), " ctrl1 ", hex(ctrl[1]), " length ", dlen, " address ", hex((ctrl[0] & CAN_CTRL_ID_MASK) >> CAN_CTRL_ID_SHIFT),"]", end = "", sep = "")
            print()
            for idx in range(dlen):
                print(hex(data[idx]), end =" ")
            print()
        else:
            print("[", ctrl[0], " ", ctrl[1],"]", sep = "")

        ret = self.rx(rxctrl, rx)

        if ret == 1:
            print("Rx send failed")

        return CAN_ACK

    def rxcomplete(self, nak):
        print(" ->", nak)

    def __del__(self):
        pass

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("usage: python3 coremodel-can.py <address> <port> <can bus>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]

    cm0 = CoreModel("cm0", address, port, "./../../")

    can0 = Can(busname,"can-test")

    try:
        cm0.attach(can0)
    except NameError as e:
        print(str(e))
        sys.exit(1)

    cm0.cycle_time = 8000000

    cm0.start()

    for i in range(3):
        time.sleep(1)

    rxctrl = [(0x456 << CAN_CTRL_ID_SHIFT) | (5 << CAN_CTRL_DLC_SHIFT), 0]
    rx = [0xa5, 0xa6, 0xa7, 0xa8, 0xa9]

    for i in range(3):
        can0.rx(rxctrl, rx)
        time.sleep(1)

    cm0.stop_event.set()
    cm0.join()
