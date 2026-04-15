from coremodel import *
import time

class gpio(CoreModelGpio):
    def __init__(self, busname, pin):
        super().__init__(busname = busname, pin = pin)
        self.mvolt = 0
        self.driven = 0

    def notify(self, mvolt):
        self.mvolt = mvolt
        print("PIN", self.pin, "MVOLT", self.mvolt, "DRIVEN", self.driven)

    def __del__(self):
        pass

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print("usage: python3 coremodel-gpio.py <address> <port> <gpio bus> <gpio> ... <gpion>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]

    cm0 = CoreModel("cm0", address, port, "./../../")

    gpio_list = []
    for pin in sys.argv[4:]:
        gpio_list.append(gpio(busname, int(pin)))

    for gpio in gpio_list:
        try:
            cm0.attach(gpio)
        except NameError as e:
            print(str(e), gpio.pin)
    if cm0.attached_objs is None:
        sys.exit(1)

    cm0.cycle_time = 8000000

    cm0.start()
    for i in range(10):
        time.sleep(1)
    cm0.stop_event.set()
    cm0.join()
