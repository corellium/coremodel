from coremodel import *
import time

class Spi(CoreModelSpi):
    def __init__(self, busname, cs, devname):
        super().__init__(busname = busname, cs = cs)
        self.devname = devname

    def cs(self, csel):
        print("cs ", csel)

    def xfr(self, wrdata, rddata):
        for idx, byte in enumerate(wrdata):
            print("WRITE idx [", idx, "] value", hex(byte))

        for idx in range(len(rddata)):
            # maintain c style (uint8_t *) access when writing data to maintain mutability
            rddata[idx] = 0xa0 + (idx & 0x3f)

        return len(rddata)

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("usage: python3 coremodel-spi.py <address> <port> <spi bus> <chip select>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]
    spi_cs = sys.argv[4]

    cm0 = CoreModel("cm0", address, port, "./../../")

    spi0 = Spi(busname, int(spi_cs), "spi-device")

    try:
        cm0.attach(spi0)
    except NameError as e:
        print(str(e))
        sys.exit(1)

    cm0.cycle_time = 100000

    cm0.start()

    for i in range(3):
        time.sleep(1)

    cm0.stop_event.set()
    cm0.join()