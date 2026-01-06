from coremodel import *
import time

class sender(CoreModelCan):
    def __init__(self, busname, devname):
        super().__init__(busname = busname)

    def tx(self, ctrl, data):
        dlen = CAN_DATALEN[(ctrl[0] & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT]
        rxctrl = [CAN_CTRL_ERTR | (0x3FFFF << CAN_CTRL_EID_SHIFT) | (0x456 << CAN_CTRL_ID_SHIFT), 0]
        rx = [0]

        if dlen > 0:
            print("[", ctrl[0], " ", ctrl[1], "] len ", dlen, sep="")
            for idx in range(dlen):
                print(hex(data[idx]), end=" ")
            print()
        else:
            print("[", ctrl[0], " ", ctrl[1], "]", sep="")

        return CAN_ACK

    def rxcomplete(self, nak):
        print("rx complete ->", nak)

    def __del__(self):
        pass

if __name__ == "__main__":
    if len(sys.argv) != 5 or len(sys.argv[4]) < 4 or sys.argv[4][3] != '#' or len(sys.argv[4]) > 20:
        print("usage: python3 coremodel-cansend.py <address> <port> <can> <data>")
        print("data is 3 hex character address followed by the # character")
        print("payload is up to 16 hex characters (8 bytes)")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]

    input_str = sys.argv[4]
    input_len = len(input_str)
    data_length = input_len - 4  # remove address length
    character = '0'
    if (data_length % 2):
        dln = int((data_length - 1) / 2)  # drop an extra char
    else:
        dln = int(data_length / 2)  # convert to bytes

    # process the address argument
    addr = 0  # address to send to
    #    for i in range (3):
    try:
        addr = int(input_str[:3], 16)
    except:
        print("error: invalid address.", file=sys.stderr)
        sys.exit(1)

    rxctrl = [0, 0]  # return control message, 2 64bit words
    rxctrl[0] |= (addr << CAN_CTRL_ID_SHIFT)  # set ID
    rxctrl[0] |= (dln << CAN_CTRL_DLC_SHIFT)  # set size

    datastr = input_str[4:20]
    rxdata = bytearray.fromhex(datastr) # convert data string to hex

    for d in rxdata:
        print(hex(d), end=" ")
    print()

    cm0 = CoreModel("cm0", address, port, "./../../")

    sender0 = sender(busname,"can-send")

    try:
        cm0.attach(sender0)
    except NameError as e:
        print(str(e))
        sys.exit(1)

    cm0.cycle_time = 500000

    cm0.start()

    for i in range(5):
        sender0.rx(rxctrl, rxdata)
        time.sleep(1)

    cm0.stop_event.set()
    cm0.join()