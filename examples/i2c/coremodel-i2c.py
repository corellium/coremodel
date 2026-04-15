from coremodel import *
import time

class I2C0(CoreModelI2C):
    def __init__(self, busname, address, devname):
        super().__init__(busname = busname, address = address)
        self.devname = devname
        self.regaddr = 0
        self.count = 0

    def start(self):
        print("start")
        return 1

    def write(self, data):
        length_of_data_consumed = len(data)
        print("WRITE len [", length_of_data_consumed, "]")

        for idx, byte in enumerate(data):
            if self.count == 0:
                self.regaddr = data[0]
            print("WRITE idx [", idx, "] value", hex(byte))
            self.count += 1
        return length_of_data_consumed

    def read(self, data):
        length_of_data_consumed = len(data)
        print("READ len [", length_of_data_consumed,"]", "register", hex(self.regaddr))
        for idx in range(length_of_data_consumed):
            # byte array
            data[idx] = 0xa0 + (idx & 0x3f)
            self.count += 1
            self.regaddr += 1
        return length_of_data_consumed

    def stop(self):
        print("stop")
        self.regaddr = 0
        self.count = 0

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("usage: python3 coremodel-i2c.py <address> <port> <i2c bus> <i2c address>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]
    i2c_address = int(sys.argv[4])

    cm0 = CoreModel("cm0", address, port, "./../../")

    i2c0 = I2C0(busname, i2c_address, "i2c_device_name")

    try:
        cm0.attach(i2c0)
    except NameError as e:
        print(str(e))
        sys.exit(1)

    cm0.cycle_time = 100000

    cm0.start()

    for i in range(5):
        time.sleep(1)

    cm0.stop_event.set()
    cm0.join()
