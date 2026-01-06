from coremodel import *
import time
from datetime import datetime

class ds3231(CoreModelI2C):
    def __init__(self, busname, address, devname):
        super().__init__(busname = busname, address = address)
        self.devname = devname

        self.system_time = time.time()
        self.index = 0
        self.started = 0
        self.twelve_hour_flag = 0
        self.Alarm1_Sec = 0
        self.Alarm1_Min = 0
        self.Alarm1_Hours = 0
        self.Alarm1_DayDate = 1
        self.Alarm2_Min = 0
        self.Alarm2_Hours = 0
        self.Alarm2_DayDate = 1
        self.control = 0b00011100
        self.status = 0b10001000
        self.aging = 0

    # Convert a binary value into 7 bit BCD
    def format_7bit_bcd(self, binary_value):
        return ((int(binary_value / 10) << 4) + binary_value % 10) & 0x7f

    # Convert a binary value into 5 bit BCD
    def format_6bit_bcd(self, binary_value):
        return self.format_7bit_bcd(binary_value) & 0x3f

    # Convert a binary value into 4 bit BCD
    def format_5bit_bcd(self, binary_value):
        return self.format_7bit_bcd(binary_value) & 0x1f

    # Print Enabled or disabled based on the high bit
    def print_enabled(self, value):
        if (value & 0x80) > 0:
            print("Alarm Enabled")
        else:
            print("Alarm Disabled")

    def start(self):
        print("START")

        # read the system time

        dt_object = datetime.fromtimestamp(self.system_time)
        old_second = dt_object.second
        old_minute = dt_object.minute
        self.system_time = time.time()
        dt_object = datetime.fromtimestamp(self.system_time)

        self.started = 1

        # calculating alarm 1
        if (self.Alarm1_Sec & 0x80) > 0: # A1M1 is high
            if old_second != self.format_7bit_bcd(dt_object.second):
                self.status |= 0b1
        elif (self.Alarm1_Min & 0x80) > 0: #A1M2 is high
            if (self.Alarm1_Sec & 0x7f) == self.format_7bit_bcd(dt_object.second):
                self.status |= 0b1
        elif (self.Alarm1_Hours & 0x80) > 0: #A1M3 is high
            if (self.Alarm1_Min & 0x7f) == self.format_7bit_bcd(dt_object.minute):
                self.status |= 0b1
        elif (self.Alarm1_DayDate & 0x80) > 0: #A1M4 is high
            if self.twelve_hour_flag > 0:
                if dt_object.hour > 11:
                    if (self.Alarm1_Hours & 0x7f) == self.format_5bit_bcd(dt_object.hour - 12) + 0b01100000:
                        self.status |= 0b1
                else:
                    if (self.Alarm1_Hours & 0x7f) == self.format_5bit_bcd(dt_object.hour - 12) + 0b01000000:
                        self.status |= 0b1

            else:
                if (self.Alarm1_Hours & 0x7f) == self.format_6bit_bcd(dt_object.hour):
                    self.status |= 0b1

        elif (self.Alarm1_DayDate & 0x40) > 0: # DY/!DT is high
            if (self.Alarm1_DayDate & 0x3f) == dt_object.day + 1:
                self.status |= 0b1
        else: # all flags are low
            if (self.Alarm1_DayDate & 0x3f) == self.format_6bit_bcd(dt_object.day):
                self.status |= 0b1

        # calculating alarm 2
        if (self.Alarm2_Min & 0x80) > 0: #A1M2 is high
            if old_minute != self.format_7bit_bcd(dt_object.minute):
                self.status |= 0b10
        elif (self.Alarm2_Hours & 0x80) > 0: #A1M3 is high
            if (self.Alarm2_Min & 0x7f) == self.format_7bit_bcd(dt_object.minute):
                self.status |= 0b10
        elif (self.Alarm2_DayDate & 0x80) > 0: #A1M4 is high
            if self.twelve_hour_flag > 0:
                if dt_object.hour > 11:
                    if (self.Alarm2_Hours & 0x7f) == self.format_5bit_bcd(dt_object.hour - 12) + 0b01100000:
                        self.status |= 0b10
                else:
                    if (self.Alarm2_Hours & 0x7f) == self.format_5bit_bcd(dt_object.hour - 12) + 0b01000000:
                        self.status |= 0b10

            else:
                if (self.Alarm2_Hours & 0x7f) == self.format_6bit_bcd(dt_object.hour):
                    self.status |= 0b10

        elif (self.Alarm2_DayDate & 0x40) > 0: # DY/!DT is high
            if (self.Alarm2_DayDate & 0x3f) == dt_object.day + 1:
                self.status |= 0b10
        else: # all flags are low
            if (self.Alarm2_DayDate & 0x3f) == self.format_6bit_bcd(dt_object.day):
                self.status |= 0b10

        # note this code does not check if an alarm should have gone off since the
        # last START, rather it only alerts if it should be on now
        return 1

    def write(self, data):

        offset = 0
        temp = 0x80
        length = len(data)
        if self.started == 1:
            print("WRITE Addr [",length ,"]:", hex(data[0]))
            self.index = data[0]
            self.started = 0
            if length == 1:
                return length

        print("WRITE [", length,"]:", sep="")

        for idx in range( 1, length):
            offset = (idx + self.index) % 0x13
            if offset == 0: # BCD Seconds
                print("Ignoring write of", hex(data[idx]), "to seconds register")

            elif offset == 1: # BCD Minutes
                print("Ignoring write of", hex(data[idx]), "to minutes register")

            elif offset == 2: # BCD Hours
                if (data[idx] & 0x40) > 0:
                    print("Setting AM/PM mode")
                    self.twelve_hour_flag = 1
                else:
                    print("Setting 24 hour mode")
                    self.twelve_hour_flag = 0

            elif offset == 3: # 1 based day of week
                print("Ignoring write of", hex(data[idx]), "to day of week register")

            elif offset == 4: # 1 based day of month
                print("Ignoring write of", hex(data[idx]), "to day of month register")

            elif offset == 5: # centry + month
                print("Ignoring write of %02x to month register", data[idx])

            elif offset == 6: # year
                print("Ignoring write of", hex(data[idx]), "to year register")

            elif offset == 7: # Alarm 1 Seconds
                self.Alarm1_Sec = data[idx]
                print("Setting Alarm 1 Seconds [00-59]:", hex(data[idx] & 0x7f))
                self.print_enabled(data[idx])

            elif offset == 8: # Alarm 1 Minutes
                    self.Alarm1_Min = data[idx]
                    print("Setting Alarm 1 Minutes [00-59]:", hex(data[idx] & 0x7f))
                    self.print_enabled(data[idx])

            elif offset == 9: # Alarm 1 Hours
                    self.Alarm1_Hours = data[idx]
                    print("Setting Alarm 1 Hours [00-23]:", hex(data[idx] & 0x6f))
                    self.print_enabled(data[idx])

            elif offset == 0xA: # Alarm 1 Day Date
                self.Alarm1_DayDate = data[idx]
                if (data[idx] & 0x40) > 0:
                    print("Setting Alarm 1 Day [1-7]:", hex(data[idx] & 0x3f))
                else:
                    print("Setting Alarm 1 Day of Month [1-31]:", hex(data[idx] & 0x3f))

                self.print_enabled(data[idx])

            elif offset == 0xB: # Alarm 2 Minutes
                self.Alarm2_Min = data[idx]
                print("Setting Alarm 2 Minutes [00-59]:", hex(data[idx] & 0x7f))
                self.print_enabled(data[idx])

            elif offset == 0xC: # Alarm 2 Hours
                self.Alarm2_Hours = data[idx]
                print("Setting Alarm 2 Hours [00-23]:", hex(data[idx] & 0x6f))
                self.print_enabled(data[idx])

            elif offset == 0xD: # Alarm 2 Day Date
                self.Alarm1_DayDate = data[idx]
                if (data[idx] & 0x40) > 0:
                    print("Setting Alarm 2 Day [1-7]:", hex(data[idx] & 0x3f))
                else:
                    print("Setting Alarm 2 Day of Month [1-31]:", hex(data[idx] & 0x3f))

                self.print_enabled(data[idx])

            elif offset == 0xE: # Control: EOSC BBSQW CONV RS2 RS1 INTCN A2IE A1IE not implemented
                self.control = data[idx] & 0x1f
                print("Setting Oscillator Enabled, ", end="")
                print("Square-Wave Disabled, ", end="")
                print("Temp Conversion Finished, ", end="")

                check = (self.control >> 3) & 0b11

                if check == 0b00:
                    print("Frequency 1Hz, ", end="")
                elif check == 0b01:
                    print("Frequency 1.024kHz, ", end="")
                elif check == 0b10:
                    print("Frequency 4.096kHz, ", end="")
                elif check == 0b11:
                    print("Frequency 8.192kHz, ", end="")



                if (self.control & 0b100) > 0:
                    print("Interrupt Mode, ", end="")
                else:
                    print("Oscillator Mode, ", end="")

                if (self.control & 0b10) > 0:
                    print("Alarm 2 Enabled, ", end="")
                else:
                    print("Alarm 2 Disabled, ", end="")

                if (self.control & 0b1) > 0:
                    print("Alarm 1 Enabled", end="")
                else:
                    print("Alarm 1 Disabled", end="")

            elif offset == 0xF: # Status: OSF 0 0 0 EN32kHz BSY A2F A1F
                if (data[idx] & 0x80) > 0: # always active, has no effect
                    print("Enabling Oscillator, ", end="")
                if (data[idx] & 0b1000) > 0:
                    print("Enabling 32kHz Output, ", end="")
                    temp = 0b1000
                else:
                    print("Disabling 32kHz Output, ", end="")

                if (data[idx] & 0b10) > 0:
                    temp += self.status & 0b10
                else:
                    print("Clearing Alarm 2, ", end="")

                if (data[idx] & 0b1) > 0:
                    temp += self.status & 0b1
                else:
                    print("Clearing Alarm 1", end="")

                self.status = temp

            elif offset == 0x10: # Aging offset has no user effect
                self.aging = data[idx]
                print("Aging offset", hex(data[idx]))

            elif offset == 0x11: # Temperature upper byte not implemented
                pass

            elif offset == 0x12: # Temperature lower byte not implemented
                print("The Temperature is read only", end="")

            else:
                print( hex(data[idx]), end="")

        print("")

        self.index = (offset + 1) % 0x13
        return length

    def read(self, data):

        offset = 0
        length = len(data)
        dt_object = datetime.fromtimestamp(self.system_time)

        print("READ [", length,"]: ", sep="", end="")

        for idx in range(length):
            offset = (idx + self.index) % 0x13
            if offset == 0: # BCD Seconds
                # reread clock every time reg 0 is read
                dt_object = datetime.fromtimestamp(self.system_time)
                data[idx] = self.format_7bit_bcd(dt_object.second)
                print("Seconds [00-59]:", hex(data[idx]), end="")

            elif offset == 1: # BCD Minutes
                data[idx] = self.format_7bit_bcd(dt_object.minute)
                print("Minutes [00-59]:", hex(data[idx]), end="")

            elif offset == 2: # BCD Hours
                if self.twelve_hour_flag > 0:
                    if dt_object.hour > 11:
                        data[idx] = self.format_5bit_bcd(dt_object.hour - 12) + 0b01100000
                    else:
                        data[idx] = self.format_5bit_bcd(dt_object.hour - 12) + 0b01000000

                    print("Hours flags + [00-11]:", hex(data[idx]), end="")
                else:
                    data[idx] = self.format_6bit_bcd(dt_object.hour)
                    print("Hours [00-23]:", hex(data[idx]), end="")


            elif offset == 3: # 1 based day of week
                data[idx] = dt_object.day + 1
                print("Day of Week [1-7]:", hex(data[idx]), end="")

            elif offset == 4: # 1 based day of month
                data[idx] = self.format_6bit_bcd(dt_object.day)
                print("Day of Month [1-31]:", hex(data[idx]), end="")

            elif offset == 5: # centry + month
                data[idx] = (int(dt_object.year / 100) << 7)+ self.format_5bit_bcd(dt_object.month + 1)
                print("Month [1-12]", hex(data[idx] & 0b11111), "+ [Century]", data[idx] >> 7, "=", data[idx], end="")

            elif offset == 6: # year
                data[idx] = (int((dt_object.year % 100) / 10) << 4) + dt_object.year % 10
                print("Year since 1900 [0-99]:", hex(data[idx]), end="")

            elif offset == 7: # Alarm 1 Seconds
                data[idx] = self.Alarm1_Sec
                print("Alarm 1 Seconds [00-59]:", hex(data[idx] & 0x7f), end="")
                self.print_enabled(data[idx])

            elif offset == 8: # Alarm 1 Minutes
                data[idx] = self.Alarm1_Min
                print("Alarm 1 Minutes [00-59]:", hex(data[idx] & 0x7f), end="")
                self.print_enabled(data[idx])

            elif offset == 9: # Alarm 1 Hours
                data[idx] = self.Alarm1_Hours
                print("Alarm 1 Hours [00-23]:", hex(data[idx] & 0x6f), end="")
                self.print_enabled(data[idx])

            elif offset == 0xA: # Alarm 1 Day Date
                data[idx] = self.Alarm1_DayDate
                if (data[idx] & 0x40) > 0:
                    print("Alarm 1 Day [1-7]:", hex(data[idx] & 0x3f), end="")
                else:
                    print("Alarm 1 Day of Month [1-31]:", hex(data[idx] & 0x3f), end="")

                self.print_enabled(data[idx])

            elif offset == 0xB: # Alarm 2 Minutes
                data[idx] = self.Alarm2_Min
                print("Alarm 2 Minutes [00-59]:", hex(data[idx] & 0x7f), end="")
                self.print_enabled(data[idx])

            elif offset == 0xC: # Alarm 2 Hours
                data[idx] = self.Alarm2_Hours
                print("Alarm 2 Hours [00-23]:", hex(data[idx] & 0x6f), end="")
                self.print_enabled(data[idx])

            elif offset == 0xD: # Alarm 2 Day Date
                data[idx] = self.Alarm1_DayDate
                if (data[idx] & 0x40) > 0:
                    print("Alarm 2 Day [1-7]:", hex(data[idx] & 0x3f), end="")
                else:
                    print("Alarm 2 Day of Month [1-31]:", hex(data[idx] & 0x3f), end="")

                self.print_enabled(data[idx])

            elif offset ==  0xE: # Control: EOSC BBSQW CONV RS2 RS1 INTCN A2IE A1IE not implemented
                data[idx] = self.control
                print("Oscillator Enabled,", end="")
                print("Square-Wave Disabled,", end="")
                print("Temp Conversion Finished,", end="")
                control = (self.control >> 3) & 0b11
                if control == 0b00:
                    print("Frequency 1Hz,", end="")

                elif control == 0b01:
                    print("Frequency 1.024kHz,", end="")

                elif control ==  0b10:
                    print("Frequency 4.096kHz,", end="")

                elif control ==  0b11:
                    print("Frequency 8.192kHz,", end="")


                if (self.control & 0b100) > 0:
                    print("Interrupt Mode,", end="")
                else:
                    print("Oscillator Mode,", end="")

                if (self.control & 0b10) > 0:
                    print("Alarm 2 Enabled,", end="")
                else:
                    print("Alarm 2 Disabled,", end="")

                if (self.control & 0b1) > 0:
                    print("Alarm 1 Enabled", end="")
                else:
                    print("Alarm 1 Disabled", end="")


            elif offset == 0xF: # Status: OSF 0 0 0 EN32kHz BSY A2F A1F
                data[idx] = self.status
                print("Oscillator Running,", end="") # bit 7
                if (self.status & 0b1000) > 0: # bit 3
                    print("32kHz Output Enabled,", end="")
                else:
                    print("32kHz Output Disabled,", end="")

                print("Temp Conversion Finished,", end="") # bit 2
                if (self.status & 0b10) > 0:
                    print("Alarm 2 Active,", end="")
                else:
                    print("Alarm 2 Inactive,", end="")

                if (self.status & 0b1) > 0:
                    print("Alarm 1 Active", end="")
                else:
                    print("Alarm 1 Inactive", end="")


            elif offset == 0x10: # Aging offset has no user effect
                data[idx] = self.aging
                print("Aging offset", hex(data[idx]), end="")

            elif offset == 0x11: # Temperature upper byte not implemented
            # statically return 25C
                data[idx] = 0b00011001
                print("Temperature Upper:", data[idx], "C", end="")

            elif offset == 0x12: # Temperature lower byte not implemented
            # statically return .25C
                data[idx] = 1 << 6
                print("Temperature Lower:", (data[idx] > 6) * 25, "C", end="")

        print("")
        self.index = (offset + 1) % 0x13
        return length

    def stop(self):
        print("stop")

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("usage: python3 i2c-ds3231.py <address> <port> <i2c bus> <i2c address>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]
    i2c_address = int(sys.argv[4])

    cm0 = CoreModel("cm0", address, port, "./../../")

    ds3231_0 = ds3231(busname, i2c_address, "ds3231_0")

    try:
        cm0.attach(ds3231_0)
    except NameError as e:
        print(str(e))
        sys.exit(1)

    cm0.cycle_time = 100000

    cm0.start()

    for i in range(10):
        time.sleep(1)

    cm0.stop_event.set()
    cm0.join()