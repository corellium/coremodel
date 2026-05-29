# CoreModel SPI Mux Example
#
# Copyright (c) 2026 Corellium Inc.
# SPDX-License-Identifier: Apache-2.0

from coremodel import *
import time

CHANNEL_LIMIT = 8
PIN_NUM       = 4

class GPIOPin(CoreModelGpio):
    def __init__(self, busname , pin, spimux):
        super().__init__(busname = busname, pin = pin)
        self.mvolt = 0
        self.spimux = spimux
    def notify(self, mvolt):
        pin_idx = self.spimux.gpiopins.index(self)
        print("gpio notifiy idx {}".format(pin_idx))
        self.spimux.chanp = self.spimux.chan

        if pin_idx < 3:
            self.mvolt = mvolt
            self.spimux.chan &= ~(1 << pin_idx)
            self.spimux.chan |= (not not self.mvolt) << pin_idx
        elif pin_idx == 3:
            self.mvolt = mvolt
            self.spimux.en = not not self.mvolt;

        print("chanp {} chan {}".format( self.spimux.chanp, self.spimux.chan))
        self.spimux.update_pin = 1

    def __del__(self):
        pass

class SPIChannel:
    def __init__(self, devname):
        self.device = CoreModelSpi
        self.devname = devname
    def __del__(self):
        pass

class SpiMux():
    def __init__(self, busname, cs, gpiobus, devname):
        self.busname = busname
        self.gpiobus = gpiobus
        self.cs = cs
        self.devname = devname
        self.gpiopins = [GPIOPin] * PIN_NUM

        self.channel = [SPIChannel] * CHANNEL_LIMIT

        self.count = 0
        self.chan = 0
        self.chanp = 0
        self.update_pin = 0
        self.en = 0

    def spi_mux_connect(self, channel, obj):
        if channel >= CHANNEL_LIMIT:
            return 1
        self.channel[channel].device = obj
        return 0

    def __del__(self):
        pass

if __name__ == "__main__":
    if len(sys.argv) != 10:
        print("usage: python3 coremodel-spi.py <address> <port> <spi bus> <chip select> <gpio> [gpio A0...A2] [gpio EN]")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]
    spi_cs = int(sys.argv[4])
    gpiobus = sys.argv[5]

    gpios = [0] * PIN_NUM

    cm0 = CoreModel("cm0", address, port, "./../../")

    spimux0 = SpiMux(busname, spi_cs, gpiobus,"spi-device")

    for i in range(PIN_NUM):
        gpios[i] = int(sys.argv[6 + i])
        spimux0.gpiopins[i] = GPIOPin(gpiobus, gpios[i], spimux0)
        cm0.attach(spimux0.gpiopins[i])

    for i in range(CHANNEL_LIMIT):
        spimux0.channel[i].device = CoreModelSpi(spimux0.busname, spimux0.cs)
        spimux0.channel[i].devname = "spi-dummy:{}".format(i)
        spimux0.channel[i].device.flags = COREMODEL_SPI_BLOCK

    spimux0.chanp = 0
    spimux0.chan = 0
    spimux0.update_pin = 0

    while 1:
        res = cm0.mainloop()
        if res < 0:
            break

        if spimux0.update_pin:
            spimux0.update_pin = 0
            if spimux0.channel[spimux0.chanp].device.handle is not None:
                print("detach channel {}".format(spimux0.chanp))
                cm0.detach(spimux0.channel[spimux0.chanp].device)
                spimux0.channel[spimux0.chanp].device.handle = None

            cm0.mainloop()

            if spimux0.en:
                cm0.attach(spimux0.channel[spimux0.chan].device)
                if spimux0.channel[spimux0.chan].device.handle:
                    print("attached channel {}".format(spimux0.chan))
                else:
                    print("failed to attached {}".format(spimux0.chan))

            cm0.mainloop()