#
# CoreModel Ethernet Example
#
# Copyright (c) 2022-2026 Corellium Inc.
# SPDX-License-Identifier: Apache-2.0

from coremodel import *
import time

class Ethernet(CoreModelEth):
    def __init__(self, busname, devname):
        super().__init__(busname=busname)
        self.devname = devname

    def hexdump(self, addr, buf, length):
        ascii = [""] * 17
        data = [*buf]

        print("{:<20}".format(hex(id(addr))), end="")
        for i in range(length):
            if (i != 0) and (0 == (i % 16)):
                for j in range(len(ascii)):
                    if ascii[j] == '.' or ascii[j] == '' or ascii[j] == ' ':
                        print("{}".format(ascii[j]), sep="", end="")
                    else:
                        print("{}".format(chr(int(ascii[j]))), sep="", end="")
                print("    \n\r{:<16}    ".format(hex(id(addr[i]))), end="")

            if (data[i] >= 0x20) and (data[i] <= 0x7e):
                ascii[i % 16] = data[i]
            else:
                ascii[i % 16] = '.'

            print("{:#04x} ".format(data[i]), end="")


        # /* Special case when len != 16 byte multiple */
        if 0 != (length % 16):
            for j in range(16 - (length % 16)):
                print("     ", sep="", end="")
                ascii[ (int(length) + j) % 16 ] = ' '

        for j in range(len(ascii)):
            if ascii[j] == '.' or ascii[j] == '' or ascii[j] == ' ':
                print("{}".format(ascii[j]), sep="", end="")
            else:
                print("{}".format(chr(int(ascii[j]))), sep="", end="")
        print("")

    def tx(self, length, data):
        print("ETH TX! {}".format(hex(length)))
        self.hexdump(data, data, length)

        # Simple loopback
        self.rx(length, data)

        return length

    def rxrdy(self):
        print("RXRDY")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("usage: python3 coremodel-eth.py <address> <port> <eth bus>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]

    cm0 = CoreModel("cm0", address, port, "./../../")

    eth0 = Ethernet(busname, "eth0")

    try:
        cm0.attach(eth0)
    except NameError as e:
        print(str(e))
        sys.exit(1)

    cm0.cycle_time = 1000000
    cm0.start()

    for i in range(10):
        time.sleep(1)

    cm0.stop_event.set()
    cm0.join()