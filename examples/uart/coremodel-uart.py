# CoreModel UART Example
#
# Copyright (c) 2022-2026 Corellium Inc.
# SPDX-License-Identifier: Apache-2.0

from coremodel import *
import time

class uart(CoreModelUart):
    def __init__(self, busname):
        super().__init__(busname = busname)

    def tx(self, data):
        length_of_data_consumed = len(data)
        print(data.decode("utf-8"), end = "")
        return length_of_data_consumed


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("usage: python3 coremodel-uart.py <address> <port> <uart bus>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]

    cm0 = CoreModel("cm0", address, port, "./../../")

    uart0 = uart(busname)

    try:
        cm0.attach(uart0)
    except NameError as e:
        print(str(e))
        sys.exit(1)

    cm0.cycle_time = 8000000

    cm0.start()

    for i in range(3):
        time.sleep(1)


    byte_data = bytearray([72, 101, 108, 108, 111, 10])

    ret = uart0.rx(byte_data)


    for i in range(3):
        time.sleep(1)

    cm0.stop_event.set()
    cm0.join()
