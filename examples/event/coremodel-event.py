from coremodel import *
import time

from coremodel import CoreModelEvent


class Event(CoreModelEvent):
    def __init__(self,eventname):
        super().__init__(eventname=eventname)

    def update(self, data0, data1, initial):
        print("event[{}] data0 {} data1 {} initial {}".format(self.name, data0, data1, initial))


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("usage: python3 coremodel-event.py <address> <port> <eventname0|eventname0=data0,data1> ... <eventnameN>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    cm0 = CoreModel("cm0", address, port, "./../../")

    event_list = []
    event_name = []
    event_data0 = []
    event_data1 = []

    for event in sys.argv[3:]:
        idx = event.find('=')
        if idx > 0:
            event_name.append(event[:idx])
            d1 = event[idx+1:].find(',')
            if d1 > 0:
                event_data0.append(int(event[idx + 1:idx + 1 +d1]))
                event_data1.append(int(event[idx + d1 + 2:]))
            else:
                event_data0.append(int(event[idx + 1:]))
                event_data1.append(0)
        else:
            event_name.append(event)
            event_data0.append(0)
            event_data1.append(0)

    for i in range(len(event_name)):
        event_list.append(Event(event_name[i]))

    for event in event_list:
        try:
            cm0.attach(event)
        except NameError as e:
            print(str(e), event.name)
            sys.exit(1)

    if cm0.attached_objs is None:
        sys.exit(1)

    cm0.cycle_time = 1000000

    cm0.start()
    for i in range(10):
        for j in range(len(event_list)):
            try:
                event_list[j].event_signal(event_data0[j], event_data1[j], 0)
            except Exception as e:
                print(str(e), event_list[j].name)
            event_data0[j] = int(not event_data0[j])
            event_data1[j] = int(not event_data1[j])
        time.sleep(1)
    cm0.stop_event.set()
    cm0.join()