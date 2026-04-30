# CoreModel I2C Mux Example
#
# Copyright (c) 2026 Corellium Inc.
# SPDX-License-Identifier: Apache-2.0

from coremodel import *

I2C_MUX_DEBUG = 0
CHANNEL_LIMIT = 8

class I2CChannel:
    def __init__(self):
        self.device = [CoreModelI2C] * 128 # [CoreModelI2C] * 128
        self.count = 0
    def __del__(self):
        pass

class I2CMux(CoreModelI2C):
    def __init__(self, busname, address, devname):
        super().__init__(busname = busname, address = address)
        self.devname = devname
        self.flags = COREMODEL_I2C_WRITE_ACK
        self.channel = [I2CChannel] * 8
        for i in range(CHANNEL_LIMIT):
            self.channel[i].__init__(self.channel[i])
            self.channel[i].count = 0
        self.count = 0
        self.addr = 0
        self.chan = 0
        self.chanp = 0
        self.writef = 0
        self.stopf = 0
        self.handle = None
        self.cm = None

    def i2c_mux_connect(self, channel, obj):

        print("i2c_mux_connect {} addr {} channel {} ".format( name, obj.address, channel))

        if (channel >= CHANNEL_LIMIT) or (self.channel[channel].count >= 128):
            return 1
        # print("channel {} channel length {} device len {}".format( channel, len(self.channel), len(self.channel[channel].device)))
        self.channel[channel].device[self.channel[channel].count] = obj
        self.channel[channel].count += 1
        return 0

    def i2c_mux_update_channels(self):

        changed = self.chanp ^ self.chan

        for i in range(CHANNEL_LIMIT):
            if changed & (1 << i) :
                if self.chan & (1 << i):
                    for j in range(self.channel[i].count):

                        self.cm.attach(self.channel[i].device[j])
                        if not self.channel[i].device[j].handle:
                            print("failed to attach {}} with address {} on channel {} ".format( self.channel[i].device[j].name, self.channel[i].device[j].address, i))
                        else:
                            print("attach success")

                else:
                    for j in range(self.channel[i].count):

                        if self.channel[i].device[j].handle:
                            self.cm.detach(self.channel[i].device[j])
                            self.channel[i].device[j].handle = None
                        else:
                            print("no handle to detach {} with address {} on channel {} ".format(self.channel[i].device[j].devname, self.channel[i].device[j].addr, i))


    def start(self):
        print("START")
        return 1

    def write(self, data):
        length_of_data_consumed = len(data)

        for i in range(length_of_data_consumed):

            if I2C_MUX_DEBUG:
                print("i2c_mux write data {} length {}".format( data[i], len))

                if self.count == 0:
                    self.chanp = self.chan
                    self.writef = 1

                self.chan = data[i]
                self.count += 1

            return length_of_data_consumed

        return length_of_data_consumed

    def read(self, data):
        length_of_data_consumed = len(data)
        print("READ {}", len)

        for i in range(length_of_data_consumed):
            data[i] = self.chan
        return length_of_data_consumed

    def __del__(self):
        pass

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("usage: coremodel-mux <address> <ports> <i2c>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    res = 0
    name = ""

    cm0 = CoreModel("cm0", address, port, "./../../")

    i2cmux0 = I2CMux(sys.argv[3],0x70, "i2c-mux:0")

    cm0.attach(i2cmux0)

    i2c_dummies = [CoreModelI2C] * CHANNEL_LIMIT

    for i in range(CHANNEL_LIMIT):
        name =  "i2c-dummy:{}".format(i)
        print("index {}".format(i))
        i2c_dummies[i] = CoreModelI2C(i2cmux0.name, 0x60 + i)
        i2c_dummies[i].devname = name
        i2cmux0.i2c_mux_connect(i, i2c_dummies[i])

    while(1):
        cm0.mainloop()
        if i2cmux0.writef and i2cmux0.stopf:
            i2cmux0.writef = 0
            i2cmux0.stopf = 0
            i2cmux0.i2c_mux_update_channels()



